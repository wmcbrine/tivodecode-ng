/*
 * tivodecode-ng
 * Copyright 2006-2015, Jeremy Drake et al.
 * See COPYING file for license terms
 */
#ifdef HAVE_CONFIG_H
#include "tdconfig.h"
#endif

#include <cstdio>
#include <cstring>

#include "hexlib.hxx"
#include "tivo_parse.hxx"
#include "tivo_decoder_ts.hxx"
#include "tivo_decoder_mpeg_parser.hxx"

TiVoDecoderTsStream::TiVoDecoderTsStream(UINT16 pid)
{
    packets.clear();

    pParent        = NULL;
    stream_pid     = pid;
    stream_type_id = 0;
    stream_id      = 0;
    stream_type    = TS_STREAM_TYPE_NONE;

    std::memset(&turing_stuff, 0, sizeof(TS_Turing_Stuff));
}

void TiVoDecoderTsStream::setDecoder(TiVoDecoderTS *pDecoder)
{
    pParent = pDecoder;
}

bool TiVoDecoderTsStream::addPkt(TiVoDecoderTsPacket *pPkt)
{
    TiVoDecoderTsPacket *pPkt2 = NULL;
    bool flushBuffers = false;
    TsPackets_it pkt_iter;
    TsLengths_it len_iter;

    if (!pPkt)
    {
        std::perror("bad parameter");
        return false;
    }

    pPkt->setStream(this);

    // If this packet's Payload Unit Start Indicator is set,
    // or one of the stream's previous packet's was set, we
    // need to buffer the packet, such that we can make an
    // attempt to determine where the end of the PES headers
    // lies.   Only after we've done that, can we determine
    // the packet offset at which decryption is to occur.
    // The accounts for the situation where the PES headers
    // straddles two packets, and decryption is needed on the 2nd.

    if ((true == pPkt->getPayloadStartIndicator()) || (0 != packets.size()))
    {
        VERBOSE("Add PktID %d from PID 0x%04x to packet list : payloadStart "
                "%d listCount %zu\n", pPkt->packetId, stream_pid, 
                pPkt->getPayloadStartIndicator(), packets.size());

        packets.push_back(pPkt);

        // Form one contiguous buffer containing all buffered packet payloads
        UINT16 pesDecodeBufferLen = 0;
        std::memset(pesDecodeBuffer, 0, 10 * TS_FRAME_SIZE);
        for (pkt_iter = packets.begin(); pkt_iter != packets.end(); pkt_iter++)
        {
            pPkt2 = *pkt_iter;
            VERBOSE("DEQUE : PktID %d from PID 0x%04x\n", pPkt2->packetId,
                    stream_pid);

            std::memcpy(&pesDecodeBuffer[pesDecodeBufferLen],
                        &pPkt2->buffer[pPkt2->payloadOffset],
                        TS_FRAME_SIZE - pPkt2->payloadOffset);
            pesDecodeBufferLen += (TS_FRAME_SIZE - pPkt2->payloadOffset);
        }
        
        if (IS_VVERBOSE)
        {
            VVERBOSE("pesDecodeBufferLen %d\n", pesDecodeBufferLen);
            hexbulk(pesDecodeBuffer, pesDecodeBufferLen);
        }

        // Scan the contiguous buffer for PES headers 
        // in order to find the end of PES headers.  
        UINT16 pesHeaderLength = 0;
        TsLengths_it len_iter;
        pesHdrLengths.clear();

        bool pesParse = getPesHdrLength(pesDecodeBuffer, pesDecodeBufferLen);
        if (false == pesParse)
        {
            std::fprintf(stderr, "failed to parse PES headers : pktID %d\n", 
                         pPkt->packetId);
            return false;
        }

        for (len_iter = pesHdrLengths.begin();
             len_iter != pesHdrLengths.end(); len_iter++)
        {
            VVERBOSE("  pes hdr len : parsed : %d\n", *len_iter);
            pesHeaderLength += *len_iter;
        }
        pesHeaderLength /= 8;

        VVERBOSE("pesDecodeBufferLen %d, pesHeaderLength %d\n", 
                 pesDecodeBufferLen, pesHeaderLength);

        // Do the PES headers end in this packet ?
        if (pesHeaderLength < pesDecodeBufferLen)
        {
            VERBOSE("FLUSH BUFFERS\n");
            flushBuffers = true;

            // For each packet, set the end point for PES headers in 
            // that packet
            for (pkt_iter = packets.begin(); (pesHeaderLength > 0) &&
                 pkt_iter != packets.end(); pkt_iter++)
            {
                pPkt2 = *pkt_iter;

                VVERBOSE("  scanning PES header lengths : pktId %d\n",
                         pPkt2->packetId);
 
                while (!pesHdrLengths.empty())
                {
                    UINT16 pesHdrLen = pesHdrLengths.front();
                    pesHdrLen /= 8;
                    pesHdrLengths.pop_front();

                    VVERBOSE("  pes hdr len : checked : %d\n", pesHdrLen);

                    // Does this PES header fit completely within 
                    // remaining packet space?

                    if (pesHdrLen + pPkt2->payloadOffset + 
                        pPkt2->pesHdrOffset < TS_FRAME_SIZE)
                    {
                        VVERBOSE("  PES header fits : %d\n", pesHdrLen);
                        
                        pPkt2->pesHdrOffset += pesHdrLen;
                        pesHeaderLength     -= pesHdrLen;
                    }
                    else
                    {
                        // Packet boundary occurs within this PES header
                        // Three cases to handle :
                        //  1. pkt boundary falls within startCode
                        //     start decrypt after startCode finish in NEXT pkt
                        //  2. pkt boundary falls between startCode and payload
                        //     start decrypt at payload start in NEXT pkt
                        //  3. pkt boundary falls within payload 
                        //     start decrypt offset into the payload

                        // Case 1                        
                        UINT16 pktBoundaryOffset = TS_FRAME_SIZE - 
                            pPkt2->payloadOffset - pPkt2->pesHdrOffset;

                        VVERBOSE("  pktBoundaryOffset : %d (%d - %d - %d)\n",
                                 pktBoundaryOffset, TS_FRAME_SIZE,
                                 pPkt2->payloadOffset, pPkt2->pesHdrOffset);

                        if (pktBoundaryOffset < 4)
                        {
                            pesHdrLen -= pktBoundaryOffset;
                            pesHdrLen *= 8;
                            VVERBOSE("  pes hdr len : re-push : %d\n",
                                     pesHdrLen);
                            pesHdrLengths.push_front(pesHdrLen);

                            pesHdrLen = pesHdrLengths.front();  
                            VVERBOSE("  pes hdr len : front now : %d\n",
                                     pesHdrLen);
                        }
                        else if (pktBoundaryOffset == 4)
                        {
                            VVERBOSE("  pes hdr len : between startCode and "
                                     "payload\n");
                        }
                        else
                        {
                            VVERBOSE("  pes hdr len : inside payload\n" );
                        }

                        break;
                    }
                }
            }
        }
        else
        {
        }
    }
    else
    {
        VERBOSE("Push Back : PayloadStartIndicator %d, packets.size() %zu \n", 
            pPkt->getPayloadStartIndicator(), packets.size());
        flushBuffers = true;
        packets.push_back(pPkt);
    }
    
    if (true == flushBuffers)
    {        
        VERBOSE("Flush packets for write\n");
        
        // Loop through each buffered packet.
        // If it is encrypted, perform decryption and then write it out.
        // Otherwise, just write it out.
        for (pkt_iter = packets.begin(); pkt_iter != packets.end(); pkt_iter++)
        {
            pPkt2 = *pkt_iter;

            VERBOSE("Flushing packet %d\n", pPkt2->packetId);

            if (true == pPkt2->getScramblingControl())
            {
                pPkt2->clrScramblingControl();
                UINT8 decryptOffset = pPkt2->payloadOffset +
                    pPkt2->pesHdrOffset;
                UINT8 decryptLen = TS_FRAME_SIZE - decryptOffset;

                VERBOSE("Decrypting PktID %d from stream 0x%04x : "
                        "decrypt offset %d len %d\n", pPkt2->packetId, 
                        stream_pid, decryptOffset, decryptLen);

                if (false == decrypt(&pPkt2->buffer[decryptOffset],
                                     decryptLen))
                {
                    std::perror("Packet decrypt fails");
                    return false;
                }
            }
        
            if (IS_VVERBOSE)
            { 
                VERBOSE("Writing PktID %d from stream 0x%04x\n",
                        pPkt2->packetId, stream_pid);
                pPkt2->dump();
            }
        
            if (std::fwrite(&pPkt2->buffer[0], 1, TS_FRAME_SIZE, pOutfile) !=
                TS_FRAME_SIZE)
            {
                std::perror("Writing packet to output file");
            }
            else
            {
                VERBOSE("Wrote PktID %d from stream 0x%04x\n",
                        pPkt2->packetId, stream_pid);
            }

            delete pPkt2;
        }

        packets.clear();
    }
    else
    {
        VERBOSE("Do NOT flush packets for write\n");
    }

    return true;
}

bool TiVoDecoderTsStream::getPesHdrLength(UINT8 *pBuffer, UINT16 bufLen)
{
    TiVoDecoder_MPEG2_Parser parser(pBuffer, bufLen);
    
    bool   done      = false;
    UINT32 startCode = 0;
    UINT16 len       = 0;
    
    while ((false == done) && (false == parser.isEndOfFile()) &&
           (bufLen > parser.getReadPos()))
    {
        VVERBOSE("PES Header Offset : %d (0x%x)\n",
                 parser.getReadPos(), parser.nextbits(8));

        if (0x000001 != parser.nextbits(24))
        {
            done = true;
            continue;
        }

        len = 0;
        startCode = parser.nextbits(32);
        parser.clear();

        if (EXTENSION_START_CODE == startCode)
        {
            VVERBOSE("%-15s   : 0x%08x : %-25.25s\n", "TS PES Packet",
                     startCode, "Extension header");
            parser.extension_header(len);
        }
        else if (GROUP_START_CODE == startCode)
        {
            VVERBOSE("%-15s   : 0x%08x : %-25.25s\n", "TS PES Packet",
                     startCode, "GOP header");
            parser.group_of_pictures_header(len);
        }
        else if (USER_DATA_START_CODE == startCode)
        {
            VVERBOSE("%-15s   : 0x%08x : %-25.25s\n", "TS PES Packet",
                     startCode, "User Data header");
            parser.user_data(len);
        }
        else if (PICTURE_START_CODE == startCode)
        {
            VVERBOSE("%-15s   : 0x%08x : %-25.25s\n", "TS PES Packet",
                     startCode, "Picture header");
            parser.picture_header(len);
        }
        else if (SEQUENCE_HEADER_CODE == startCode)
        {
            VVERBOSE("%-15s   : 0x%08x : %-25.25s\n", "TS PES Packet",
                     startCode, "Sequence header");
            parser.sequence_header(len);
        }
        else if (SEQUENCE_END_CODE == startCode)
        {
            VVERBOSE("%-15s   : 0x%08x : %-25.25s\n", "TS PES Packet",
                     startCode, "Sequence End header");
            parser.sequence_end(len);
        }
        else if (ANCILLARY_DATA_CODE == startCode)
        {
            VVERBOSE("%-15s   : 0x%08x : %-25.25s\n", "TS PES Packet",
                     startCode, "Ancillary Data header");
            parser.ancillary_data(len);            
        }
        else if (startCode >= 0x101 && startCode <= 0x1AF)
        {
            VVERBOSE("%-15s   : 0x%08x : %-25.25s\n", "TS PES Packet",
                     startCode, "Slice" );    
//            parser.slice(len);
            done = true;
        }
        else if ((startCode == 0x1BD) ||
                 (startCode >= 0x1C0 && startCode <= 0x1EF))
        {
            VVERBOSE("%-15s   : 0x%08x : %-25.25s\n", "TS PES Packet",
                     startCode, "Audio/Video Stream");
            parser.pes_header(len);
        }
        else
        {
            VERBOSE("Unhandled PES header : 0x%08x\n", startCode);
            return false;
        }
        
        if (len)
        {
            VVERBOSE("%-15s   : %d : %-25.25s\n", "TS PES Packet",
                     len, "PES Hdr Len");  
            pesHdrLengths.push_back(len);
        }
    }
    
    return true;
}

/* vi:set ai ts=4 sw=4 expandtab: */

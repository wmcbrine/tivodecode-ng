/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#include <algorithm>
#include <iostream>

#include "hexlib.hxx"
#include "tivo_parse.hxx"
#include "tivo_decoder_ts.hxx"
#include "tivo_decoder_mpeg_parser.hxx"

TiVoDecoderTsStream::TiVoDecoderTsStream(uint16_t pid)
{
    packets.clear();

    pParent        = NULL;
    stream_pid     = pid;
    stream_type_id = 0;
    stream_id      = 0;
    stream_type    = TS_STREAM_TYPE_NONE;
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
        if (IS_VVERBOSE())
            std::cerr << "Add PktID " << pPkt->packetId << " from PID "
                      << stream_pid << " to packet list : payloadStart "
                      << pPkt->getPayloadStartIndicator()
                      << " listCount " << packets.size() << "\n";

        packets.push_back(pPkt);

        // Form one contiguous buffer containing all buffered packet payloads
        uint16_t pesDecodeBufferLen = 0;
        for (pkt_iter = packets.begin(); pkt_iter != packets.end(); pkt_iter++)
        {
            pPkt2 = *pkt_iter;
            if (IS_VVERBOSE())
                std::cerr << "DEQUE : PktID " << pPkt2->packetId
                          << " from PID " << stream_pid << "\n";

            std::copy(pPkt2->buffer + pPkt2->payloadOffset,
                      pPkt2->buffer + TS_FRAME_SIZE,
                      pesDecodeBuffer + pesDecodeBufferLen);
            pesDecodeBufferLen += (TS_FRAME_SIZE - pPkt2->payloadOffset);
        }

        if (IS_VVERBOSE())
        {
            std::cerr << "pesDecodeBufferLen " << pesDecodeBufferLen << "\n";
            hexbulk(pesDecodeBuffer, pesDecodeBufferLen);
        }

        // Scan the contiguous buffer for PES headers
        // in order to find the end of PES headers.
        uint16_t pesHeaderLength = 0;
        TsLengths_it len_iter;
        pesHdrLengths.clear();

        bool pesParse = getPesHdrLength(pesDecodeBuffer, pesDecodeBufferLen);
        if (false == pesParse)
        {
            std::cerr << "failed to parse PES headers : pktID "
                      << pPkt->packetId << "\n";
            return false;
        }

        for (len_iter = pesHdrLengths.begin();
             len_iter != pesHdrLengths.end(); len_iter++)
        {
            if (IS_VVERBOSE())
                std::cerr << "  pes hdr len : parsed : " << *len_iter << "\n";
            pesHeaderLength += *len_iter;
        }
        pesHeaderLength /= 8;

        if (IS_VVERBOSE())
            std::cerr << "pesDecodeBufferLen " << pesDecodeBufferLen
                      << ", pesHeaderLength " << pesHeaderLength << "\n";

        // Do the PES headers end in this packet ?
        if (pesHeaderLength < pesDecodeBufferLen)
        {
            VVERBOSE("FLUSH BUFFERS\n");
            flushBuffers = true;

            // For each packet, set the end point for PES headers in
            // that packet
            for (pkt_iter = packets.begin(); (pesHeaderLength > 0) &&
                 pkt_iter != packets.end(); pkt_iter++)
            {
                pPkt2 = *pkt_iter;

                if (IS_VVERBOSE())
                    std::cerr << "  scanning PES header lengths : pktId "
                              << pPkt2->packetId << "\n";

                while (!pesHdrLengths.empty())
                {
                    uint16_t pesHdrLen = pesHdrLengths.front();
                    pesHdrLen /= 8;
                    pesHdrLengths.pop_front();

                    if (IS_VVERBOSE())
                        std::cerr << "  pes hdr len : checked : "
                                  << pesHdrLen << "\n";

                    // Does this PES header fit completely within
                    // remaining packet space?

                    if (pesHdrLen + pPkt2->payloadOffset +
                        pPkt2->pesHdrOffset < TS_FRAME_SIZE)
                    {
                        if (IS_VVERBOSE())
                            std::cerr << "  PES header fits : "
                                      << pesHdrLen << "\n";

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
                        uint16_t pktBoundaryOffset = TS_FRAME_SIZE -
                            pPkt2->payloadOffset - pPkt2->pesHdrOffset;

                        if (IS_VVERBOSE())
                            std::cerr << "  pktBoundaryOffset : "
                                      << pktBoundaryOffset << " ("
                                      << TS_FRAME_SIZE << " - "
                                      << pPkt2->payloadOffset << " - "
                                      << pPkt2->pesHdrOffset << ")\n";

                        if (pktBoundaryOffset < 4)
                        {
                            pesHdrLen -= pktBoundaryOffset;
                            pesHdrLen *= 8;
                            if (IS_VVERBOSE())
                                std::cerr << "  pes hdr len : re-push : "
                                          << pesHdrLen << "\n";
                            pesHdrLengths.push_front(pesHdrLen);

                            pesHdrLen = pesHdrLengths.front();
                            if (IS_VVERBOSE())
                                std::cerr << "  pes hdr len : front now : "
                                          << pesHdrLen << "\n";
                        }
                        else if (pktBoundaryOffset == 4)
                        {
                            VVERBOSE("  pes hdr len : between startCode and "
                                     "payload\n");
                        }
                        else
                            VVERBOSE("  pes hdr len : inside payload\n" );

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
        if (IS_VVERBOSE())
            std::cerr << "Push Back : PayloadStartIndicator "
                      << pPkt->getPayloadStartIndicator()
                      << ", packets.size() " << packets.size() << "\n";
        flushBuffers = true;
        packets.push_back(pPkt);
    }

    if (true == flushBuffers)
    {
        VVERBOSE("Flush packets for write\n");

        // Loop through each buffered packet.
        // If it is encrypted, perform decryption and then write it out.
        // Otherwise, just write it out.
        for (pkt_iter = packets.begin(); pkt_iter != packets.end(); pkt_iter++)
        {
            pPkt2 = *pkt_iter;

            if (IS_VVERBOSE())
                std::cerr << "Flushing packet " << pPkt2->packetId << "\n";

            if (true == pPkt2->getScramblingControl())
            {
                pPkt2->clrScramblingControl();
                uint8_t decryptOffset = pPkt2->payloadOffset +
                    pPkt2->pesHdrOffset;
                uint8_t decryptLen = TS_FRAME_SIZE - decryptOffset;

                if (IS_VVERBOSE())
                    std::cerr << "Decrypting PktID " << pPkt2->packetId
                              << " from stream " << stream_pid
                              << " : decrypt offset " << decryptOffset
                              << " len " << decryptLen << "\n";

                if (false == decrypt(&pPkt2->buffer[decryptOffset],
                                     decryptLen))
                {
                    std::perror("Packet decrypt fails");
                    delete pPkt2;
                    packets.clear();
                    return false;
                }
            }

            if (IS_VVERBOSE())
            {
                std::cerr << "Writing PktID " << pPkt2->packetId
                          << " from stream " << stream_pid << "\n";
                pPkt2->dump();
            }

            if (pOutfile->write(&pPkt2->buffer[0], TS_FRAME_SIZE) !=
                TS_FRAME_SIZE)
            {
                std::perror("Writing packet to output file");
            }
            else
            {
                if (IS_VVERBOSE())
                    std::cerr << "Wrote PktID " << pPkt2->packetId
                              << " from stream " << stream_pid << "\n";
            }

            delete pPkt2;
        }

        packets.clear();
    }
    else
        VERBOSE("Do NOT flush packets for write\n");

    return true;
}

bool TiVoDecoderTsStream::getPesHdrLength(uint8_t *pBuffer, uint16_t bufLen)
{
    TiVoDecoder_MPEG2_Parser parser(pBuffer, bufLen);

    bool     done      = false;
    uint32_t startCode = 0;
    uint16_t len       = 0;

    while ((false == done) && (false == parser.isEndOfFile()) &&
           (bufLen > parser.getReadPos()))
    {
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
            if (IS_VVERBOSE())
                std::cerr << "  TS PES Packet   : " << startCode
                          << " : Extension header\n";
            parser.extension_header(len);
        }
        else if (GROUP_START_CODE == startCode)
        {
            if (IS_VVERBOSE())
                std::cerr << "  TS PES Packet   : " << startCode
                          << " : GOP header\n";
            parser.group_of_pictures_header(len);
        }
        else if (USER_DATA_START_CODE == startCode)
        {
            if (IS_VVERBOSE())
                std::cerr << "  TS PES Packet   : " << startCode
                          << " : User Data header\n";
            parser.user_data(len);
        }
        else if (PICTURE_START_CODE == startCode)
        {
            if (IS_VVERBOSE())
                std::cerr << "  TS PES Packet   : " << startCode
                          << " : Picture header\n";
            parser.picture_header(len);
        }
        else if (SEQUENCE_HEADER_CODE == startCode)
        {
            if (IS_VVERBOSE())
                std::cerr << "  TS PES Packet   : " << startCode
                          << " : Sequence header\n";
            parser.sequence_header(len);
        }
        else if (SEQUENCE_END_CODE == startCode)
        {
            if (IS_VVERBOSE())
                std::cerr << "  TS PES Packet   : " << startCode
                          << " : Sequence End header\n";
            parser.sequence_end(len);
        }
        else if (ANCILLARY_DATA_CODE == startCode)
        {
            if (IS_VVERBOSE())
                std::cerr << "  TS PES Packet   : " << startCode
                          << " : Ancillary Data header\n";
            parser.ancillary_data(len);
        }
        else if (startCode >= SLICE_START_CODE_MIN &&
                 startCode <= SLICE_START_CODE_MAX)
        {
            if (IS_VVERBOSE())
                std::cerr << "  TS PES Packet   : " << startCode
                          << " : Slice\n";
            done = true;
        }
        else if ((startCode == 0x1BD) ||
                 (startCode >= 0x1C0 && startCode <= 0x1EF))
        {
            if (IS_VVERBOSE())
                std::cerr << "  TS PES Packet   : " << startCode
                          << " : Audio/Video Stream\n";
            parser.pes_header(len);
        }
        else
        {
            if (IS_VERBOSE())
                std::cerr << "Unhandled PES header : " << startCode << "\n";
            return false;
        }

        if (len)
        {
            if (IS_VVERBOSE())
                std::cerr << "  TS PES Packet   : " << len
                          << " : PES Hdr Len\n";
            pesHdrLengths.push_back(len);
        }
    }

    return true;
}

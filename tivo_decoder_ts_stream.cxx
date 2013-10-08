/*
* tivodecode, (c) 2006, Jeremy Drake
* See COPYING file for license terms
*/
#ifdef HAVE_CONFIG_H
#include "tdconfig.h"
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "sha1.h"
#include "hexlib.h"
#include "md5.h"
#include "tivo_parse.hxx"
#include "tivo_decoder_ts.hxx"


TiVoDecoderTsStream::TiVoDecoderTsStream(UINT16 pid)
{
    packets.clear();

    pParent         = NULL;
    stream_pid      = pid;
    stream_type_id  = 0;
    stream_id       = 0;
    stream_type     = TS_STREAM_TYPE_NONE;

    memset(&turing_stuff, 0, sizeof(TS_Turing_Stuff) );

}

void TiVoDecoderTsStream::setDecoder(TiVoDecoderTS * pDecoder)
{
    pParent = pDecoder;
}

BOOL TiVoDecoderTsStream::addPkt( TiVoDecoderTsPacket * pPkt )
{
    TiVoDecoderTsPacket * pPkt2 = NULL;
    BOOL flushBuffers           = FALSE;
    TsPackets_it iter;

    if(!pPkt)
    {
        perror("bad parameter");
        return(FALSE);
    }

    // If this packet's Payload Unit Start Indicator is set,
    // or one of the stream's previous packet's was set, we
    // need to buffer the packet, such that we can make an
    // attempt to determine where the end of the PES headers
    // lies.   Only after we've done that, can we determine
    // the packet offset at which decryption is to occur.
    // The accounts for the situation where the PES headers
    // span more than one TS packet, and decryption is needed.

    pPkt->setStream(this);

    if( (TRUE==pPkt->getPayloadStartIndicator()) || (0!=packets.size() ) )
    {
        VERBOSE("Add PktID %d from PID 0x%04x to packet list : payloadStart %d listCount %d\n",
            pPkt->packetId, stream_pid, pPkt->getPayloadStartIndicator(), packets.size() );
        
        packets.push_back(pPkt);

        // Form one contiguous buffer containing all buffered packet payloads
        UINT16 pesDecodeBufferLen = 0;
        memset( pesDecodeBuffer, 0, 10 * TS_FRAME_SIZE);
        for(iter=packets.begin(); iter!=packets.end(); iter++)
        {
            pPkt2 = *iter;
            VERBOSE("DEQUE : PktID %d from PID 0x%04x\n", pPkt2->packetId, stream_pid );

            memcpy( &pesDecodeBuffer[pesDecodeBufferLen],
                    &pPkt2->buffer[pPkt2->payloadOffset],
                    TS_FRAME_SIZE - pPkt2->payloadOffset);
            pesDecodeBufferLen += (TS_FRAME_SIZE - pPkt2->payloadOffset);
        }
        
        if(IS_VVERBOSE)
        {
            VVERBOSE("pesDecodeBufferLen %d\n", pesDecodeBufferLen);
            hexbulk( pesDecodeBuffer, pesDecodeBufferLen );
        }

        // Scan the contiguous buffer for PES headers 
        // in order to find the end of PES headers.  
        UINT16 pesHeaderLength = 0;
        BOOL pesParse = getPesHdrLength( pesDecodeBuffer, pesDecodeBufferLen, pesHeaderLength );
        if ( FALSE == pesParse )
        {
            perror("failed to parse PES headers");
            return(FALSE);
        }
        
        VVERBOSE("pesDecodeBufferLen %d, pesHeaderLength %d\n", pesDecodeBufferLen, pesHeaderLength );

        // Do the PES headers end in this packet ?
        if( pesHeaderLength < pesDecodeBufferLen)
        {
            VERBOSE("FLUSH BUFFERS\n");
            flushBuffers = TRUE;

            // For each packet, set the end point for PES headers in that packet
            for(iter=packets.begin(); (pesHeaderLength>0) && iter!=packets.end(); iter++)
            {
                pPkt2 = *iter;
                if( (TS_FRAME_SIZE - pPkt2->payloadOffset) < pesHeaderLength )
                {
                    // Entire packet is PES headers
                    pPkt2->pesHdrOffset =  (TS_FRAME_SIZE - pPkt2->payloadOffset);
                    pesHeaderLength     -= (TS_FRAME_SIZE - pPkt2->payloadOffset);
                }
                else
                {
                    // Only first portion of packet is PES headers
                    pPkt2->pesHdrOffset = pesHeaderLength;
                    pesHeaderLength     = 0;
                }
            }
        }
        else
        {
            
        }
    }
    else
    {
        flushBuffers = TRUE;
        packets.push_back(pPkt);
    }
    
    
    if ( TRUE == flushBuffers )
    {        
        // Loop through each buffered packet.
        // If it is encrypted, perform decryption and then write it out.
        // Otherwise, just write it out.
        for(iter=packets.begin(); iter!=packets.end(); iter++)
        {
            pPkt2 = *iter;
        
            if ( TRUE == pPkt2->getScramblingControl() )
            {
                pPkt2->clrScramblingControl();
                UINT8 decryptOffset = pPkt2->payloadOffset + pPkt2->pesHdrOffset;
                UINT8 decryptLen    = TS_FRAME_SIZE - decryptOffset;

                VERBOSE("Decrypting PktID %d from stream 0x%04x : decrypt offset %d len %d\n",
                    pPkt2->packetId, stream_pid, decryptOffset, decryptLen );
                                    
                if( FALSE == decrypt( &pPkt2->buffer[decryptOffset], decryptLen ) )
                {
                    perror("Packet decrypt fails");
                    return(FALSE);
                }
            }
        
            if( writeFunc(&pPkt2->buffer[0], TS_FRAME_SIZE, pOutfile) != TS_FRAME_SIZE)
            {
                perror("Writing packet to output file");
            }
            else
            {
                VERBOSE("Wrote PktID %d from stream 0x%04x\n", pPkt2->packetId, stream_pid);
            }        
            
            delete pPkt2;
        }
        
        packets.clear();
    }

    return(TRUE);
}


BOOL TiVoDecoderTsStream::getPesHdrLength(UINT8 * pBuffer, UINT16 bufLen, UINT16 & pesLength)
{
    UINT8 * pPtr    = pBuffer;
    BOOL done       = FALSE;
    pesLength       = 0;
    char startCode[] = { 0x00, 0x00, 0x01 };
    
    while ( ( FALSE == done ) && ( (UINT16)(pPtr-pBuffer) < bufLen ) )
    {
        VVERBOSE("PES Header Offset : %d (0x%x)\n", pPtr-pBuffer, *pPtr );

        if ( (UINT16)(pPtr-pBuffer+4) > bufLen )
        {
            // PES headers run into next packet
            pPtr = pBuffer + bufLen;
            done = TRUE;
            continue;            
        }
        else if (memcmp(startCode, pPtr, 3)) 
        {
            // Invalid PES elementary start code
            done = TRUE;
            continue;
        }

        pPtr += 3;
        UINT8 streamId = *pPtr;
        pPtr++;

        VVERBOSE( "\n--- Elementary Stream : 0x%x (%d)\n", streamId, streamId );

        if ( streamId == EXTENSION_START_CODE )
        {
            unsigned int ext_hdr_len = 0;

            if ( (*pPtr & 0x10) == 0x10 )
            {
                ext_hdr_len += 6;
            }
            else if ( (*pPtr & 0x20) == 0x20 )
            {
                if ( (*pPtr & 0x01) == 0x01 )
                {
                    ext_hdr_len += 8;
                }
                else
                {
                    ext_hdr_len += 5;
                }
            }
            else if ( (*pPtr & 0x80) == 0x80 )
            {
                if ( (*(pPtr+4) & 0x40) == 0x40 )
                {
                    ext_hdr_len += 2;
                }
                ext_hdr_len += 5;
            }

            pPtr += ext_hdr_len;

//            VVERBOSE("pPtr[0-2] = %02x:%02x:%02x\n", *(pPtr+0), *(pPtr+1), *(pPtr+2) );
//
//            while ( ((UINT16)(pPtr-pBuffer)<bufLen) && (memcmp(startCode, pPtr, 3)) )
//            {
//                pPtr++;
//                VVERBOSE("pPtr[0-2] = %02x:%02x:%02x\n", *(pPtr+0), *(pPtr+1), *(pPtr+2) );
//            }

            VVVERBOSE( "%-15s : %-25.25s : %d bytes\n", "TS PES Packet",
                    "Extension header", ext_hdr_len );
        }
        else if ( streamId == GROUP_START_CODE )
        {
            VVVERBOSE("%-15s : %-25.25s : %d bytes\n", "TS PES Packet", "Group Of Pictures", 4 );
            pPtr += 4;
        }
        else if ( streamId == USER_DATA_START_CODE )
        {
            unsigned char * pPtr2 = pPtr;
            unsigned int i = 0;

            while ( 1 )
            {
                if ( memcmp(startCode, pPtr, 3) )
                {
                    break;
                }

                i++;
                pPtr2++;
            }

            VVVERBOSE( "%-15s : %-25.25s : %d bytes\n", "TS PES Packet", "User Data", i);
            pPtr += i;
        }
        else if ( streamId == PICTURE_START_CODE )
        {
            VVVERBOSE( "%-15s : %-25.25s : %d bytes\n", "TS PES Packet", "Picture", 4 );
            pPtr += 4;
        }
        else if ( streamId == SEQUENCE_HEADER_CODE )
        {
            unsigned int PES_load_intra_flag        = 0;
            unsigned int PES_load_non_intra_flag    = 0;

            VVVERBOSE( "%-15s : %-25.25s\n", "TS PES Packet", "Sequence header" );
            pPtr += 7;

            PES_load_intra_flag = *pPtr & 0x02;
            if ( PES_load_intra_flag ) pPtr += 64;

            PES_load_non_intra_flag = *pPtr & 0x01;
            if ( PES_load_non_intra_flag ) pPtr += 64;

            pPtr++;

            VVVERBOSE( "%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "PES_load_intra_flag", PES_load_intra_flag );

            VVVERBOSE( "%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "PES_load_non_intra_flag", PES_load_non_intra_flag );
        }
        else if ( ( streamId >= 0x01 && streamId <= 0xAF ) )
        {
            VVVERBOSE( "%-15s : %-25.25s\n", "TS PES Packet", "Slice" );            
        }            
        else if ( ( streamId == 0xBD ) ||
                  ( streamId >= 0xC0 && streamId <= 0xEF ) )
        {
            TS_Stream_Element   ts_elem_stream;

            ts_elem_stream.pesPktLength = portable_ntohs( pPtr );
            pPtr += 2;
            VVVERBOSE( "%-15s : %-25.25s\n", "TS PES Packet", "Extension Hdr" );

            ts_elem_stream.PES_hdr.marker_bits                  = (*pPtr & 0xc0) >> 6;
            ts_elem_stream.PES_hdr.scrambling_control           = (*pPtr & 0x30) >> 4;
            ts_elem_stream.PES_hdr.priority                     = (*pPtr & 0x08) >> 3;
            ts_elem_stream.PES_hdr.data_alignment_indicator     = (*pPtr & 0x04) >> 2;
            ts_elem_stream.PES_hdr.copyright                    = (*pPtr & 0x02) >> 1;
            ts_elem_stream.PES_hdr.original_or_copy             = (*pPtr & 0x01);
            pPtr++;
            ts_elem_stream.PES_hdr.PTS_DTS_indicator            = (*pPtr & 0xc0) >> 6;
            ts_elem_stream.PES_hdr.ESCR_flag                    = (*pPtr & 0x20) >> 5;
            ts_elem_stream.PES_hdr.ES_rate_flag                 = (*pPtr & 0x10) >> 4;
            ts_elem_stream.PES_hdr.DSM_trick_mode_flag          = (*pPtr & 0x08) >> 3;
            ts_elem_stream.PES_hdr.additional_copy_info_flag    = (*pPtr & 0x04) >> 2;
            ts_elem_stream.PES_hdr.CRC_flag                     = (*pPtr & 0x02) >> 1;
            ts_elem_stream.PES_hdr.extension_flag               = (*pPtr & 0x01);
            pPtr++;
            ts_elem_stream.PES_hdr.PES_header_length            = *pPtr;
            pPtr++;

            pPtr += ts_elem_stream.PES_hdr.PES_header_length;

            VVVERBOSE("%-15s : %-25.25s : 0x%02x (%d)(%s)\n", "TS PES Packet",
                    "stream_id", ts_elem_stream.streamId,
                    ts_elem_stream.streamId,
                    (ts_elem_stream.streamId<0xe0) ? "audio" : "video" );

            VVVERBOSE("%-15s : %-25.25s : 0x%04x (%d)\n", "TS PES Packet",
                    "pkt_length", ts_elem_stream.pesPktLength,
                    ts_elem_stream.pesPktLength );

            VVVERBOSE("%-15s : %-25.25s : 0x%x (%d)\n", "TS PES Packet",
                    "scrambling_control",
                    ts_elem_stream.PES_hdr.scrambling_control,
                    ts_elem_stream.PES_hdr.scrambling_control );

            VVVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "priority", ts_elem_stream.PES_hdr.priority );

            VVVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "data_alignment_indicator",
                    ts_elem_stream.PES_hdr.data_alignment_indicator );

            VVVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "copyright", ts_elem_stream.PES_hdr.copyright );

            VVVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "original_or_copy", ts_elem_stream.PES_hdr.original_or_copy );

           VVVERBOSE("%-15s : %-25.25s : 0x%x (%d)\n", "TS PES Packet",
                    "PTS_DTS_indicator",
                    ts_elem_stream.PES_hdr.PTS_DTS_indicator,
                    ts_elem_stream.PES_hdr.PTS_DTS_indicator );

            VVVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "ESCR_flag", ts_elem_stream.PES_hdr.ESCR_flag );

            VVVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "ES_rate_flag", ts_elem_stream.PES_hdr.ES_rate_flag );

            VVVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "DSM_trick_mode_flag", ts_elem_stream.PES_hdr.DSM_trick_mode_flag );

            VVVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "additional_copy_info_flag",
                    ts_elem_stream.PES_hdr.additional_copy_info_flag );

            VVVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "CRC_flag", ts_elem_stream.PES_hdr.CRC_flag );

            VVVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "extension_flag", ts_elem_stream.PES_hdr.extension_flag );

            VVVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet",
                    "PES_header_length", ts_elem_stream.PES_hdr.PES_header_length );
        }
        else
        {
            perror("Unhandled PES header");
            return(FALSE);
        }
    }

    pesLength = (UINT16) (pPtr - pBuffer);
    if(pesLength > bufLen)
        pesLength = bufLen;
    return(TRUE);
}

/* vi:set ai ts=4 sw=4 expandtab: */


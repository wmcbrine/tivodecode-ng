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
#include "tivo_decoder_mpeg_parser.hxx"

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

    pPkt->setStream(this);

    // If this packet's Payload Unit Start Indicator is set,
    // or one of the stream's previous packet's was set, we
    // need to buffer the packet, such that we can make an
    // attempt to determine where the end of the PES headers
    // lies.   Only after we've done that, can we determine
    // the packet offset at which decryption is to occur.
    // The accounts for the situation where the PES headers
    // span more than one TS packet, and decryption is needed.

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
            fprintf(stderr,"failed to parse PES headers : pktID %d\n", 
                pPkt->packetId);
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
        VERBOSE("Push Back : PayloadStartIndicator %d, packets.size() %d \n", 
            pPkt->getPayloadStartIndicator(), packets.size());
        flushBuffers = TRUE;
        packets.push_back(pPkt);
    }
    
    
    if ( TRUE == flushBuffers )
    {        
        VERBOSE("Flush packets for write\n");
        
        // Loop through each buffered packet.
        // If it is encrypted, perform decryption and then write it out.
        // Otherwise, just write it out.
        for(iter=packets.begin(); iter!=packets.end(); iter++)
        {
            pPkt2 = *iter;
            
            VERBOSE("Flushing packet %d\n", pPkt2->packetId );
        
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
    else
    {
        VERBOSE("Do NOT flush packets for write\n");        
    }
        

    return(TRUE);
}


BOOL TiVoDecoderTsStream::getPesHdrLength(UINT8 * pBuffer, UINT16 bufLen, UINT16 & pesLength)
{
    TiVoDecoder_MPEG2_Parser parser(pBuffer,bufLen);
    
    BOOL done        = FALSE;
    UINT8 streamId   = 0;
    UINT32 startCode = 0;
    
    while ( (FALSE==done) && (FALSE==parser.isEndOfFile()) && (bufLen > parser.getReadPos()) )
    {
        VVERBOSE("PES Header Offset : %d (0x%x)\n", parser.getReadPos(), parser.nextbits(8) );
        
        if( 0x000001 != parser.nextbits(24) )
        {
            done = TRUE;
            continue;
        }

        startCode = parser.nextbits(32);
 
        if( EXTENSION_START_CODE == startCode )
        {
            VVERBOSE( "%-15s   : 0x%08x : %-25.25s\n", 
                "TS PES Packet", startCode, "Extension header" );    
            parser.extension_header();
        }
        else if ( GROUP_START_CODE == startCode )
        {
            VVERBOSE( "%-15s   : 0x%08x : %-25.25s\n", 
                "TS PES Packet", startCode,"GOP header" );    
            parser.group_of_pictures_header();
        }
        else if ( USER_DATA_START_CODE == startCode )
        {
            VVERBOSE( "%-15s   : 0x%08x : %-25.25s\n", 
                "TS PES Packet", startCode,"User Data header" );    
            parser.user_data();
        }
        else if ( PICTURE_START_CODE == startCode )
        {
            VVERBOSE( "%-15s   : 0x%08x : %-25.25s\n", 
                "TS PES Packet", startCode,"Picture header" );    
            parser.picture_header();
        }
        else if ( SEQUENCE_HEADER_CODE == startCode )
        {
            VVERBOSE( "%-15s   : 0x%08x : %-25.25s\n", 
                "TS PES Packet", startCode,"Sequence header" );    
            parser.sequence_header();
        }
        else if ( SEQUENCE_END_CODE == startCode )
        {
            VVERBOSE( "%-15s   : 0x%08x : %-25.25s\n", 
                "TS PES Packet", startCode,"Sequence End header" );    
            parser.sequence_end();
        }
        else if ( ( startCode >= 0x101 && startCode <= 0x1AF ) )
        {
            VVERBOSE( "%-15s   : 0x%08x : %-25.25s\n", 
                "TS PES Packet", startCode,"Slice" );    
//            parser.slice();
            done = TRUE;
        }
        else if ( ( startCode == 0x1BD ) || ( startCode >= 0x1C0 && startCode <= 0x1EF ) )
        {
            VVERBOSE( "%-15s   : 0x%08x : %-25.25s\n", 
                "TS PES Packet", startCode,"Audio/Video Stream" );
            parser.pes_header();
        }
        else
        {
            VERBOSE("Unhandled PES header : 0x%08x\n", startCode );
            return(FALSE);
        }
    }

    pesLength = parser.getReadPos();
    if(pesLength > bufLen)
        pesLength = bufLen;
        
    return(TRUE);
}

/* vi:set ai ts=4 sw=4 expandtab: */


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

#ifdef WIN32
#include <windows.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <cstring>

#include "hexlib.hxx"
#include "tivo_parse.hxx"
#include "tivo_decoder_ts.hxx"

TsPktDump pktDumpMap;

ts_packet_tag_info ts_packet_tags[] = {
    {0x0000, 0x0000, TS_PID_TYPE_PROGRAM_ASSOCIATION_TABLE},
    {0x0001, 0x0001, TS_PID_TYPE_CONDITIONAL_ACCESS_TABLE},
    {0x0002, 0x000F, TS_PID_TYPE_RESERVED},
    {0x0010, 0x0010, TS_PID_TYPE_NETWORK_INFORMATION_TABLE},
    {0x0011, 0x0011, TS_PID_TYPE_SERVICE_DESCRIPTION_TABLE},
    {0x0012, 0x0012, TS_PID_TYPE_EVENT_INFORMATION_TABLE},
    {0x0013, 0x0013, TS_PID_TYPE_RUNNING_STATUS_TABLE},
    {0x0014, 0x0014, TS_PID_TYPE_TIME_DATE_TABLE},
    {0x0015, 0x001F, TS_PID_TYPE_RESERVED},
    {0x0020, 0x1FFE, TS_PID_TYPE_AUDIO_VIDEO_PRIVATE_DATA},
    {0xFFFF, 0xFFFF, TS_PID_TYPE_NONE}
};


ts_pmt_stream_type_info ts_pmt_stream_tags[] = {
    // video
    { 0x01, 0x01, TS_STREAM_TYPE_VIDEO},        // MPEG1Video
    { 0x02, 0x02, TS_STREAM_TYPE_VIDEO},        // MPEG2Video
    { 0x10, 0x10, TS_STREAM_TYPE_VIDEO},        // MPEG4Video
    { 0x1b, 0x1b, TS_STREAM_TYPE_VIDEO},        // H264Video
    { 0x80, 0x80, TS_STREAM_TYPE_VIDEO},        // OpenCableVideo
    { 0xea, 0xea, TS_STREAM_TYPE_VIDEO},        // VC1Video

    // audio
    { 0x03, 0x03, TS_STREAM_TYPE_AUDIO},        // MPEG1Audio
    { 0x04, 0x04, TS_STREAM_TYPE_AUDIO},        // MPEG2Audio
    { 0x11, 0x11, TS_STREAM_TYPE_AUDIO},        // MPEG2AudioAmd1
    { 0x0f, 0x0f, TS_STREAM_TYPE_AUDIO},        // AACAudio
    { 0x81, 0x81, TS_STREAM_TYPE_AUDIO},        // AC3Audio
    { 0x8a, 0x8a, TS_STREAM_TYPE_AUDIO},        // DTSAudio

    // DSM-CC Object Carousel
    { 0x08, 0x08, TS_STREAM_TYPE_OTHER},        // DSMCC
    { 0x0a, 0x0a, TS_STREAM_TYPE_OTHER},        // DSMCC_A
    { 0x0b, 0x0b, TS_STREAM_TYPE_OTHER},        // DSMCC_B
    { 0x0c, 0x0c, TS_STREAM_TYPE_OTHER},        // DSMCC_C
    { 0x0d, 0x0d, TS_STREAM_TYPE_OTHER},        // DSMCC_D
    { 0x14, 0x14, TS_STREAM_TYPE_OTHER},        // DSMCC_DL
    { 0x15, 0x15, TS_STREAM_TYPE_OTHER},        // MetaDataPES
    { 0x16, 0x16, TS_STREAM_TYPE_OTHER},        // MetaDataSec
    { 0x17, 0x17, TS_STREAM_TYPE_OTHER},        // MetaDataDC
    { 0x18, 0x18, TS_STREAM_TYPE_OTHER},        // MetaDataOC
    { 0x19, 0x19, TS_STREAM_TYPE_OTHER},        // MetaDataDL

    // other
    { 0x05, 0x05, TS_STREAM_TYPE_OTHER},        // PrivSec
    { 0x06, 0x06, TS_STREAM_TYPE_OTHER},        // PrivData
    { 0x07, 0x07, TS_STREAM_TYPE_OTHER},        // MHEG
    { 0x09, 0x09, TS_STREAM_TYPE_OTHER},        // H222_1
    { 0x0e, 0x0e, TS_STREAM_TYPE_OTHER},        // MPEG2Aux
    { 0x12, 0x12, TS_STREAM_TYPE_OTHER},        // FlexMuxPES
    { 0x13, 0x13, TS_STREAM_TYPE_OTHER},        // FlexMuxSec
    { 0x1a, 0x1a, TS_STREAM_TYPE_OTHER},        // MPEG2IPMP
    { 0x7f, 0x7f, TS_STREAM_TYPE_OTHER},        // MPEG2IPMP2

    { 0x97, 0x97, TS_STREAM_TYPE_PRIVATE_DATA}, // TiVo Private Data

    { 0x00, 0x00, TS_STREAM_TYPE_NONE}
};


TiVoDecoderTS::TiVoDecoderTS(
        TuringState *pTuringState,
        HappyFile *pInfile,
        FILE *pOutfile) :
    TiVoDecoder(pTuringState,
        pInfile,
        pOutfile)
{
    pktCounter = 0;
    streams.clear();
    std::memset(&patData, 0, sizeof(TS_PAT_data));

    // Create stream for PAT
    VERBOSE("Creating new stream for PID (0x%04x)\n", 0 );
    TiVoDecoderTsStream * pStream = new TiVoDecoderTsStream(0);
    pStream->pOutfile   = pFileOut;
    pStream->setDecoder(this);
    streams[0]          = pStream;
}


TiVoDecoderTS::~TiVoDecoderTS()
{
    streams.clear();
}


int TiVoDecoderTS::handlePkt_PAT( TiVoDecoderTsPacket * pPkt )
{
    unsigned short pat_field            = 0;
    unsigned short section_length       = 0;
    unsigned short transport_stream_id  = 0;
    unsigned char * pPtr                = NULL;

    if ( !pPkt )
    {
        perror("Invalid handlePkt_PAT argument");
        return -1;
    }

    pPtr = &pPkt->buffer[pPkt->payloadOffset];

    if ( pPkt->getPayloadStartIndicator() )
    {
        pPtr++; // advance past pointer field
    }

    if ( *pPtr != 0x00 )
    {
        perror("PAT Table ID must be 0x00");
        return -1;
    }
    else
    {
        pPtr++;
    }

    pat_field = portable_ntohs( pPtr );
    section_length = pat_field & 0x03ff;
    pPtr += 2;

    if ( (pat_field & 0xC000) != 0x8000 )
    {
        perror("Failed to validate PAT Misc field");
        return -1;
    }

    if ( (pat_field & 0x0C00) != 0x0000 )
    {
        perror("Failed to validate PAT MBZ of section length");
        return -1;
    }

    transport_stream_id = portable_ntohs( pPtr );
    pPtr += 2;
    section_length -= 2;

    if ( (*pPtr & 0x3E) != patData.version_number )
    {
        patData.version_number = *pPtr & 0x3E;
        VERBOSE( "%-15s : version changed : %d\n", "TS ProgAssocTbl",
                patData.version_number );
    }

    pPtr++;
    section_length--;

    patData.section_number = *pPtr++;
    section_length--;

    patData.last_section_number = *pPtr++;
    section_length--;

    section_length -= 4; // ignore the CRC for now

    while ( section_length > 0 )
    {
        pat_field = portable_ntohs( pPtr );
        VERBOSE( "%-15s : Program Num : %d\n", "TS ProgAssocTbl", pat_field );
        pPtr += 2;
        section_length -= 2;

        pat_field = portable_ntohs( pPtr );

        patData.program_map_pid = pat_field & 0x1FFF;
        VERBOSE( "%-15s : Program PID : 0x%x (%d)\n", "TS ProgAssocTbl",
                patData.program_map_pid, patData.program_map_pid );
                
        // locate previous stream definition
        // if lookup fails, create a new stream
        TsStreams_it stream_iter = streams.find( patData.program_map_pid );
        if( stream_iter == streams.end() )
        {
            VERBOSE("Creating new stream for PMT PID 0x%04x\n", 
                patData.program_map_pid );
                
            TiVoDecoderTsStream * pStream       = new TiVoDecoderTsStream(patData.program_map_pid);
            pStream->pOutfile                   = pFileOut;
            pStream->setDecoder(this);
            streams[patData.program_map_pid]    = pStream;
        }
        else
        {
            VERBOSE("Re-use existing stream for PMT PID 0x%04x\n", 
                patData.program_map_pid );
        }                
                
        pPtr += 2;
        section_length -= 2;
    }

    return 0;
}

int TiVoDecoderTS::handlePkt_PMT( TiVoDecoderTsPacket * pPkt )
{
    unsigned short section_length   = 0;
    unsigned short pmt_field        = 0;
    unsigned short i                = 0;
    unsigned char * pPtr            = NULL;

    if ( !pPkt )
    {
        perror("Invalid handlePkt_PMT argument");
        return -1;
    }

    pPtr = &pPkt->buffer[pPkt->payloadOffset];

    if ( pPkt->getPayloadStartIndicator() )
    {
        pPtr++; // advance past pointer field
    }

    // advance past table_id field
    pPtr++;

    pmt_field = portable_ntohs( pPtr );
    section_length = pmt_field & 0x0fff;

    // advance past section_length
    pPtr += 2;

    // advance past program/section/next numbers
    pPtr += 9;
    section_length -= 9;

    // ignore the CRC for now
    section_length -= 4;

    for ( i=0; section_length > 0; i++ )
    {
        unsigned short es_info_length = 0;
        char strTypeStr[25];

        UINT16 streamPid           = 0;
        UINT8 streamTypeId         = *pPtr;
        ts_stream_types streamType = TS_STREAM_TYPE_PRIVATE_DATA;

        for (int j = 0; ts_pmt_stream_tags[j].ts_stream_type != TS_STREAM_TYPE_NONE; j++)
        {
            if ( ( streamTypeId >= ts_pmt_stream_tags[j].code_match_lo )  &&
                 ( streamTypeId <= ts_pmt_stream_tags[j].code_match_hi ) )
            {
                streamType = ts_pmt_stream_tags[j].ts_stream_type;
                break;
            }
        }

        switch( streamType )
        {
            case TS_STREAM_TYPE_PRIVATE_DATA : sprintf(strTypeStr,"PrivateData"); break;
            case TS_STREAM_TYPE_AUDIO        : sprintf(strTypeStr,"Audio"); break;
            case TS_STREAM_TYPE_VIDEO        : sprintf(strTypeStr,"Video"); break;
            case TS_STREAM_TYPE_OTHER        : sprintf(strTypeStr,"Other"); break;
            default                          : sprintf(strTypeStr,"Unknown"); break;
        }

        // advance past stream_type field
        pPtr++;
        section_length--;

        pmt_field = portable_ntohs( pPtr );
        streamPid = pmt_field & 0x1fff;

        // advance past elementary field
        pPtr += 2;
        section_length -= 2;

        pmt_field      = portable_ntohs( pPtr );
        es_info_length = pmt_field & 0x1fff;

        // advance past ES info length field
        pPtr += 2;
        section_length -= 2;

        // advance past es info
        pPtr += es_info_length;
        section_length -= es_info_length;

        VERBOSE("%-15s : StreamId 0x%02x (%d), PID 0x%04x, Type 0x%02x (%d : %s)\n", "TS ProgMapTbl",
                streamTypeId, streamTypeId, streamPid,
                streamType, streamType, strTypeStr );

        // locate previous stream definition
        // if lookup fails, create a new stream
        TsStreams_it stream_iter = streams.find( streamPid );
        if( stream_iter == streams.end() )
        {
            VERBOSE("Creating new stream for PID 0x%04x\n", streamPid );
            TiVoDecoderTsStream * pStream = new TiVoDecoderTsStream(streamPid);
            pStream->stream_type_id = streamTypeId;
            pStream->stream_type    = streamType;
            pStream->pOutfile       = pFileOut;
            pStream->setDecoder(this);

            streams[streamPid]      = pStream;
        }
        else
        {
            VERBOSE("Re-use existing stream for PID 0x%04x\n", streamPid );
        }
    }

    return 0;
}

int TiVoDecoderTS::handlePkt_TiVo( TiVoDecoderTsPacket * pPkt )
{

    unsigned char * pPtr    = NULL;
    int stream_loop         = 0;
    unsigned int validator      = 0;
    unsigned short pid          = 0;
    unsigned char  stream_id    = 0;
    unsigned short stream_bytes = 0;
    unsigned int foundit        = 0;

    if ( !pPkt )
    {
        perror("Invalid handlePkt_TiVo argument");
        return -1;
    }

    pPtr = &pPkt->buffer[pPkt->payloadOffset];

    // TiVo Private Data format
    // ------------------------
    //   4 bytes   : validator -- "TiVo"
    //   4 bytes   : Unknown -- always 0x81 0x3 0x7d 0x0 -- ???
    //   2 bytes   : number of elementary stream bytes following
    //   For each elementary stream :
    //     2 byte  : packet id
    //     1 byte  : stream id
    //     1 byte  : Unknown -- always 0x10 -- reserved ???
    //    16 bytes : Turing key

    VERBOSE("\n");

    validator = portable_ntohl( pPtr );
    if ( validator != 0x5469566f )
    {
        perror("Invalid TiVo private data validator");
        return -1;
    }

    VERBOSE("%-15s : %-25.25s : 0x%08x (%c%c%c%c)\n",   "TiVo Private",
            "Validator", validator, *pPtr, *(pPtr+1), *(pPtr+2), *(pPtr+3) );

    VERBOSE("%-15s : %-25.25s : 0x%x 0x%x 0x%x 0x%x\n", "TiVo Private",
            "Unknown", *(pPtr+4), *(pPtr+5), *(pPtr+6), *(pPtr+7) );

    pPtr += 4;  // advance past "TiVo"

    pPtr += 2;  // advance past validator field
    pPtr += 2;  // advance past unknown1 field
    pPtr += 1;  // advance past unknown2 field

    stream_bytes = *pPtr;
    pPtr += 1;  // advance past unknown3 field

    VERBOSE("%-15s : %-25.25s : %d\n", "TiVo Private",
            "Stream Bytes", stream_bytes );

    while ( stream_bytes > 0 )
    {
        pid = portable_ntohs( pPtr );
        stream_bytes -= 2;
        pPtr += 2;  // advance past pid

        stream_id = *pPtr;
        stream_bytes--;
        pPtr++;     // advance past stream_id;

        stream_bytes--;
        pPtr++;     // advance past reserved???

        // locate previous stream definition
        // if lookup fails, create a new stream
        TsStreams_it stream_iter = streams.find( pid );
        if( stream_iter == streams.end() )
        {
            VERBOSE("TiVo private data : No such PID 0x%04x\n", pid );
            return -1;
        }
        else
        {
            VERBOSE("TiVo private data : matched PID 0x%04x\n", pid );
            TiVoDecoderTsStream * pStream = stream_iter->second;

            pStream->stream_id = stream_id;

            if (std::memcmp(&pStream->turing_stuff.key[0], pPtr, 16))
            {
                VERBOSE( "\nUpdating PID 0x%04x Type 0x%02x Turing Key\n", pid, stream_id );
                if( IS_VERBOSE )
                    hexbulk( &pStream->turing_stuff.key[0], 16 );
                if( IS_VERBOSE )
                    hexbulk( pPtr, 16 );
                std::memcpy(&pStream->turing_stuff.key[0], pPtr, 16);
            }

            VERBOSE("%-15s : %-25.25s : %d\n", "TiVo Private", "Block No",
                    pStream->turing_stuff.block_no );
            VERBOSE("%-15s : %-25.25s : 0x%08x\n", "TiVo Private", "Crypted",
                    pStream->turing_stuff.crypted );
            VERBOSE("%-15s : %-25.25s : 0x%04x (%d)\n", "TiVo Private", "PID",
                    pStream->stream_pid,
                    pStream->stream_pid );
            VERBOSE("%-15s : %-25.25s : 0x%02x (%d)\n", "TiVo Private", "Stream ID",
                    pStream->stream_id,
                    pStream->stream_id );
                    
            VERBOSE("%-15s : %-25.25s : \n", "TiVo Private", "Turing Key" );
            if( IS_VERBOSE )
                hexbulk( &pStream->turing_stuff.key[0], 16 );
        }

        pPtr += 16;
        stream_bytes -= 16;
    }

    return 0;
}

int TiVoDecoderTS::handlePkt_AudioVideo( TiVoDecoderTsPacket * pPkt )
{
    return 0;
}


BOOL TiVoDecoderTS::process()
{
    int err         = 0;
    UINT16 pid      = 0;
    hoff_t position = 0;
    TiVoDecoderTsStream * pStream = NULL;
    TsStreams_it        stream_iter;
    TsPktDump_iter      pktDump_iter;

    if(FALSE==isValid)
    {
        VERBOSE("TS Process : not valid\n");
        return FALSE;
    }

    while( running )
    {
        err      = 0;
        position = pFileIn->tell();
        pid      = 0;

        pktCounter++;
        VERBOSE( "Packet : %d\n", pktCounter);

        TiVoDecoderTsPacket * pPkt = new TiVoDecoderTsPacket;
        if(!pPkt)
        {
            perror("failed to allocate TS packet");
            return 10;
        }
        
        pPkt->packetId = pktCounter;

        int readSize = pPkt->read(pFileIn);
        
        VVERBOSE("Read Size : %d\n", readSize );
        
        if( readSize < 0 )
        {
            fprintf(stderr,"Error TS packet read : pkt %d : size read %d", pktCounter, readSize);
            return 10;
        }
        else if( readSize == 0 )
        {
            VERBOSE( "End of File\n");
            running = FALSE;
            continue;
        }
        else if(TS_FRAME_SIZE != readSize )
        {
            fprintf(stderr,"Error TS packet read : pkt %d : size read %d", pktCounter, readSize);
            return 10;
        }
        
        pktDump_iter = pktDumpMap.find( pktCounter );
        o_pkt_dump   = (pktDump_iter != pktDumpMap.end()) ? TRUE : FALSE;

        if( FALSE==pPkt->decode() )
        {
            fprintf(stderr,"packet decode fails : pktId %d\n", pktCounter);
            return 10;
        }
        
        if(IS_VERBOSE)
        {
            VERBOSE("=============== Packet : %d ===============\n", pPkt->packetId );
            pPkt->dump();
        }

        pid = pPkt->getPID();

        switch ( pPkt->ts_packet_type )
        {
            case TS_PID_TYPE_PROGRAM_ASSOCIATION_TABLE :
            {
                err = handlePkt_PAT( pPkt );
                if ( err )
                    perror("ts_handle_pat failed");
                break;
            }
            case TS_PID_TYPE_AUDIO_VIDEO_PRIVATE_DATA :
            {
                if(pPkt->getPID() == patData.program_map_pid)
                {
                    pPkt->setPmtPkt( TRUE );
                    err = handlePkt_PMT( pPkt );
                    if ( err )
                        perror("ts_handle_pmt failed");
                }
                else
                {
                    for( stream_iter=streams.begin(); stream_iter!=streams.end(); stream_iter++ )
                    {
                        pStream = stream_iter->second;
                        if( (pPkt->getPID()==pStream->stream_pid) && 
                            (TS_STREAM_TYPE_PRIVATE_DATA==pStream->stream_type ) )
                        {
                            pPkt->setTiVoPkt(TRUE);
                            break;
                        }
                    }

                    if ( TRUE == pPkt->isTiVoPkt() )
                    {
                        err = handlePkt_TiVo( pPkt );
                        if ( err )
                            perror("handlePkt_Tivo failed");
                    }
                    else
                    {
                        err = handlePkt_AudioVideo( pPkt );
                        if ( err )
                            perror("handlePkt_AudoVideo failed");
                    }
                }
                break;
            }
            default :
            {
                perror( "Unknown Packet Type" );
                return 10;
            }
        }

        stream_iter = streams.find(pPkt->getPID());
        if(stream_iter == streams.end())
        {
            perror("Can not locate packet stream by PID");
        }
        else
        {
            VVERBOSE("Adding packet %d to PID 0x%x (%d)\n",
                pktCounter, pPkt->getPID(), pPkt->getPID() );

            pStream = stream_iter->second;
            if(FALSE == pStream->addPkt( pPkt ) )
            {
                fprintf(stderr,"Failed to add packet to stream : pktId %d\n", 
                    pPkt->packetId);
            }
            else
            {
                VVERBOSE("Added packet %d to PID 0x%x (%d)\n",
                    pktCounter, pPkt->getPID(), pPkt->getPID() );
            }
        }
    }

    return TRUE;
}

/* vi:set ai ts=4 sw=4 expandtab: */


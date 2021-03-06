/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING.md for license terms
 */

#include <algorithm>
#include <iostream>

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
        TuringState &pTuringState,
        HappyFile &pInfile,
        HappyFile &pOutfile) :
    TiVoDecoder(pTuringState,
        pInfile,
        pOutfile)
{
    pktCounter = 0;
    streams.clear();

    // Create stream for PAT
    VERBOSE("Creating new stream for PID 0\n");
    streams[0] = new TiVoDecoderTsStream(pFileOut, this, 0);
}

TiVoDecoderTS::~TiVoDecoderTS()
{
    streams.clear();
}

int TiVoDecoderTS::handlePkt_PAT(TiVoDecoderTsPacket *pPkt)
{
    uint16_t pat_field           = 0;
    uint16_t section_length      = 0;
    uint8_t *pPtr                = NULL;

    if (!pPkt)
    {
        std::perror("Invalid handlePkt_PAT argument");
        return -1;
    }

    pPtr = &pPkt->buffer[pPkt->payloadOffset];

    if (pPkt->getPayloadStartIndicator())
        pPtr++; // advance past pointer field

    if (*pPtr != 0x00)
    {
        std::perror("PAT Table ID must be 0x00");
        return -1;
    }
    else
        pPtr++;

    pat_field = GET16(pPtr);
    section_length = pat_field & 0x03ff;
    pPtr += 2;

    if ((pat_field & 0xC000) != 0x8000)
    {
        std::perror("Failed to validate PAT Misc field");
        return -1;
    }

    if ((pat_field & 0x0C00) != 0x0000)
    {
        std::perror("Failed to validate PAT MBZ of section length");
        return -1;
    }

    pPtr += 2;
    section_length -= 2;

    if ((*pPtr & 0x3E) != patData.version_number)
    {
        patData.version_number = *pPtr & 0x3E;
        if (IS_VERBOSE())
            std::cerr << "TS ProgAssocTbl : version changed : "
                      << patData.version_number << "\n";
    }

    pPtr++;
    section_length--;

    patData.section_number = *pPtr++;
    section_length--;

    patData.last_section_number = *pPtr++;
    section_length--;

    section_length -= 4; // ignore the CRC for now

    while (section_length > 0)
    {
        pat_field = GET16(pPtr);
        if (IS_VERBOSE())
            std::cerr << "TS ProgAssocTbl : Program Num : "
                      << pat_field << "\n";
        pPtr += 2;
        section_length -= 2;

        pat_field = GET16(pPtr);

        patData.program_map_pid = pat_field & 0x1FFF;
        if (IS_VERBOSE())
            std::cerr << "TS ProgAssocTbl : Program PID : "
                      << patData.program_map_pid << "\n";

        // locate previous stream definition
        // if lookup fails, create a new stream
        TsStreams_it stream_iter = streams.find(patData.program_map_pid);
        if (stream_iter == streams.end())
        {
            if (IS_VERBOSE())
                std::cerr << "Creating new stream for PMT PID "
                          << patData.program_map_pid << "\n";

            streams[patData.program_map_pid] =
                new TiVoDecoderTsStream(pFileOut, this,
                                        patData.program_map_pid);
        }
        else
        {
            if (IS_VERBOSE())
                std::cerr << "Re-use existing stream for PMT PID "
                          << patData.program_map_pid << "\n";
        }

        pPtr += 2;
        section_length -= 2;
    }

    return 0;
}

int TiVoDecoderTS::handlePkt_PMT(TiVoDecoderTsPacket *pPkt)
{
    uint16_t section_length = 0;
    uint16_t pmt_field      = 0;
    uint16_t i              = 0;
    uint8_t *pPtr           = NULL;

    if (!pPkt)
    {
        std::perror("Invalid handlePkt_PMT argument");
        return -1;
    }

    pPtr = &pPkt->buffer[pPkt->payloadOffset];

    if (pPkt->getPayloadStartIndicator())
        pPtr++; // advance past pointer field

    // advance past table_id field
    pPtr++;

    pmt_field = GET16(pPtr);
    section_length = pmt_field & 0x0fff;

    // advance past section_length
    pPtr += 2;

    // advance past program/section/next numbers
    pPtr += 9;
    section_length -= 9;

    // ignore the CRC for now
    section_length -= 4;

    for (i = 0; section_length > 0; i++)
    {
        uint16_t es_info_length = 0;
        const char *strTypeStr;

        uint16_t streamPid           = 0;
        uint8_t streamTypeId         = *pPtr;
        ts_stream_types streamType = TS_STREAM_TYPE_PRIVATE_DATA;

        for (int j = 0; ts_pmt_stream_tags[j].ts_stream_type !=
             TS_STREAM_TYPE_NONE; j++)
        {
            if ((streamTypeId >= ts_pmt_stream_tags[j].code_match_lo) &&
                (streamTypeId <= ts_pmt_stream_tags[j].code_match_hi))
            {
                streamType = ts_pmt_stream_tags[j].ts_stream_type;
                break;
            }
        }

        switch (streamType)
        {
            case TS_STREAM_TYPE_PRIVATE_DATA:
                strTypeStr = "PrivateData";
                break;
            case TS_STREAM_TYPE_AUDIO:
                strTypeStr = "Audio";
                break;
            case TS_STREAM_TYPE_VIDEO:
                strTypeStr = "Video";
                break;
            case TS_STREAM_TYPE_OTHER:
                strTypeStr = "Other";
                break;
            default:
                strTypeStr = "Unknown";
        }

        // advance past stream_type field
        pPtr++;
        section_length--;

        pmt_field = GET16(pPtr);
        streamPid = pmt_field & 0x1fff;

        // advance past elementary field
        pPtr += 2;
        section_length -= 2;

        pmt_field      = GET16(pPtr);
        es_info_length = pmt_field & 0x0fff;

        // advance past ES info length field
        pPtr += 2;
        section_length -= 2;

        // advance past es info
        pPtr += es_info_length;
        section_length -= es_info_length;

        if (IS_VERBOSE())
            std::cerr << "  TS ProgMapTbl : StreamId " << streamTypeId
                      << ", PID " << streamPid << ", Type " << streamType
                      << " : " << strTypeStr << "\n";

        // locate previous stream definition
        // if lookup fails, create a new stream
        TsStreams_it stream_iter = streams.find(streamPid);
        if (stream_iter == streams.end())
        {
            if (IS_VERBOSE())
                std::cerr << "Creating new stream for PID "
                          << streamPid << "\n";
            streams[streamPid] = new TiVoDecoderTsStream(pFileOut,
                this, streamPid);
            streams[streamPid]->stream_type_id = streamTypeId;
            streams[streamPid]->stream_type    = streamType;
        }
        else
        {
            if (IS_VERBOSE())
                std::cerr << "Re-use existing stream for PID "
                          << streamPid << "\n";
        }
    }

    return 0;
}

int TiVoDecoderTS::handlePkt_TiVo(TiVoDecoderTsPacket *pPkt)
{
    uint8_t *pPtr          = NULL;
    unsigned int validator = 0;
    uint16_t pid           = 0;
    uint8_t  stream_id     = 0;
    uint16_t stream_bytes  = 0;

    if (!pPkt)
    {
        std::perror("Invalid handlePkt_TiVo argument");
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

    validator = GET32(pPtr);
    if (validator != 0x5469566f)
    {
        std::perror("Invalid TiVo private data validator");
        return -1;
    }

    if (IS_VERBOSE())
    {
        std::cerr << "   TiVo Private : Validator : " << validator
                  << "(" << *pPtr << *(pPtr + 1)
                  << *(pPtr + 2) << *(pPtr + 3) << ")\n"
                  << "   TiVo Private :   Unknown : "
                  << *(pPtr + 4) << *(pPtr + 5)
                  << *(pPtr + 6) << *(pPtr + 7) << "\n";
    }

    pPtr += 4;  // advance past "TiVo"

    pPtr += 2;  // advance past validator field
    pPtr += 2;  // advance past unknown1 field
    pPtr += 1;  // advance past unknown2 field

    stream_bytes = *pPtr;
    pPtr += 1;  // advance past unknown3 field

    if (IS_VERBOSE())
        std::cerr << "   TiVo Private : Stream Bytes : "
                  << stream_bytes << "\n";

    while (stream_bytes > 0)
    {
        pid = GET16(pPtr);
        stream_bytes -= 2;
        pPtr += 2;  // advance past pid

        stream_id = *pPtr;
        stream_bytes--;
        pPtr++;     // advance past stream_id;

        stream_bytes--;
        pPtr++;     // advance past reserved???

        // locate previous stream definition
        // if lookup fails, create a new stream
        TsStreams_it stream_iter = streams.find(pid);
        if (stream_iter == streams.end())
        {
            if (IS_VERBOSE())
                std::cerr << "TiVo private data : No such PID "
                          << pid << "\n";
            return -1;
        }
        else
        {
            if (IS_VERBOSE())
                std::cerr << "TiVo private data : matched PID "
                          << pid << "\n";
            TiVoDecoderTsStream *pStream = stream_iter->second;

            pStream->stream_id = stream_id;

            if (!std::equal(pPtr, pPtr + 16, pStream->turing_stuff.key))
            {
                if (IS_VERBOSE())
                {
                    std::cerr << "\nUpdating PID " << pid
                              << " Type " << stream_id << " Turing Key\n";
                    hexbulk(pStream->turing_stuff.key, 16);
                    hexbulk(pPtr, 16);
                }
                std::copy(pPtr, pPtr + 16, pStream->turing_stuff.key);
            }

            if (IS_VERBOSE())
            {
                std::cerr << "   TiVo Private :   Block No : "
                          << pStream->turing_stuff.block_no
                          << "\n   TiVo Private :    Crypted : "
                          << pStream->turing_stuff.crypted
                          << "\n   TiVo Private :        PID : "
                          << pStream->stream_pid
                          << "\n   TiVo Private :  Stream ID : "
                          << pStream->stream_id
                          << "\n   TiVo Private : Turing Key :\n";
                hexbulk(pStream->turing_stuff.key, 16);
            }
        }

        pPtr += 16;
        stream_bytes -= 16;
    }

    return 0;
}

bool TiVoDecoderTS::process()
{
    int err = 0;
    TiVoDecoderTsStream *pStream = NULL;
    TsStreams_it stream_iter;
    TsPktDump_iter pktDump_iter;

    if (false == isValid)
    {
        VERBOSE("TS Process : not valid\n");
        return false;
    }

    while (running)
    {
        err = 0;

        pktCounter++;
        if (IS_VVERBOSE())
            std::cerr << "Packet : " << pktCounter << "\n";

        TiVoDecoderTsPacket *pPkt = new TiVoDecoderTsPacket;
        pPkt->packetId = pktCounter;

        int readSize = pPkt->read(pFileIn);

        if (IS_VVERBOSE())
            std::cerr << "Read Size : " << readSize << "\n";

        if (readSize < 0)
        {
            std::cerr << "Error TS packet read : pkt " << pktCounter
                      << " : size read " << readSize;
            return false;
        }
        else if (readSize == 0)
        {
            VERBOSE("End of File\n");
            running = false;
            continue;
        }
        else if (TS_FRAME_SIZE != readSize)
        {
            std::cerr << "Error TS packet read : pkt " << pktCounter
                      << " : size read " << readSize;
            return false;
        }

        pktDump_iter = pktDumpMap.find(pktCounter);
        o_pkt_dump   = (pktDump_iter != pktDumpMap.end()) ? true : false;

        if (false == pPkt->decode())
        {
            std::cerr << "packet decode fails : pktId "
                      << pktCounter << "\n";
            return false;
        }

        if (IS_VVERBOSE())
        {
            std::cerr << "=============== Packet : "
                      << pPkt->packetId
                      << " ===============\n";
            pPkt->dump();
        }

        switch (pPkt->ts_packet_type)
        {
            case TS_PID_TYPE_PROGRAM_ASSOCIATION_TABLE:
            {
                err = handlePkt_PAT(pPkt);
                if (err)
                    std::perror("ts_handle_pat failed");
                break;
            }
            case TS_PID_TYPE_AUDIO_VIDEO_PRIVATE_DATA:
            {
                if (pPkt->getPID() == patData.program_map_pid)
                {
                    pPkt->setPmtPkt(true);
                    err = handlePkt_PMT(pPkt);
                    if (err)
                        std::perror("ts_handle_pmt failed");
                }
                else
                {
                    for (stream_iter = streams.begin();
                         stream_iter != streams.end(); stream_iter++)
                    {
                        pStream = stream_iter->second;
                        if ((pPkt->getPID() == pStream->stream_pid) &&
                            (TS_STREAM_TYPE_PRIVATE_DATA ==
                             pStream->stream_type))
                        {
                            pPkt->setTiVoPkt(true);
                            break;
                        }
                    }

                    if (true == pPkt->isTiVoPkt())
                    {
                        err = handlePkt_TiVo(pPkt);
                        if (err)
                            std::perror("handlePkt_Tivo failed");
                    }
                }
                break;
            }
            default:
            {
                std::perror("Unknown Packet Type");
                return false;
            }
        }

        stream_iter = streams.find(pPkt->getPID());
        if (stream_iter == streams.end())
            std::perror("Can not locate packet stream by PID");
        else
        {
            if (IS_VVERBOSE())
                std::cerr << "Adding packet " << pktCounter
                          << " to PID " << pPkt->getPID() << "\n";

            pStream = stream_iter->second;
            if (false == pStream->addPkt(pPkt))
            {
                std::cerr << "Failed to add packet to stream : pktId "
                          << pPkt->packetId << "\n";
            }
            else
            {
                if (IS_VVERBOSE())
                    std::cerr << "Added packet " << pktCounter
                              << " to PID " << pPkt->getPID() << "\n";
            }
        }
    }

    return true;
}

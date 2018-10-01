/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#include <cstring>
#include <cinttypes>
#include <algorithm>
#include <iostream>

#include "hexlib.hxx"
#include "tivo_parse.hxx"
#include "tivo_decoder_ts.hxx"

int TiVoDecoderTsPacket::globalBufferLen = 0;
uint8_t TiVoDecoderTsPacket::globalBuffer[];

TiVoDecoderTsPacket::TiVoDecoderTsPacket()
{
    pParent         = NULL;
    isValid         = false;
    isPmt           = false;
    isTiVo          = false;
    payloadOffset   = 0;
    pesHdrOffset    = 0;
    ts_packet_type  = TS_PID_TYPE_NONE;
    packetId        = 0;

    std::memset(buffer, 0, TS_FRAME_SIZE);
    std::memset(&tsHeader, 0, sizeof(TS_Header));
    std::memset(&tsAdaptation, 0, sizeof(TS_Adaptation_Field));
}

void TiVoDecoderTsPacket::setStream(TiVoDecoderTsStream *pStream)
{
    pParent = pStream;
}

int TiVoDecoderTsPacket::read(HappyFile *pInfile)
{
    int size = 0;
    bool loss_of_sync = false;

    if (!pInfile)
    {
        std::perror("bad parameter");
        return -1;
    }

    if (globalBufferLen > TS_FRAME_SIZE)
    {
        std::cerr << "globalBufferLen > TS_FRAME_SIZE, "
                     "pulling packet from globalBuffer[]\n";

        globalBufferLen -= TS_FRAME_SIZE;

        std::memmove(globalBuffer, globalBuffer + TS_FRAME_SIZE,
                     globalBufferLen);

        size = std::min(globalBufferLen, TS_FRAME_SIZE);
        std::memcpy(buffer, globalBuffer, size);
    }
    else
    {
        size = pInfile->read(buffer, TS_FRAME_SIZE);
        globalBufferLen = 0;

        if (IS_VVERBOSE)
            std::cerr << "Read handler : size " << size << "\n";
    }

    if (0 == size)
    {
        VERBOSE("End of file\n");
        isValid = true;
        return size;
    }
    else if (TS_FRAME_SIZE != size)
    {
        std::cerr << "Read error : TS Frame Size : " << TS_FRAME_SIZE
                  << ", Size Read " << size << "\n";
        if (size < 0)
            return -1;
        else
            return 0; // indicate EOF on partial packet
    }

    if (buffer[0] != 'G')
    {
        loss_of_sync = true;
        std::cerr << "loss_of_sync\n";

        if (globalBufferLen == 0)
        {
            std::memcpy(globalBuffer, buffer, size);
            globalBufferLen = size;
        }
    }

    int skip = 0;

    while (loss_of_sync)
    {
        for (int i = 1; 1; i++)
        {
            skip++;

            if (i < globalBufferLen)
            {
                if (globalBuffer[i] == 'G')
                {
                    std::cerr << "skipped " << skip << " bytes, found a SYNC\n";

                    globalBufferLen -= i;
                    std::memmove(globalBuffer, globalBuffer + i,
                                 globalBufferLen);
                    break;
                }
            }
            else
            {
                size = pInfile->read(globalBuffer, TS_FRAME_SIZE * 3);

                if (IS_VVERBOSE)
                    std::cerr << "Read handler : size " << size << "\n";

                if (0 == size)
                {
                    VERBOSE("End of file\n");
                    isValid = true;
                    return 0;
                }
                else if ((TS_FRAME_SIZE * 3) != size)
                {
                    std::cerr << "Read error : TS Frame Size * 3 : "
                              << TS_FRAME_SIZE * 3 << ", Size Read "
                              << size << "\n";

                    if (size < 0)
                        return -1;
                    else
                        return 0; // indicate EOF on partial packet
                }

                globalBufferLen = size;
                i = 0;
            }
        }

        size = pInfile->read(globalBuffer + globalBufferLen,
                             (3 * TS_FRAME_SIZE) - globalBufferLen);
        if (size < 0)
        {
            std::cerr << "ERROR: size=" << size << "\n";
            return -1;
        }

        globalBufferLen += size;

        if (globalBufferLen != (3 * TS_FRAME_SIZE))
        {
            std::cerr << "ERROR: globalBufferLen != (3 * TS_FRAME_SIZE)\n";
            return 0;  // indicate EOF on partial packet
        }

        if (globalBuffer[0] == 'G' && globalBuffer[TS_FRAME_SIZE] == 'G' &&
            globalBuffer[TS_FRAME_SIZE * 2] == 'G')
        {
            loss_of_sync = false;
            std::cerr << "found 3 syncs in a row, loss_of_sync = false\n";

            size = TS_FRAME_SIZE;
            std::memcpy(buffer, globalBuffer, size);
        }
    }

    isValid = true;
    return size;
}

bool TiVoDecoderTsPacket::decode()
{
    if (false == isValid)
    {
        std::perror("Packet not valid");
        return false;
    }

    // TS packet streams are big endian, and we may be running on little
    // endian platform.

    payloadOffset = 0;
    std::memset(&tsHeader, 0, sizeof(TS_Header));

    uint32_t ts_hdr_val = get32(&buffer[payloadOffset]);
    payloadOffset += 4;

    tsHeader.sync_byte                    = (ts_hdr_val & 0xff000000) >> 24;
    tsHeader.transport_error_indicator    = (ts_hdr_val & 0x00800000) >> 23;
    tsHeader.payload_unit_start_indicator = (ts_hdr_val & 0x00400000) >> 22;
    tsHeader.transport_priority           = (ts_hdr_val & 0x00200000) >> 21;
    tsHeader.pid                          = (ts_hdr_val & 0x001FFF00) >> 8;
    tsHeader.transport_scrambling_control = (ts_hdr_val & 0x000000C0) >> 6;
    tsHeader.adaptation_field_exists      = (ts_hdr_val & 0x00000020) >> 5;
    tsHeader.payload_data_exists          = (ts_hdr_val & 0x00000010) >> 4;
    tsHeader.continuity_counter           = (ts_hdr_val & 0x0000000F);

    if (tsHeader.sync_byte != 0x47)
    {
        std::perror("invalid ts pkt header");
        isValid = false;
        return false;
    }

    for (int i = 0; ts_packet_tags[i].ts_packet != TS_PID_TYPE_NONE; i++)
    {
        if (tsHeader.pid >= ts_packet_tags[i].code_match_lo &&
            tsHeader.pid <= ts_packet_tags[i].code_match_hi)
        {
            ts_packet_type = ts_packet_tags[i].ts_packet;
            break;
        }
    }

    if (tsHeader.adaptation_field_exists)
    {
        std::memset(&tsAdaptation, 0, sizeof(TS_Adaptation_Field));

        tsAdaptation.adaptation_field_length = buffer[payloadOffset];
        payloadOffset++;

        uint8_t ts_adapt_val = get16(&buffer[payloadOffset]);

        tsAdaptation.discontinuity_indicator =
            (ts_adapt_val & 0x80) >> 7;
        tsAdaptation.random_access_indicator =
            (ts_adapt_val & 0x40) >> 6;
        tsAdaptation.elementary_stream_priority_indicator =
            (ts_adapt_val & 0x20) >> 5;
        tsAdaptation.pcr_flag = (ts_adapt_val & 0x10) >> 4;
        tsAdaptation.opcr_flag = (ts_adapt_val & 0x08) >> 3;
        tsAdaptation.splicing_point_flag = (ts_adapt_val & 0x04) >> 2;
        tsAdaptation.transport_private_data_flag = (ts_adapt_val & 0x02) >> 1;
        tsAdaptation.adaptation_field_extension_flag = (ts_adapt_val & 0x01);

        payloadOffset += (tsAdaptation.adaptation_field_length);
    }

    return true;
}

void TiVoDecoderTsPacket::dump()
{
    const char *pidType;

    if (!(IS_VVERBOSE))
        return;

    switch (ts_packet_type)
    {
        case TS_PID_TYPE_RESERVED:
            { pidType = "Reserved"; break; }
        case TS_PID_TYPE_NULL_PACKET:
            { pidType = "NULL Packet"; break; }
        case TS_PID_TYPE_PROGRAM_ASSOCIATION_TABLE:
            { pidType = "Program Association Table"; break; }
        case TS_PID_TYPE_PROGRAM_MAP_TABLE:
            { pidType = "Program Map Table"; break; }
        case TS_PID_TYPE_CONDITIONAL_ACCESS_TABLE:
            { pidType = "Conditional Access Table"; break; }
        case TS_PID_TYPE_NETWORK_INFORMATION_TABLE:
            { pidType = "Network Information Table"; break; }
        case TS_PID_TYPE_SERVICE_DESCRIPTION_TABLE:
            { pidType = "Service Description Table"; break; }
        case TS_PID_TYPE_EVENT_INFORMATION_TABLE:
            { pidType = "Event Information Table"; break; }
        case TS_PID_TYPE_RUNNING_STATUS_TABLE:
            { pidType = "Running Status Table"; break; }
        case TS_PID_TYPE_TIME_DATE_TABLE:
            { pidType = "Time Date Table"; break; }
        case TS_PID_TYPE_NONE:
            { pidType = "None"; break; }
        case TS_PID_TYPE_AUDIO_VIDEO_PRIVATE_DATA:
        {
            if (true == isPmtPkt())
                pidType = "Program Map Table";
            else
                pidType = "Audio/Video/PrivateData";

            break;
        }

        default:
        {
            pidType = "**** UNKNOWN ***";
        }
    }

    std::cerr << "  TS Pkt header : " << pidType << "   : PktID "
              << packetId
              << "\n  TS Pkt header : " << pidType << "   : Valid Decode "
              << isValid
              << "\n  TS Pkt header :                 sync_byte : "
              << tsHeader.sync_byte
              << "\n  TS Pkt header : transport_error_indicator : "
              << tsHeader.transport_error_indicator
              << "\n  TS Pkt header : payload_unit_start_indica : "
              << tsHeader.payload_unit_start_indicator
              << "\n  TS Pkt header :        transport_priority : "
              << tsHeader.transport_priority
              << "\n  TS Pkt header :                       pid : "
              << tsHeader.pid
              << "\n  TS Pkt header : transport_scrambling_cont : "
              << tsHeader.transport_scrambling_control
              << "\n  TS Pkt header :   adaptation_field_exists : "
              << tsHeader.adaptation_field_exists
              << "\n  TS Pkt header :       payload_data_exists : "
              << tsHeader.payload_data_exists
              << "\n  TS Pkt header :        continuity_counter : "
              << tsHeader.continuity_counter << "\n";

    if (tsHeader.adaptation_field_exists)
    {
        std::cerr << "  TS Adaptation :   adaptation_field_length : "
                  << tsAdaptation.adaptation_field_length
                  << "\n  TS Adaptation :   discontinuity_indicator : "
                  << tsAdaptation.discontinuity_indicator
                  << "\n  TS Adaptation :   random_access_indicator : "
                  << tsAdaptation.random_access_indicator
                  << "\n  TS Adaptation : elementary_stream_priorit : "
                  << tsAdaptation.elementary_stream_priority_indicator
                  << "\n  TS Adaptation :                  pcr_flag : "
                  << tsAdaptation.pcr_flag
                  << "\n  TS Adaptation :                 opcr_flag : "
                  << tsAdaptation.opcr_flag
                  << "\n  TS Adaptation :       splicing_point_flag : "
                  << tsAdaptation.splicing_point_flag
                  << "\n  TS Adaptation : transport_private_data_fl : "
                  << tsAdaptation.transport_private_data_flag
                  << "\n  TS Adaptation : adaptation_field_extensio : "
                  << tsAdaptation.adaptation_field_extension_flag << "\n";
    }

    hexbulk(buffer, TS_FRAME_SIZE);
}

bool TiVoDecoderTsStream::decrypt(uint8_t *pBuffer, uint16_t bufferLen)
{
    if (!pParent)
    {
        std::perror("Stream does not have a parent decoder");
        return false;
    }

    if (IS_VVVERBOSE)
    {
        std::cerr << "AAA : dump turing : INIT\n";

        pParent->pTuring.dump();
    }

    if (pParent->do_header(&turing_stuff.key[0],
                           &(turing_stuff.block_no), NULL,
                           &(turing_stuff.crypted), NULL, NULL))
    {
        std::perror("do_header did not return 0!\n");
        return false;
    }

    if (IS_VVVERBOSE)
        std::cerr << "BBB : stream_id " << stream_id << ", blockno "
                  << turing_stuff.block_no << ", crypted "
                  << turing_stuff.crypted << "\n";
    if (IS_VERBOSE)
        std::cerr << pParent->pFileIn->tell() << " : stream_id: "
                  << stream_id << ", block_no: "
                  << turing_stuff.block_no << "\n";

    pParent->pTuring.prepare_frame(stream_id, turing_stuff.block_no);

    if (IS_VVVERBOSE)
    {
        std::cerr << "CCC : stream_id " << stream_id << ", blockno "
                     << turing_stuff.block_no << ", crypted "
                     << turing_stuff.crypted << "\n";

        std::cerr << "ZZZ : dump turing : BEFORE DECRYPT\n";

        pParent->pTuring.dump();
    }

    pParent->pTuring.active->decrypt_buffer(pBuffer, bufferLen);

    if (IS_VVVERBOSE)
    {
        std::cerr << "---Decrypted transport packet\n";

        hexbulk(pBuffer, bufferLen);
    }

    return true;
}

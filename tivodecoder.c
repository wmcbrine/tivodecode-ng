/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 *
 * derived from mpegcat, copyright 2006 Kees Cook, used with permission
 */
#include "tivodecoder.h"
#include "hexlib.h"
#include "Turing.h"
#include <stdio.h>
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif

/* TODO: clean this up */
extern int o_verbose;
extern int o_no_verify;
extern int o_ts_pkt_dump;

static packet_tag_info packet_tags[] = {
    {0x00, 0x00, PACK_SPECIAL},     // pic start
    {0x01, 0xAF, PACK_SPECIAL},     // video slices
    {0xB0, 0xB1, PACK_SPECIAL},     // reserved
    {0xB2, 0xB5, PACK_SPECIAL},     // user data, sequences
    {0xB6, 0xB6, PACK_SPECIAL},     // reserved
    {0xB7, 0xB9, PACK_SPECIAL},     // sequence, gop, end
    {0xBA, 0xBA, PACK_SPECIAL},     // pack
    {0xBB, 0xBB, PACK_PES_SIMPLE},  // system: same len as PES
    {0xBC, 0xBC, PACK_PES_SIMPLE},  // PES: prog stream map     *
    {0xBD, 0xBD, PACK_PES_COMPLEX}, // PES: priv 1
    {0xBE, 0xBF, PACK_PES_SIMPLE},  // PES: padding, priv 2     *
    {0xC0, 0xDF, PACK_PES_COMPLEX}, // PES: Audio
    {0xE0, 0xEF, PACK_PES_COMPLEX}, // PES: Video
    {0xF0, 0xF2, PACK_PES_SIMPLE},  // PES: ecm, emm, dsmcc     *
    {0xF3, 0xF7, PACK_PES_COMPLEX}, // PES: iso 13522/h2221a-d
    {0xF8, 0xF8, PACK_PES_SIMPLE},  // PES: h2221e              *
    {0xF9, 0xF9, PACK_PES_COMPLEX}, // PES: ancillary
    {0xFA, 0xFE, PACK_PES_SIMPLE},  // PES: reserved
    {0xFF, 0xFF, PACK_PES_SIMPLE},  // PES: prog stream dir     *
    {0, 0, PACK_NONE}       // end of list
};

static ts_packet_tag_info ts_packet_tags[] = {
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


static ts_pmt_stream_type_info ts_pmt_stream_tags[] = {
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


/**
 * This is from analyzing the TiVo directshow dll.  Most of the parameters I have no idea what they are for.
 *
 * @param arg_0     pointer to the 16 byte private data section of the packet header.
 * @param block_no  pointer to an integer to contain the block number used in the turing key
 * @param arg_8     no clue
 * @param crypted   pointer to an integer to contain 4 bytes of data encrypted
 *                  with the same turing cipher as the video.  No idea what to do with it once
 *                  it is decrypted, tho, but the turing needs to have 4 bytes
 *                  consumed in order to line up with the video/audio data.  My
 *                  guess is it is a checksum of some sort.
 * @param arg_10    no clue
 * @param arg_14    no clue
 *
 * @return count of particular bits which are zero.  They should all be 1, so the return value should be zero.
 *         I would consider a non-zero return an error.
 */
static int do_header(BYTE * arg_0, int * block_no, int * arg_8, int * crypted, int * arg_10, int * arg_14)
{
    int var_4 = 0;

    if (!(arg_0[0] & 0x80))
        var_4++;

    if (arg_10)
    {
        *arg_10 = (arg_0[0x0] & 0x78) >> 3;
    }

    if (arg_14)
    {
        *arg_14  = (arg_0[0x0] & 0x07) << 1;
        *arg_14 |= (arg_0[0x1] & 0x80) >> 7;
    }

    if (!(arg_0[1] & 0x40))
        var_4++;

    if (block_no)
    {
        *block_no  = (arg_0[0x1] & 0x3f) << 0x12;
        *block_no |= (arg_0[0x2] & 0xff) << 0xa;
        *block_no |= (arg_0[0x3] & 0xc0) << 0x2;

        if (!(arg_0[3] & 0x20))
            var_4++;

        *block_no |= (arg_0[0x3] & 0x1f) << 0x3;
        *block_no |= (arg_0[0x4] & 0xe0) >> 0x5;
    }

    if (!(arg_0[4] & 0x10))
        var_4++;

    if (arg_8)
    {
        *arg_8  = (arg_0[0x4] & 0x0f) << 0x14;
        *arg_8 |= (arg_0[0x5] & 0xff) << 0xc;
        *arg_8 |= (arg_0[0x6] & 0xf0) << 0x4;

        if (!(arg_0[6] & 0x8))
            var_4++;

        *arg_8 |= (arg_0[0x6] & 0x07) << 0x5;
        *arg_8 |= (arg_0[0x7] & 0xf8) >> 0x3;
    }

    if (crypted)
    {
        *crypted  = (arg_0[0xb] & 0x03) << 0x1e;
        *crypted |= (arg_0[0xc] & 0xff) << 0x16;
        *crypted |= (arg_0[0xd] & 0xfc) << 0xe;

        if (!(arg_0[0xd] & 0x2))
            var_4++;

        *crypted |= (arg_0[0xd] & 0x01) << 0xf;
        *crypted |= (arg_0[0xe] & 0xff) << 0x7;
        *crypted |= (arg_0[0xf] & 0xfe) >> 0x1;
    }

    if (!(arg_0[0xf] & 0x1))
        var_4++;

    return var_4;
}

#define LOOK_AHEAD(fh, bytes, n) do {\
    int retval = read_handler((bytes) + looked_ahead, (n) - looked_ahead, fh);\
    if ( retval == 0 )\
    {\
        return(0);  \
    }\
    else if ( retval != (n) - looked_ahead) { \
        perror ("read"); \
        return -1; \
    } else { \
        looked_ahead = (n); \
    } \
} while (0)

/*
 * called for each PS frame
 */
int process_ps_frame(unsigned char code, turing_state * turing, OFF_T_TYPE packet_start, void * packet_stream, read_func_t read_handler, void * ofh, write_func_t write_handler)
{
    static union {
        td_uint64_t align;
        unsigned char packet_buffer[65536 + sizeof(td_uint64_t) + 2];
    } aligned_buf;
    unsigned char bytes[32];
    int looked_ahead = 0;
    int i;
    int scramble=0;
    unsigned int header_len = 0;
    unsigned int length;

    memset( bytes, 0, 32 );

    for (i = 0; packet_tags[i].packet != PACK_NONE; i++)
    {
        if (code >= packet_tags[i].code_match_lo &&
                code <= packet_tags[i].code_match_hi)
        {
            if (packet_tags[i].packet == PACK_PES_SIMPLE
                    || packet_tags[i].packet == PACK_PES_COMPLEX)
            {
                if (packet_tags[i].packet == PACK_PES_COMPLEX)
                {
                    LOOK_AHEAD (packet_stream, bytes, 5);

                    // packet_length is 0 and 1
                    // PES header variables
                    // |    2        |    3         |   4   |
                    //  76 54 3 2 1 0 76 5 4 3 2 1 0 76543210
                    //  10 scramble   pts/dts    pes_crc
                    //        priority   escr      extension
                    //          alignment  es_rate   header_data_length
                    //            copyright  dsm_trick
                    //              copy       addtl copy

                    if ((bytes[2]>>6) != 0x2) {
                        VERBOSE( "PES (0x%02X) header mark != 0x2: 0x%x (is this an MPEG2-PS file?)\n",code,(bytes[2]>>6));
                    }

                    scramble=((bytes[2]>>4)&0x3);

                    header_len = 5 + bytes[4];

                    if (scramble == 3)
                    {
                        if (bytes[3] & 0x1)
                        {
                            int off = 6;
                            int ext_byte = 5;
                            int goagain = 0;
                            // extension
                            if (header_len > 32)
                                return -1;

                            LOOK_AHEAD (packet_stream, bytes, header_len);

                            do
                            {
                                goagain = 0;

                                //packet seq counter flag
                                if (bytes[ext_byte] & 0x20)
                                {
                                    off += 4;
                                }

                                //private data flag
                                if (bytes[ext_byte] & 0x80)
                                {
                                    int block_no    = 0;
                                    int crypted     = 0;

                                    VERBOSE("\n\n---Turing : Key\n");
                                    if ( IS_VERBOSE )
                                        hexbulk( (unsigned char *)&bytes[off], 16 );
                                    VERBOSE("---Turing : header : block %d crypted 0x%08x\n", block_no, crypted );

                                    if (do_header (&bytes[off], &block_no, NULL, &crypted, NULL, NULL))
                                    {
                                        VERBOSE( "do_header did not return 0!\n");
                                    }

                                    VERBOSE("BBB : code 0x%02x, blockno %d, crypted 0x%08x\n", code, block_no, crypted );
                                    VERBOSE("%zu" OFF_T_FORMAT ": stream_no: %x, block_no: %d\n", packet_start, code, block_no);
                                    VERBOSE("---Turing : prepare : code 0x%02x block_no %d\n", code, block_no );

                                    prepare_frame(turing, code, block_no);

                                    VERBOSE("CCC : code 0x%02x, blockno %d, crypted 0x%08x\n", code, block_no, crypted );
                                    VERBOSE("---Turing : decrypt : crypted 0x%08x len %d\n", crypted, 4 );

                                    decrypt_buffer(turing, (unsigned char *)&crypted, 4);

                                    VERBOSE("DDD : code 0x%02x, blockno %d, crypted 0x%08x\n", code, block_no, crypted );

                                }

                                // STD buffer flag
                                if (bytes[ext_byte] & 0x10)
                                {
                                    off += 2;
                                }

                                // extension flag 2
                                if (bytes[ext_byte] & 0x1)
                                {
                                    ext_byte = off;
                                    off++;
                                    goagain = 1;
                                    continue;
                                }
                            } while (goagain);
                        }
                    }
                }
                else
                {
                    LOOK_AHEAD (packet_stream, bytes, 2);
                }

                length = bytes[1] | (bytes[0] << 8);

                memcpy (aligned_buf.packet_buffer + sizeof(td_uint64_t), bytes, looked_ahead);

                LOOK_AHEAD (packet_stream, aligned_buf.packet_buffer + sizeof(td_uint64_t), length + 2);
                {
                    unsigned char * packet_ptr = aligned_buf.packet_buffer + sizeof(td_uint64_t);
                    size_t packet_size;

                    aligned_buf.packet_buffer[sizeof(td_uint64_t)-1] = code;

                    if (header_len)
                    {
                        packet_ptr += header_len;
                        packet_size = length - header_len + 2;
                    }
                    else
                    {
                        packet_ptr += 2;
                        packet_size = length;
                    }

                    if (scramble == 3)
                    {
                        VERBOSE("---Turing : decrypt : size %d\n", (int)packet_size );

                        decrypt_buffer(turing, packet_ptr, packet_size);

                        // turn off scramble bits
                        aligned_buf.packet_buffer[sizeof(td_uint64_t)+2] &= ~0x30;

                        // scan video buffer for Slices.  If no slices are
                        // found, the MAK is wrong.
                        if (!o_no_verify && code == 0xe0) {
                            int slice_count=0;
                            size_t offset;

                            for (offset=sizeof(td_uint64_t);offset+4<packet_size;offset++)
                            {
                                if (aligned_buf.packet_buffer[offset] == 0x00 &&
                                    aligned_buf.packet_buffer[offset+1] == 0x00 &&
                                    aligned_buf.packet_buffer[offset+2] == 0x01 &&
                                    aligned_buf.packet_buffer[offset+3] >= 0x01 &&
                                    aligned_buf.packet_buffer[offset+3] <= 0xAF)
                                {
                                    slice_count++;
                                }
                                // choose 8 as a good test that if 8 slices
                                // are seen, it's probably not random noise
                                if (slice_count>8)
                                {
                                    // disable future verification
                                    o_no_verify = 1;
                                }
                            }
                            if (!o_no_verify)
                            {
                                VERBOSE( "Invalid MAK -- aborting\n");
                                return -2;
                            }
                        }
                    }
                    else if (code == 0xbc)
                    {
                        // don't know why, but tivo dll does this.
                        // I can find no good docs on the format of the program_stream_map
                        // but I think this clears a reserved bit.  No idea why
                        aligned_buf.packet_buffer[sizeof(td_uint64_t)+2] &= ~0x20;
                    }

                    if (write_handler(aligned_buf.packet_buffer + sizeof(td_uint64_t) - 1, length + 3, ofh) != length + 3)
                    {
                        perror ("writing buffer");
                    }

                    return 1;
                }
            }

            return 0;
        }
    }

    return -1;
}

/*
 * called for each TS frame
 */

int ts_fill_headers( TS_Stream * pStream )
{
    unsigned int ts_hdr_val     = 0;
    unsigned short ts_adapt_val = 0;
    int i                       = 0;
    unsigned char * pPtr        = NULL;

    if ( !pStream || !pStream->pPacket )
    {
        perror("Invalid TS header argument");
        return(-1);
    }

    pPtr = pStream->pPacket;

    // Yuck.  Make sure that we're dealing with the proper endianess.
    // TS packet streams are big endian, and we may be running on little endian platform.

    memset( &pStream->ts_header, 0, sizeof(TS_Header) );
    ts_hdr_val  = portable_ntohl( pPtr );
    pPtr        += 4;

    pStream->ts_header.sync_byte                    = ( ts_hdr_val & 0xff000000 ) >> 24;
    pStream->ts_header.transport_error_indicator    = ( ts_hdr_val & 0x00800000 ) >> 23;
    pStream->ts_header.payload_unit_start_indicator = ( ts_hdr_val & 0x00400000 ) >> 22;
    pStream->ts_header.transport_priority           = ( ts_hdr_val & 0x00200000 ) >> 21;
    pStream->ts_header.pid                          = ( ts_hdr_val & 0x001FFF00 ) >> 8;
    pStream->ts_header.transport_scrambling_control = ( ts_hdr_val & 0x000000C0 ) >> 6;
    pStream->ts_header.adaptation_field_exists      = ( ts_hdr_val & 0x00000020 ) >> 5;
    pStream->ts_header.payload_data_exists          = ( ts_hdr_val & 0x00000010 ) >> 4;
    pStream->ts_header.continuity_counter           = ( ts_hdr_val & 0x0000000F );

    if ( pStream->ts_header.sync_byte != 0x47 )
    {
        VERBOSE( "TS header : incorrect sync byte [%02x]\n", pStream->ts_header.sync_byte );
        VERBOSE( "TS header : ts_hdr_val 0x%08x\n", ts_hdr_val );
        return(-1);
    }

    for (i = 0; ts_packet_tags[i].ts_packet != TS_PID_TYPE_NONE; i++)
    {
        if (pStream->ts_header.pid >= ts_packet_tags[i].code_match_lo &&
                pStream->ts_header.pid <= ts_packet_tags[i].code_match_hi)
        {
            pStream->ts_packet_type = ts_packet_tags[i].ts_packet;
            break;
        }
    }

    if ( pStream->ts_header.adaptation_field_exists )
    {
        memset( &pStream->ts_adapt, 0, sizeof(TS_Adaptation_Field) );

        ts_adapt_val                                            = portable_ntohs( pPtr );
        pPtr++;

        pStream->ts_adapt.adaptation_field_length               = (ts_adapt_val & 0xff00) >> 8;
        pStream->ts_adapt.discontinuity_indicator               = (ts_adapt_val & 0x0080) >> 7;
        pStream->ts_adapt.random_access_indicator               = (ts_adapt_val & 0x0040) >> 6;
        pStream->ts_adapt.elementary_stream_priority_indicator  = (ts_adapt_val & 0x0020) >> 5;
        pStream->ts_adapt.pcr_flag                              = (ts_adapt_val & 0x0010) >> 4;
        pStream->ts_adapt.opcr_flag                             = (ts_adapt_val & 0x0008) >> 3;
        pStream->ts_adapt.splicing_point_flag                   = (ts_adapt_val & 0x0004) >> 2;
        pStream->ts_adapt.transport_private_data_flag           = (ts_adapt_val & 0x0002) >> 1;
        pStream->ts_adapt.adaptation_field_extension_flag       = (ts_adapt_val & 0x0001);

        pPtr += pStream->ts_adapt.adaptation_field_length;
    }

    pStream->payload_offset = (unsigned int)(pPtr - pStream->pPacket);
    return(0);
}


void ts_dump_headers( OFF_T_TYPE ts_offset, TS_Stream * pStream )
{
    char pidType[50];

    if ( !pStream )
    {
        perror("Invalid TS header argument");
        return;
    }

    VERBOSE("TS   Offset 0x%zx (%zu)\n", ts_offset,   ts_offset );
    VERBOSE("MPEG Offset 0x%zx (%zu)\n", ts_offset - pStream->initial_offset, ts_offset - pStream->initial_offset );
    VERBOSE("Packet Counter 0x%08x (%d)\n", pStream->packet_counter, pStream->packet_counter );

    switch(pStream->ts_packet_type)
    {
        case TS_PID_TYPE_RESERVED                   : { sprintf(pidType,  "Reserved"); break; }
        case TS_PID_TYPE_NULL_PACKET                : { sprintf(pidType,  "NULL Packet"); break; }
        case TS_PID_TYPE_PROGRAM_ASSOCIATION_TABLE  : { sprintf(pidType,  "Program Association Table"); break; }
        case TS_PID_TYPE_PROGRAM_MAP_TABLE          : { sprintf(pidType,  "Program Map Table"); break; }
        case TS_PID_TYPE_CONDITIONAL_ACCESS_TABLE   : { sprintf(pidType,  "Conditional Access Table"); break; }
        case TS_PID_TYPE_NETWORK_INFORMATION_TABLE  : { sprintf(pidType,  "Network Information Table"); break; }
        case TS_PID_TYPE_SERVICE_DESCRIPTION_TABLE  : { sprintf(pidType,  "Service Description Table"); break; }
        case TS_PID_TYPE_EVENT_INFORMATION_TABLE    : { sprintf(pidType,  "Event Information Table"); break; }
        case TS_PID_TYPE_RUNNING_STATUS_TABLE       : { sprintf(pidType,  "Running Status Table"); break; }
        case TS_PID_TYPE_TIME_DATE_TABLE            : { sprintf(pidType,  "Time Date Table"); break; }
        case TS_PID_TYPE_NONE                       : { sprintf(pidType,  "None"); break; }
        case TS_PID_TYPE_AUDIO_VIDEO_PRIVATE_DATA :
        {
            if ( pStream->ts_pat.program_map_pid == pStream->ts_header.pid )
            {
                sprintf(pidType,  "Program Map Table");
            }
            else
            {
                sprintf(pidType,  "Audio/Video/PrivateData");
            }

            break;
        }

        default :
        {
            sprintf(pidType,  "**** UNKNOWN ***");
        }
    }

    VERBOSE("%-15s : %s\n", "TS Pkt header", pidType );

    VERBOSE("%-15s : %-25.25s : 0x%04x\n", "TS Pkt header",
            "sync_byte", pStream->ts_header.sync_byte );
    VERBOSE("%-15s : %-25.25s : %06d\n", "TS Pkt header",
            "transport_error_indicator", pStream->ts_header.transport_error_indicator );
    VERBOSE("%-15s : %-25.25s : %06d\n", "TS Pkt header",
            "payload_unit_start_indicator", pStream->ts_header.payload_unit_start_indicator);
    VERBOSE("%-15s : %-25.25s : %06d\n", "TS Pkt header",
            "transport_priority", pStream->ts_header.transport_priority);
    VERBOSE("%-15s : %-25.25s : 0x%04x\n", "TS Pkt header",
            "pid", pStream->ts_header.pid);
    VERBOSE("%-15s : %-25.25s : %06d\n", "TS Pkt header",
            "transport_scrambling_control", pStream->ts_header.transport_scrambling_control);
    VERBOSE("%-15s : %-25.25s : %06d\n", "TS Pkt header",
            "adaptation_field_exists", pStream->ts_header.adaptation_field_exists);
    VERBOSE("%-15s : %-25.25s : %06d\n", "TS Pkt header",
            "payload_data_exists", pStream->ts_header.payload_data_exists);
    VERBOSE("%-15s : %-25.25s : %06d\n", "TS Pkt header",
            "continuity_counter", pStream->ts_header.continuity_counter);

    if ( pStream->ts_header.adaptation_field_exists )
    {
        VERBOSE("%-15s : %-25.25s : %06d\n", "TS Adaptation",
                "adaptation_field_length", pStream->ts_adapt.adaptation_field_length);

        VERBOSE("%-15s : %-25.25s : %06d\n", "TS Adaptation",
                "discontinuity_indicator", pStream->ts_adapt.discontinuity_indicator);

        VERBOSE("%-15s : %-25.25s : %06d\n", "TS Adaptation",
                "random_access_indicator", pStream->ts_adapt.random_access_indicator);

        VERBOSE("%-15s : %-25.25s : %06d\n", "TS Adaptation",
                "elementary_stream_priority_indicator", pStream->ts_adapt.elementary_stream_priority_indicator);

        VERBOSE("%-15s : %-25.25s : %06d\n", "TS Adaptation",
                "pcr_flag", pStream->ts_adapt.pcr_flag);

        VERBOSE("%-15s : %-25.25s : %06d\n", "TS Adaptation",
                "opcr_flag", pStream->ts_adapt.opcr_flag);

        VERBOSE("%-15s : %-25.25s : %06d\n", "TS Adaptation",
                "splicing_point_flag", pStream->ts_adapt.splicing_point_flag);

        VERBOSE("%-15s : %-25.25s : %06d\n", "TS Adaptation",
                "transport_private_data_flag",  pStream->ts_adapt.transport_private_data_flag);

        VERBOSE("%-15s : %-25.25s : %06d\n", "TS Adaptation",
                "adaptation_field_extension_flag", pStream->ts_adapt.adaptation_field_extension_flag);
    }
    return;
}


int ts_handle_tivo_private_data( TS_Stream * pStream, turing_state * turing )
{
    unsigned char * pPtr    = NULL;
    int stream_loop         = 0;
    unsigned int validator      = 0;
    unsigned short pid          = 0;
    unsigned char  stream_id    = 0;
    unsigned short stream_bytes = 0;
    unsigned int foundit        = 0;

    if ( !pStream || !pStream->pPacket )
    {
        perror("Invalid TS header argument");
        return(-1);
    }

    pPtr = pStream->pPacket + pStream->payload_offset;

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
        return(-1);
    }

    VERBOSE("%-15s : %-25.25s : 0x%08x (%c%c%c%c)\n",   "TiVo Private",
            "Validator", validator, *pPtr, *(pPtr+1), *(pPtr+2), *(pPtr+3) );

    VERBOSE("%-15s : %-25.25s : 0x%x 0x%x 0x%x 0x%x\n", "TiVo Private",
            "Unknown", *(pPtr+4), *(pPtr+5), *(pPtr+6), *(pPtr+7) );

    pPtr += 4;  // advance past "TiVo"

    pPtr += 4;  // advance past ??? field

    stream_bytes = portable_ntohs( pPtr );
    pPtr += 2;  // advance past stream_bytes

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

        for ( foundit = 0, stream_loop = 0; stream_loop<TS_STREAM_ELEMENT_MAX; stream_loop++ )
        {
            if ( pStream->ts_stream_elem[stream_loop].stream_pid == pid )
            {
                foundit = 1;
                pStream->ts_stream_elem[stream_loop].stream_id = stream_id;

                if ( memcmp( &pStream->ts_stream_elem[stream_loop].turing_stuff.key[0], pPtr, 16 ) )
                {
                    VVERBOSE( "\nUpdating PID 0x%04x Type 0x%02x Turing Key\n", pid, stream_id );
                    if ( IS_VVERBOSE )
                    {
                        hexbulk( &pStream->ts_stream_elem[stream_loop].turing_stuff.key[0], 16 );
                        hexbulk( pPtr, 16 );
                    }

                    memcpy( &pStream->ts_stream_elem[stream_loop].turing_stuff.key[0], pPtr, 16);
                }

                VERBOSE("%-15s : %-25.25s : %d\n", "TiVo Private", "Block No",
                        pStream->ts_stream_elem[stream_loop].turing_stuff.block_no );
                VERBOSE("%-15s : %-25.25s : 0x%08x\n", "TiVo Private", "Crypted",
                        pStream->ts_stream_elem[stream_loop].turing_stuff.crypted );
                VERBOSE("%-15s : %-25.25s : 0x%04x (%d)\n", "TiVo Private", "PID",
                        pStream->ts_stream_elem[stream_loop].stream_pid,
                        pStream->ts_stream_elem[stream_loop].stream_pid );
                VERBOSE("%-15s : %-25.25s : 0x%02x (%d)\n", "TiVo Private", "Stream ID",
                        pStream->ts_stream_elem[stream_loop].stream_id,
                        pStream->ts_stream_elem[stream_loop].stream_id );
                VERBOSE("%-15s : %-25.25s : ", "TiVo Private", "Turing Key" );
                if ( IS_VERBOSE )
                    hexbulk( &pStream->ts_stream_elem[stream_loop].turing_stuff.key[0], 16 );
                break;
            }
        }

        if ( !foundit )
        {
            perror("TiVo Private Data : Unmatched Stream ID");
            return(-1);
        }

        pPtr += 16;
        stream_bytes -= 16;
    }

    return(0);
}


int ts_handle_pmt( TS_Stream * pStream, turing_state * turing )
{
    unsigned short section_length   = 0;
    unsigned short pmt_field        = 0;
    unsigned short i                = 0;
    unsigned char * pPtr            = NULL;

    if ( !pStream || !pStream->pPacket )
    {
        perror("Invalid TS header argument");
        return(-1);
    }

    VERBOSE("\n" );

    pPtr = pStream->pPacket + pStream->payload_offset;

    if ( pStream->ts_header.payload_unit_start_indicator )
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
        int foundit = 0;
        int j = 0;

        pStream->ts_stream_elem[i].stream_type_id = *pPtr;
        for (j = 0; ts_pmt_stream_tags[j].ts_stream_type != TS_STREAM_TYPE_NONE; j++)
        {
            if ( ( pStream->ts_stream_elem[i].stream_type_id >= ts_pmt_stream_tags[j].code_match_lo )  &&
                 ( pStream->ts_stream_elem[i].stream_type_id <= ts_pmt_stream_tags[j].code_match_hi ) )
            {
                pStream->ts_stream_elem[i].stream_type = ts_pmt_stream_tags[j].ts_stream_type;
                foundit = 1;
                break;
        }
        }

        if ( !foundit )
        {
            pStream->ts_stream_elem[i].stream_type_id = TS_STREAM_TYPE_PRIVATE_DATA;
        }

        switch( pStream->ts_stream_elem[i].stream_type )
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
        pStream->ts_stream_elem[i].stream_pid = pmt_field & 0x1fff;

        // advance past elementary field
        pPtr += 2;
        section_length -= 2;

        pmt_field = portable_ntohs( pPtr );
        es_info_length = pmt_field & 0x1fff;

        // advance past ES info length field
        pPtr += 2;
        section_length -= 2;

        // advance past es info
        pPtr += es_info_length;
        section_length -= es_info_length;

        VERBOSE("%-15s : StreamId 0x%x (%d), PID 0x%x (%d), Type 0x%0x (%d)(%s)\n", "TS ProgMapTbl",
                pStream->ts_stream_elem[i].stream_type_id,
                pStream->ts_stream_elem[i].stream_type_id,
                pStream->ts_stream_elem[i].stream_pid,
                pStream->ts_stream_elem[i].stream_pid,
                pStream->ts_stream_elem[i].stream_type,
                pStream->ts_stream_elem[i].stream_type,
                strTypeStr );
    }

    return(0);
}


int ts_handle_pat( TS_Stream * pStream, turing_state * turing )
{
    unsigned short pat_field            = 0;
    unsigned short section_length       = 0;
    unsigned short transport_stream_id  = 0;
    unsigned char * pPtr                = NULL;

    VERBOSE("\n" );

    if ( !pStream || !pStream->pPacket )
    {
        perror("Invalid TS header argument");
        return(-1);
    }

    pPtr = pStream->pPacket + pStream->payload_offset;

    if ( pStream->ts_header.payload_unit_start_indicator )
    {
        pPtr++; // advance past pointer field
    }

    if ( *pPtr != 0x00 )
    {
        perror("PAT Table ID must be 0x00");
        return(-1);
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
        return(-1);
    }

    if ( (pat_field & 0x0C00) != 0x0000 )
    {
        perror("Failed to validate PAT MBZ of section length");
        return(-1);
    }

    transport_stream_id = portable_ntohs( pPtr );
    pPtr += 2;
    section_length -= 2;

    if ( (*pPtr & 0x3E) != pStream->ts_pat.version_number )
    {
        pStream->ts_pat.version_number = *pPtr & 0x3E;
        VERBOSE( "%-15s : version changed : %d\n", "TS ProgAssocTbl",
                pStream->ts_pat.version_number );
    }

    pPtr++;
    section_length--;

    pStream->ts_pat.section_number = *pPtr++;
    section_length--;

    pStream->ts_pat.last_section_number = *pPtr++;
    section_length--;

    section_length -= 4; // ignore the CRC for now

    while ( section_length > 0 )
    {
        pat_field = portable_ntohs( pPtr );
        VERBOSE( "%-15s : Program Num : %d\n", "TS ProgAssocTbl", pat_field );
        pPtr += 2;
        section_length -= 2;

        pat_field = portable_ntohs( pPtr );

        pStream->ts_pat.program_map_pid = pat_field & 0x1FFF;
        VERBOSE( "%-15s : Program PID : 0x%x (%d)\n", "TS ProgAssocTbl",
                pStream->ts_pat.program_map_pid, pStream->ts_pat.program_map_pid );
        pPtr += 2;
        section_length -= 2;
    }

    return(0);
}


int ts_handle_audio_video( TS_Stream * pStream, turing_state * turing )
{
    unsigned char * pPtr        = pStream->pPacket + pStream->payload_offset;
    int stream_loop             = 0;
    int foundit                 = 0 ;
    int pkt_offset              = pStream->payload_offset;
    unsigned char pes_start[]   = { 0x00, 0x00, 0x01 };

    if ( !pStream || !pStream->pPacket )
    {
        perror("Invalid TS header argument");
        return(-1);
    }

    // locate the specific stream that we're dealing with
    for ( stream_loop = 0; stream_loop<TS_STREAM_ELEMENT_MAX; stream_loop++ )
    {
        if ( pStream->ts_stream_elem[stream_loop].stream_pid == pStream->ts_header.pid )
        {
            foundit = 1;
            break;
        }
    }

    // valid stream id ?
    if (!foundit)
    {
        perror("Invalid TS PID");
        return(0);
    }

    // no need to decrypt is there's no payload,
    // or if the payload is not encrypted
    //  if ( !pStream->ts_header.payload_data_exists || !pStream->ts_header.transport_scrambling_control )
    if ( !pStream->ts_header.payload_data_exists  )
    {
        return(0);
    }

    if ( pStream->ts_header.transport_scrambling_control )
    {
        VVERBOSE( "\n--- Encrypted transport packet\n");
        if ( IS_VVERBOSE )
        {
            hexbulk( (unsigned char *)pStream->pPacket, TS_FRAME_SIZE );
        }
    }

    // reset the PES helper fields if this is the start of a PES
    if ( pStream->ts_header.payload_unit_start_indicator )
    {
        memset( pStream->ts_stream_elem[stream_loop].PES_hdr, 0, TS_PES_HDR_BUFSIZE );
        pStream->ts_stream_elem[stream_loop].PES_hdr_len = 0;
    }

    if ( pStream->ts_header.payload_unit_start_indicator || pStream->ts_stream_elem[stream_loop].PES_hdr_len )
    {

        // advance, byte by byte, through the PES headers.   painful but necessary...
        // we need to do this to cover the case where PES headers span multiple TS packets.
        for ( pkt_offset=pStream->payload_offset; pkt_offset<TS_FRAME_SIZE; pkt_offset++ )
        {
            printf("[%d]stream pkt %d : pkt_offset %d\n", __LINE__, pStream->packet_counter, pkt_offset );

            pStream->ts_stream_elem[stream_loop].PES_hdr[pStream->ts_stream_elem[stream_loop].PES_hdr_len++] = *(pStream->pPacket+pkt_offset);
            pPtr++;

            if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 4 )
            {
                printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                continue;
            }
            else if ( memcmp( pes_start, pStream->ts_stream_elem[stream_loop].PES_hdr, 3 ) )
            {
                printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                continue;
            }

            // Start of Video Elementary Stream
            if ( ( pStream->ts_stream_elem[stream_loop].PES_hdr[3] == 0xBD ) ||
                 ( pStream->ts_stream_elem[stream_loop].PES_hdr[3] >= 0xC0 && pStream->ts_stream_elem[stream_loop].PES_hdr[3] <= 0xEF ) )
            {
                printf("[%d]stream pkt %d : Video Elementary Stream\n", __LINE__, pStream->packet_counter );

                pStream->ts_stream_elem[stream_loop].stream_id = pStream->ts_stream_elem[stream_loop].PES_hdr[3];

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 4+2 )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                pStream->ts_stream_elem[stream_loop].pkt_length = portable_ntohs( &pStream->ts_stream_elem[stream_loop].PES_hdr[4] );

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 4+3 )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                pStream->ts_stream_elem[stream_loop].PES_pkt.marker_bits                = (pStream->ts_stream_elem[stream_loop].PES_hdr[6] & 0xc0) >> 6;
                pStream->ts_stream_elem[stream_loop].PES_pkt.scrambling_control         = (pStream->ts_stream_elem[stream_loop].PES_hdr[6] & 0x30) >> 4;
                pStream->ts_stream_elem[stream_loop].PES_pkt.priority                   = (pStream->ts_stream_elem[stream_loop].PES_hdr[6] & 0x08) >> 3;
                pStream->ts_stream_elem[stream_loop].PES_pkt.data_alignment_indicator   = (pStream->ts_stream_elem[stream_loop].PES_hdr[6] & 0x04) >> 2;
                pStream->ts_stream_elem[stream_loop].PES_pkt.copyright                  = (pStream->ts_stream_elem[stream_loop].PES_hdr[6] & 0x02) >> 1;
                pStream->ts_stream_elem[stream_loop].PES_pkt.original_or_copy           = (pStream->ts_stream_elem[stream_loop].PES_hdr[6] & 0x01);

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 4+4 )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                pStream->ts_stream_elem[stream_loop].PES_pkt.PTS_DTS_indicator          = (pStream->ts_stream_elem[stream_loop].PES_hdr[7] & 0xc0) >> 6;
                pStream->ts_stream_elem[stream_loop].PES_pkt.ESCR_flag                  = (pStream->ts_stream_elem[stream_loop].PES_hdr[7] & 0x20) >> 5;
                pStream->ts_stream_elem[stream_loop].PES_pkt.ES_rate_flag               = (pStream->ts_stream_elem[stream_loop].PES_hdr[7] & 0x10) >> 4;
                pStream->ts_stream_elem[stream_loop].PES_pkt.DSM_trick_mode_flag        = (pStream->ts_stream_elem[stream_loop].PES_hdr[7] & 0x08) >> 3;
                pStream->ts_stream_elem[stream_loop].PES_pkt.additional_copy_info_flag  = (pStream->ts_stream_elem[stream_loop].PES_hdr[7] & 0x04) >> 2;
                pStream->ts_stream_elem[stream_loop].PES_pkt.CRC_flag                   = (pStream->ts_stream_elem[stream_loop].PES_hdr[7] & 0x02) >> 1;
                pStream->ts_stream_elem[stream_loop].PES_pkt.extension_flag             = (pStream->ts_stream_elem[stream_loop].PES_hdr[7] & 0x01);

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 4+5 )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                pStream->ts_stream_elem[stream_loop].PES_pkt.PES_header_length = pStream->ts_stream_elem[stream_loop].PES_hdr[8];

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 4+5+pStream->ts_stream_elem[stream_loop].PES_pkt.PES_header_length )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                VVERBOSE("%-15s : %-25.25s : 0x%02x (%d)(%s)\n", "TS PES Packet", "stream_id",
                        pStream->ts_stream_elem[stream_loop].stream_id,
                        pStream->ts_stream_elem[stream_loop].stream_id,
                        (pStream->ts_stream_elem[stream_loop].stream_id<0xe0) ? "audio" : "video" );

                VVERBOSE("%-15s : %-25.25s : 0x%04x (%d)\n", "TS PES Packet", "pkt_length",
                        pStream->ts_stream_elem[stream_loop].pkt_length,
                        pStream->ts_stream_elem[stream_loop].pkt_length );

                VVERBOSE( "%-15s : %-25.25s\n", "TS PES Packet", "Extension Hdr" );

                VVERBOSE("%-15s : %-25.25s : 0x%x (%d)\n", "TS PES Packet", "scrambling_control",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.scrambling_control,
                        pStream->ts_stream_elem[stream_loop].PES_pkt.scrambling_control );

                VVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet", "priority",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.priority );

                VVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet", "data_alignment_indicator",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.data_alignment_indicator );

                VVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet", "copyright",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.copyright );

                VVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet", "original_or_copy",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.original_or_copy );

                VVERBOSE("%-15s : %-25.25s : 0x%x (%d)\n", "TS PES Packet", "PTS_DTS_indicator",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.PTS_DTS_indicator,
                        pStream->ts_stream_elem[stream_loop].PES_pkt.PTS_DTS_indicator );

                VVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet", "ESCR_flag",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.ESCR_flag );

                VVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet", "ES_rate_flag",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.ES_rate_flag );

                VVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet", "DSM_trick_mode_flag",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.DSM_trick_mode_flag );

                VVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet", "additional_copy_info_flag",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.additional_copy_info_flag );

                VVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet", "CRC_flag",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.CRC_flag );

                VVERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet", "extension_flag",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.extension_flag );

                VERBOSE("%-15s : %-25.25s : %d\n", "TS PES Packet", "PES_header_length",
                        pStream->ts_stream_elem[stream_loop].PES_pkt.PES_header_length );

                // reset PES header helper structs -- we're done with this one
                memset( pStream->ts_stream_elem[stream_loop].PES_hdr, 0, TS_PES_HDR_BUFSIZE );
                pStream->ts_stream_elem[stream_loop].PES_hdr_len = 0;
            }
            else if ( pStream->ts_stream_elem[stream_loop].PES_hdr[3] == EXTENSION_START_CODE )
            {
                unsigned int ext_hdr_len = 0;

                printf("[%d]stream pkt %d : EXTENSION_START_CODE\n", __LINE__, pStream->packet_counter );

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 4+4 )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                if ( (pStream->ts_stream_elem[stream_loop].PES_hdr[4] & 0x10) == 0x10 )
                {
                    ext_hdr_len += 6;
                }
                else if ( (pStream->ts_stream_elem[stream_loop].PES_hdr[4] & 0x20) == 0x20 )
                {
                    if ( (pStream->ts_stream_elem[stream_loop].PES_hdr[4] & 0x01) == 0x01 )
                    {
                        ext_hdr_len += 3;
                    }
                    ext_hdr_len += 5;
                }
                else if ( (pStream->ts_stream_elem[stream_loop].PES_hdr[4] & 0x80) == 0x80 )
                {
                    if ( ((pStream->ts_stream_elem[stream_loop].PES_hdr[4+4]) & 0x40) == 0x40 )
                    {
                        ext_hdr_len += 2;
                    }
                    ext_hdr_len += 5;
                }

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 4+ext_hdr_len )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

    //          while ( (*(pPtr+0)!=0x00) || (*(pPtr+1)!=0x00) || (*(pPtr+2)!=0x01) )
    //          {
    //              pPtr++;
    //          }

                // reset PES header helper structs -- we're done with this one
                memset( pStream->ts_stream_elem[stream_loop].PES_hdr, 0, TS_PES_HDR_BUFSIZE );
                pStream->ts_stream_elem[stream_loop].PES_hdr_len = 0;

                VERBOSE( "%-15s : %-25.25s : %d bytes\n", "TS PES Packet",
                        "Extension header", ext_hdr_len );
            }
            else if ( pStream->ts_stream_elem[stream_loop].PES_hdr[3] == GROUP_START_CODE )
            {
                printf("[%d]stream pkt %d : GROUP_START_CODE\n", __LINE__, pStream->packet_counter );

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 4+4 )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                // reset PES header helper structs -- we're done with this one
                memset( pStream->ts_stream_elem[stream_loop].PES_hdr, 0, TS_PES_HDR_BUFSIZE );
                pStream->ts_stream_elem[stream_loop].PES_hdr_len = 0;

                VERBOSE("%-15s : %-25.25s : %d bytes\n", "TS PES Packet", "Group Of Pictures", 4 );
            }
            else if ( pStream->ts_stream_elem[stream_loop].PES_hdr[3] == USER_DATA_START_CODE )
            {
                printf("[%d]stream pkt %d : USER_DATA_START_CODE\n", __LINE__, pStream->packet_counter );


                // Continue into packet until we see the next start code
                if ( !memcmp( &pStream->ts_stream_elem[stream_loop].PES_hdr[pStream->ts_stream_elem[stream_loop].PES_hdr_len-3], pes_start, 3 ) )
                {
                    // reset PES header helper structs -- we're done with this one
                    pStream->ts_stream_elem[stream_loop].PES_hdr_len = 3;
                    memcpy(pStream->ts_stream_elem[stream_loop].PES_hdr, pes_start, 3);
                }

                VERBOSE( "%-15s : %-25.25s\n", "TS PES Packet", "User Data");
            }
            else if ( pStream->ts_stream_elem[stream_loop].PES_hdr[3] == PICTURE_START_CODE )
            {
                printf("[%d]stream pkt %d : PICTURE_START_CODE\n", __LINE__, pStream->packet_counter );

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 4+4 )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr[pStream->ts_stream_elem[stream_loop].PES_hdr_len-1] & 0x80 )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                // reset PES header helper structs -- we're done with this one
                memset( pStream->ts_stream_elem[stream_loop].PES_hdr, 0, TS_PES_HDR_BUFSIZE );
                pStream->ts_stream_elem[stream_loop].PES_hdr_len = 0;

                VERBOSE( "%-15s : %-25.25s : %d bytes\n", "TS PES Packet", "Picture", 4 );
            }
            else if ( pStream->ts_stream_elem[stream_loop].PES_hdr[3] == SEQUENCE_HEADER_CODE )
            {
                unsigned int PES_load_intra_flag        = 0;
                unsigned int PES_load_non_intra_flag    = 0;
                unsigned int optional_bytes             = 0;

                printf("[%d]stream pkt %d : SEQUENCE_HEADER_CODE\n", __LINE__, pStream->packet_counter );

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 12 )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                PES_load_intra_flag = pStream->ts_stream_elem[stream_loop].PES_hdr[11] & 0x02;
                if ( PES_load_intra_flag )
                    optional_bytes += 64;

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 12+optional_bytes )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                PES_load_non_intra_flag = pStream->ts_stream_elem[stream_loop].PES_hdr[11+optional_bytes] & 0x01;
                if ( PES_load_non_intra_flag )
                    optional_bytes += 64;

                if ( pStream->ts_stream_elem[stream_loop].PES_hdr_len < 12+optional_bytes )
                {
                    printf("[%d]stream pkt %d : continue\n", __LINE__, pStream->packet_counter );
                    continue;
                }

                // reset PES header helper structs -- we're done with this one
                memset( pStream->ts_stream_elem[stream_loop].PES_hdr, 0, TS_PES_HDR_BUFSIZE );
                pStream->ts_stream_elem[stream_loop].PES_hdr_len = 0;

                VVERBOSE( "%-15s : %-25.25s\n", "TS PES Packet", "Sequence header" );

                VERBOSE( "%-15s : %-25.25s : %d\n", "TS PES Packet",
                        "PES_load_intra_flag", PES_load_intra_flag );

                VERBOSE( "%-15s : %-25.25s : %d\n", "TS PES Packet",
                        "PES_load_non_intra_flag", PES_load_non_intra_flag );
            }
            else
            {
                printf("[%d]stream pkt %d : UNKNOWN\n", __LINE__, pStream->packet_counter );

                // reset PES header helper structs -- we're done with this one
                memset( pStream->ts_stream_elem[stream_loop].PES_hdr, 0, TS_PES_HDR_BUFSIZE );
                pStream->ts_stream_elem[stream_loop].PES_hdr_len = 0;

                perror("Unhandled PES header");
                // return(0);
                break;
            }
        }
    }

    printf("[%d]stream pkt %d : done with byte loop : PES_hdr_len %d \n", __LINE__, pStream->packet_counter, pStream->ts_stream_elem[stream_loop].PES_hdr_len );

    // OK, enough MPEG parsing -- let's do some decryption
    if ( pStream->ts_header.transport_scrambling_control )
    {
        // turn off crypto bits in TS header
        unsigned char * pTsHeader = pStream->pPacket;
        pTsHeader += 3;
        *pTsHeader &= ~0xC0;

        if ( do_header(&pStream->ts_stream_elem[stream_loop].turing_stuff.key[0],
                &(pStream->ts_stream_elem[stream_loop].turing_stuff.block_no), NULL,
                &(pStream->ts_stream_elem[stream_loop].turing_stuff.crypted), NULL, NULL) )
        {
            perror("do_header did not return 0!\n");
            return(-1);
        }

        prepare_frame( turing, pStream->ts_stream_elem[stream_loop].stream_id,
            pStream->ts_stream_elem[stream_loop].turing_stuff.block_no);

        decrypt_buffer( turing, pPtr, TS_FRAME_SIZE - ((int)(pPtr - pStream->pPacket)) );

        VVERBOSE("---Decrypted transport packet\n");
        if (IS_VVERBOSE)
            hexbulk( (unsigned char *)pStream->pPacket, TS_FRAME_SIZE );
    }

    return(0);
}


int process_ts_frame(turing_state * turing, OFF_T_TYPE packet_start, void * packet_stream, read_func_t read_handler, void * ofh, write_func_t write_handler)
{
    static union
    {
        td_uint64_t align;
        unsigned char packet_buffer[TS_FRAME_SIZE + sizeof(td_uint64_t) + 2];
    } aligned_buf;

    static int tsStreamInit = 0;
    static TS_Stream tsStream;

    int looked_ahead = 0;
    int err = 0;
    int old_verbose_level = o_verbose;

    if ( !tsStreamInit )
    {
        memset( &tsStream, 0, sizeof(TS_Stream) );
        tsStream.pPacket        = &aligned_buf.packet_buffer[sizeof(td_uint64_t)];
        tsStream.initial_offset = packet_start;
        tsStreamInit            = 1;
    }

    tsStream.packet_counter++;

    if ( o_ts_pkt_dump && (o_ts_pkt_dump!=tsStream.packet_counter) )
    {
        old_verbose_level = o_verbose;
        o_verbose = 0;
    }

    LOOK_AHEAD(packet_stream, &aligned_buf.packet_buffer[sizeof(td_uint64_t)], TS_FRAME_SIZE);

    if ( ts_fill_headers( &tsStream ) )
    {
        perror("Failed to fill TS headers");
        o_verbose = old_verbose_level;
        return (-1);
    }

    VVVERBOSE("\n ==================================== \n" );
    if (IS_VVVERBOSE)
    {
        hexbulk( tsStream.pPacket, TS_FRAME_SIZE );
        ts_dump_headers( packet_start, &tsStream );
    }

    switch ( tsStream.ts_packet_type )
    {
        case TS_PID_TYPE_RESERVED :
        case TS_PID_TYPE_NULL_PACKET :
        case TS_PID_TYPE_PROGRAM_MAP_TABLE :
        case TS_PID_TYPE_CONDITIONAL_ACCESS_TABLE :
        case TS_PID_TYPE_NETWORK_INFORMATION_TABLE :
        case TS_PID_TYPE_SERVICE_DESCRIPTION_TABLE :
        case TS_PID_TYPE_EVENT_INFORMATION_TABLE :
        case TS_PID_TYPE_RUNNING_STATUS_TABLE :
        case TS_PID_TYPE_TIME_DATE_TABLE :
        {
            break;
        }
        case TS_PID_TYPE_PROGRAM_ASSOCIATION_TABLE :
        {
            err = ts_handle_pat( &tsStream, turing );
            if ( err )
            {
                perror("ts_handle_pat failed");
            }
            break;
        }
        case TS_PID_TYPE_AUDIO_VIDEO_PRIVATE_DATA :
        {
            if ( tsStream.ts_header.pid == tsStream.ts_pat.program_map_pid )
            {
                err = ts_handle_pmt( &tsStream, turing );
                if ( err )
                {
                    perror("ts_handle_pmt failed");
                }
            }
            else
            {
                unsigned int i              = 0;
                unsigned int is_tivo_pkt    = 0;

                for ( i=0; i<TS_STREAM_ELEMENT_MAX; i++ )
                {
                    if ( ( tsStream.ts_stream_elem[i].stream_pid == tsStream.ts_header.pid ) &&
                         ( tsStream.ts_stream_elem[i].stream_type == TS_STREAM_TYPE_PRIVATE_DATA ) )
                    {
                        is_tivo_pkt = 1;
                        break;
                    }
                }

                if ( is_tivo_pkt )
                {
                    err = ts_handle_tivo_private_data( &tsStream, turing );
                    if ( err )
                    {
                        perror("ts_handle_tivo_private_data failed");
                    }
                }
                else
                {
                    err = ts_handle_audio_video( &tsStream, turing );
                    if ( err )
                    {
                        perror("ts_handle_audio_video failed");
                    }
                }
            }
            break;
        }
        default :
        {
            perror( "Unknown Packet Type" );
            o_verbose = old_verbose_level;
            return (-1);
        }
    }

    if ( err )
    {
        o_verbose = old_verbose_level;
        return(err);
    }

    if (write_handler(aligned_buf.packet_buffer + sizeof(td_uint64_t), TS_FRAME_SIZE, ofh) != TS_FRAME_SIZE)
    {
        o_verbose = old_verbose_level;
        perror ("writing buffer");
        return (-1);
    }

    o_verbose = old_verbose_level;
    return looked_ahead;
}

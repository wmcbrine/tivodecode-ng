/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#include <algorithm>
#include <cstdio>
#include <iostream>

#include "hexlib.hxx"
#include "tivo_parse.hxx"
#include "tivo_decoder_ps.hxx"

extern int o_verbose;
extern bool o_no_verify;

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

TiVoDecoderPS::TiVoDecoderPS(
        TuringState &pTuringState,
        HappyFile *pInfile,
        HappyFile *pOutfile) :
    TiVoDecoder(pTuringState,
        pInfile,
        pOutfile)
{
    marker = 0xFFFFFFFF;
}

TiVoDecoderPS::~TiVoDecoderPS()
{
}

bool TiVoDecoderPS::process()
{
    if (false == isValid)
    {
        VERBOSE("PS Process : not valid\n");
        return false;
    }

    bool first = true;
    uint8_t byte = 0x00;

    while (running)
    {
        if ((marker & 0xFFFFFF00) == 0x100)
        {
            int64_t position = pFileIn->tell();
            int ret = process_frame(byte, position);

            if (ret == 1)
                marker = 0xFFFFFFFF;
            else if (ret == 0)
                pFileOut->write(&byte, 1);
            else if (ret < 0)
            {
                std::perror("processing frame");
                return false;
            }
        }
        else if (!first)
            pFileOut->write(&byte, 1);

        marker <<= 8;

        if (pFileIn->read(&byte, 1) == 0)
        {
            VERBOSE("End of File\n");
            running = false;
        }
        else
            marker |= byte;

        first = false;
    }

    VERBOSE("PS Process\n");
    return true;
}

int TiVoDecoderPS::process_frame(uint8_t code, int64_t packet_start)
{
    static uint8_t packet_buffer[65536 + sizeof(uint64_t) + 2];
    uint8_t bytes[32];
    int looked_ahead = 0;
    int i;
    int scramble = 0;
    int header_len = 0;
    int length;

    for (i = 0; packet_tags[i].packet != PACK_NONE; i++)
    {
        if (code >= packet_tags[i].code_match_lo &&
            code <= packet_tags[i].code_match_hi)
        {
            if (packet_tags[i].packet == PACK_PES_SIMPLE ||
                packet_tags[i].packet == PACK_PES_COMPLEX)
            {
                if (packet_tags[i].packet == PACK_PES_COMPLEX)
                {
                    int retval = pFileIn->read(bytes + looked_ahead,
                                                   5 - looked_ahead);
                    if (retval == 0)
                        return 0;
                    else if (retval != 5 - looked_ahead)
                    {
                        std::perror("read");
                        return -1;
                    }
                    else
                        looked_ahead = 5;

                    // packet_length is 0 and 1
                    // PES header variables
                    // |    2        |    3         |   4   |
                    //  76 54 3 2 1 0 76 5 4 3 2 1 0 76543210
                    //  10 scramble   pts/dts    pes_crc
                    //        priority   escr      extension
                    //          alignment  es_rate   header_data_length
                    //            copyright  dsm_trick
                    //              copy       addtl copy

                    if (!(bytes[2] & 0x80))
                    {
                        if (IS_VERBOSE())
                            std::cerr << "PES (" << code
                                      << ") no header mark ! "
                                      << "(is this an MPEG2-PS file?)\n";
                    }

                    scramble = bytes[2] & 0x30;

                    header_len = 5 + bytes[4];

                    if (scramble == 0x30)
                    {
                        if (bytes[3] & 0x1)
                        {
                            int off = 6;
                            int ext_byte = 5;
                            int goagain = 0;
                            // extension
                            if (header_len > 32)
                                return -1;

                            int retval = pFileIn->read(bytes + looked_ahead,
                                                  header_len - looked_ahead);
                            if (retval == 0)
                                return 0;
                            else if (retval != header_len - looked_ahead)
                            {
                                std::perror("read");
                                return -1;
                            }
                            else
                                looked_ahead = header_len;

                            do
                            {
                                goagain = 0;

                                //packet seq counter flag
                                if (bytes[ext_byte] & 0x20)
                                    off += 4;

                                //private data flag
                                if (bytes[ext_byte] & 0x80)
                                {
                                    int block_no = 0;
                                    int crypted  = 0;

                                    VVERBOSE("\n\n---Turing : Key\n");
                                    if (IS_VVERBOSE())
                                    {
                                        hexbulk( (uint8_t *)&bytes[off], 16 );
                                        std::cerr << "---Turing : header : block "
                                                  << block_no << " crypted "
                                                  << crypted << "\n";
                                    }

                                    if (do_header(&bytes[off], block_no, crypted))
                                    {
                                        VERBOSE("do_header did not return 0!\n");
                                    }

                                    if (IS_VVERBOSE())
                                        std::cerr << "BBB : code " << code
                                                  << ", blockno " << block_no
                                                  << ", crypted " << crypted
                                                  << "\n";
                                    if (IS_VERBOSE())
                                        std::cerr << packet_start
                                                  << " : stream_no: " << code
                                                  << ", block_no: " << block_no
                                                  << "\n";
                                    if (IS_VVERBOSE())
                                        std::cerr << "---Turing : prepare : code "
                                                  << code << " block_no "
                                                  << block_no << "\n";

                                    pTuring.prepare_frame(code, block_no);

                                    if (IS_VVERBOSE())
                                    {
                                        std::cerr << "CCC : code " << code
                                                  << ", blockno " << block_no
                                                  << ", crypted " << crypted
                                                  << "\n"
                                                  << "---Turing : decrypt : crypted "
                                                  << crypted << " len 4\n";
                                    }

                                    pTuring.active->decrypt_buffer((uint8_t *)&crypted, 4);

                                    if (IS_VVERBOSE())
                                    {
                                        std::cerr << "DDD : code " << code
                                                  << ", blockno " << block_no
                                                  << ", crypted " << crypted
                                                  << "\n";
                                    }
                                }

                                // STD buffer flag
                                if (bytes[ext_byte] & 0x10)
                                    off += 2;

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
                    int retval = pFileIn->read(bytes + looked_ahead,
                                                   2 - looked_ahead);
                    if (retval == 0)
                        return 0;
                    else if (retval != 2 - looked_ahead)
                    {
                        std::perror("read");
                        return -1;
                    }
                    else
                        looked_ahead = 2;
                }

                length = bytes[1] | (bytes[0] << 8);

                std::copy(bytes, bytes + looked_ahead,
                          packet_buffer + sizeof(uint64_t));

                int retval = pFileIn->read(packet_buffer + sizeof(uint64_t) +
                                   looked_ahead, length + 2 - looked_ahead);
                if (retval == 0)
                    return 0;
                else if (retval != length + 2 - looked_ahead)
                {
                    std::perror("read");
                    return -1;
                }
                else
                    looked_ahead = length + 2;

                uint8_t *packet_ptr = packet_buffer + sizeof(uint64_t);
                size_t packet_size;

                packet_buffer[sizeof(uint64_t) - 1] = code;

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

                if (scramble == 0x30)
                {
                    if (IS_VVERBOSE())
                        std::cerr << "---Turing : decrypt : size "
                                  << packet_size << "\n";

                    pTuring.active->decrypt_buffer(packet_ptr, packet_size);

                    // turn off scramble bits
                    packet_buffer[sizeof(uint64_t) + 2] &= ~0x30;

                    // scan video buffer for Slices.  If no slices are
                    // found, the MAK is wrong.
                    if (!o_no_verify && code == 0xe0) {
                        int slice_count = 0;
                        size_t offset;

                        for (offset = sizeof(uint64_t);
                             offset + 4 < packet_size; offset++)
                        {
                            if (packet_buffer[offset] == 0x00 &&
                                packet_buffer[offset + 1] == 0x00 &&
                                packet_buffer[offset + 2] == 0x01 &&
                                packet_buffer[offset + 3] >= 0x01 &&
                                packet_buffer[offset + 3] <= 0xAF)
                            {
                                slice_count++;
                            }
                            // choose 8 as a good test that if 8 slices
                            // are seen, it's probably not random noise
                            if (slice_count > 8)
                            {
                                // disable future verification
                                o_no_verify = true;
                            }
                        }
                        if (!o_no_verify)
                        {
                            VERBOSE("Invalid MAK -- aborting\n");
                            return -2;
                        }
                    }
                }
                else if (code == 0xbc)
                {
                    // don't know why, but tivo dll does this.
                    // I can find no good docs on the format of the
                    // program_stream_map but I think this clears a
                    // reserved bit.  No idea why
                    packet_buffer[sizeof(uint64_t) + 2] &= ~0x20;
                }

                if (pFileOut->write(packet_buffer +
                                sizeof(uint64_t) - 1, length + 3) !=
                    (size_t)(length + 3))
                {
                    std::perror("writing buffer");
                }

                return 1;
            }

            return 0;
        }
    }

    return -1;
}

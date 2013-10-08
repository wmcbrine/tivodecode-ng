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
#include "tivo_decoder_ps.hxx"

extern int o_verbose;
extern int o_no_verify;

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
        turing_state * pTuringState, 
        happy_file * pInfile, 
        hoff_t fileOffset, 
        FILE * pOutfile, 
        read_func_t readFunc, 
        write_func_t writeFunc) : 
    TiVoDecoder(pTuringState, 
        pInfile, 
        fileOffset, 
        pOutfile, 
        readFunc, 
        writeFunc)
{
    marker = 0xFFFFFFFF;
}


TiVoDecoderPS::~TiVoDecoderPS()
{

}


BOOL TiVoDecoderPS::process()
{
    if(FALSE==isValid)
    {
        VERBOSE("PS Process : not valid\n");
        return(FALSE);
    }

    BOOL first   = TRUE;
    UINT8 byte   = 0x00;
    
    while(running)
    {
        if((marker & 0xFFFFFF00) == 0x100)
        {
            hoff_t position = htell(pFileIn);           
            int ret = process_frame(byte, position);

            if(ret == 1)
            {
                marker = 0xFFFFFFFF;
            }
            else if(ret == 0)
            {
                fwrite(&byte, 1, 1, pFileOut);
            }
            else if(ret < 0)
            {
                perror("processing frame");
                return 10;
            }
        }
        else if(!first)
        {
            fwrite(&byte, 1, 1, pFileOut);
        }
        
        marker <<= 8;
        
        if(hread(&byte, 1, pFileIn) == 0)
        {
            VERBOSE( "End of File\n");
            running = FALSE;
        }
        else
        {
            marker |= byte;
        }
        
        first = FALSE;
    }    

    VERBOSE("PS Process\n");
    return(TRUE);    
}



int TiVoDecoderPS::process_frame(UINT8 code, hoff_t packet_start)
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
                    LOOK_AHEAD(pFileIn, bytes, 5);

                    // packet_length is 0 and 1
                    // PES header variables
                    // |    2        |    3         |   4   |
                    //  76 54 3 2 1 0 76 5 4 3 2 1 0 76543210
                    //  10 scramble   pts/dts    pes_crc
                    //        priority   escr      extension
                    //          alignment  es_rate   header_data_length
                    //            copyright  dsm_trick
                    //              copy       addtl copy

                    if ((bytes[2]>>6) != 0x2) 
                    {
                        VERBOSE( "PES (0x%02X) header mark != 0x2: 0x%x (is this an MPEG2-PS file?)\n",
                            code,(bytes[2]>>6));
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

                            LOOK_AHEAD (pFileIn, bytes, header_len);

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
                                    VERBOSE("%Zu : stream_no: %x, block_no: %d\n", (size_t)packet_start, code, block_no);
                                    VERBOSE("---Turing : prepare : code 0x%02x block_no %d\n", code, block_no );

                                    prepare_frame(pTuring, code, block_no);

                                    VERBOSE("CCC : code 0x%02x, blockno %d, crypted 0x%08x\n", code, block_no, crypted );
                                    VERBOSE("---Turing : decrypt : crypted 0x%08x len %d\n", crypted, 4 );

                                    decrypt_buffer(pTuring, (unsigned char *)&crypted, 4);

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
                    LOOK_AHEAD (pFileIn, bytes, 2);
                }

                length = bytes[1] | (bytes[0] << 8);

                memcpy (aligned_buf.packet_buffer + sizeof(td_uint64_t), bytes, looked_ahead);

                LOOK_AHEAD (pFileIn, aligned_buf.packet_buffer + sizeof(td_uint64_t), length + 2);
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

                        decrypt_buffer(pTuring, packet_ptr, packet_size);

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

                    if (write_handler(aligned_buf.packet_buffer + sizeof(td_uint64_t) - 1, length + 3, pFileOut) != length + 3)
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

/* vi:set ai ts=4 sw=4 expandtab: */


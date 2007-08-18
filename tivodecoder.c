/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 *
 * derived from mpegcat, copyright 2006 Kees Cook, used with permission
 */
#include "tivodecoder.h"
#include "Turing.h"
#include <stdio.h>
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif

/* TODO: clean this up */
extern int o_verbose;
extern int o_no_verify;

typedef enum
{
    PACK_NONE,
    PACK_SPECIAL,
    PACK_PES_SIMPLE,            // packet length == data length
    PACK_PES_COMPLEX,           // crazy headers need skipping
}
packet_type;

typedef struct
{
    // the byte value match for the packet tags
    unsigned char code_match_lo;      // low end of the range of matches
    unsigned char code_match_hi;      // high end of the range of matches

    // what kind of PES is it?
    packet_type packet;
}
packet_tag_info;

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
    if (read_handler((bytes) + looked_ahead, (n) - looked_ahead, fh) != (n) - looked_ahead) { \
        perror ("read"); \
        return -1; \
    } else { \
        looked_ahead = (n); \
    } \
} while (0)

/*
 * called for each frame
 */
int process_frame(unsigned char code, turing_state * turing, OFF_T_TYPE packet_start, void * packet_stream, read_func_t read_handler, void * ofh, write_func_t write_handler)
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
                        fprintf(stderr, "PES (0x%02X) header mark != 0x2: 0x%x (is this an MPEG2-PS file?)\n",code,(bytes[2]>>6));
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
                                    int block_no, crypted;

                                    if (do_header (&bytes[off], &block_no, NULL, &crypted, NULL, NULL))
                                    {
                                        fprintf(stderr, "do_header not returned 0!\n");
                                    }

                                    if (o_verbose)
                                        fprintf(stderr, "%10" OFF_T_FORMAT ": stream_no: %x, block_no: %d\n", packet_start, code, block_no);

                                    prepare_frame(turing, code, block_no);
                                    decrypt_buffer(turing, (unsigned char *)&crypted, 4);
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
                        decrypt_buffer (turing, packet_ptr, packet_size);
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
                                fprintf(stderr, "Invalid MAK -- aborting\n");
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


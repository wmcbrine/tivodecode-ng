#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "getopt_long.h"
#include "happyfile.h"
#include "tivo-parse.h"
#include "turing_stream.h"

#if _FILE_OFFSET_BITS==64 || defined(__NetBSD__)
# define OFF_T_FORMAT  "llu"
# define ATOL(arg)     atoll(arg)
#else
# warning "Not compiling for large file (>2G) support!"
# define OFF_T_FORMAT  "lu"
# define ATOL(arg)     atol(arg)
#endif

int o_verbose = 0;

happy_file * hfh=NULL;
// file position options
off_t begin_at = 0;


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

packet_tag_info packet_tags[] = {
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
int do_header(BYTE * arg_0, int * block_no, int * arg_8, int * crypted, int * arg_10, int * arg_14)
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
    if (hread((bytes) + looked_ahead, (n) - looked_ahead, fh) != (n) - looked_ahead) { \
        perror ("read"); \
        return -1; \
    } else { \
        looked_ahead = (n); \
    } \
} while (0)

/*
 * called for each frame
 */
int process_frame(unsigned char code, turing_state * turing, FILE * ofh)
{
    static char packet_buffer[65536 + 3];
    unsigned char bytes[32];
    int looked_ahead = 0;
    int i;
    int scramble=0;
    unsigned int header_len = 0;
    unsigned int length;
    off_t packet_start = htell(hfh);

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
                    LOOK_AHEAD (hfh, bytes, 5);

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

                    if ((code == 0xe0 || code == 0xc0) && scramble == 3)
                    {
                        if (bytes[3] & 0x1)
                        {
                            int off = 6;
                            int ext_byte = 5;
                            int goagain = 0;
                            // extension
                            if (header_len > 32)
                                return -1;

                            LOOK_AHEAD (hfh, bytes, header_len);

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
                                    decrypt_buffer(turing, (char *)&crypted, 4);
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
                    LOOK_AHEAD (hfh, bytes, 2);
                }

                length = bytes[1] | (bytes[0] << 8);

                memcpy (packet_buffer + 1, bytes, looked_ahead);

                LOOK_AHEAD (hfh, packet_buffer + 1, length + 2);
                {
                    char * packet_ptr = packet_buffer + 1;
                    size_t packet_size;

                    packet_buffer[0] = code;

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

                    if ((code == 0xe0 || code == 0xc0) && scramble == 3)
                    {
                        decrypt_buffer (turing, packet_ptr, packet_size);
                        // turn off scramble bits
                        packet_buffer[1+2] &= ~0x30;
                    }
                    else if (code == 0xbc)
                    {
                        // don't know why, but tivo dll does this.
                        // I can find no good docs on the format of the program_stream_map
                        // but I think this clears a reserved bit.  No idea why
                        packet_buffer[1+2] &= ~0x20;
                    }

                    if (fwrite(packet_buffer, 1, length + 3, ofh) != length + 3)
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

static struct option long_options[] = {
    {"mak", 1, 0, 'm'},
    {"out", 1, 0, 'o'},
    {"help", 0, 0, 'h'},
    {"verbose", 0, 0, 'v'},
    {0, 0, 0, 0}
};

static void do_help(char * arg0, int exitval)
{
    fprintf(stderr, "Usage: %s [--help] [--verbose|-v] {--mak|-m} mak [{--out|-o} outfile] <tivofile>\n\n", arg0);
#define ERROUT(s) fprintf(stderr, s)
    ERROUT ("  --mak, -m        media access key (required)\n");
    ERROUT ("  --out, -o        output file (default stdout)\n");
    ERROUT ("  --verbose, -v    verbose\n");
    ERROUT ("  --help           print this help and exit\n\n");
    ERROUT ("The file names specified for the output file or the tivo file may be -, which\n");
    ERROUT ("means stdout or stdin respectively\n\n");
#undef ERROUT

    exit (exitval);
}


int main(int argc, char *argv[])
{
    unsigned int marker;
    unsigned char byte;
    char first = 1;

    int running = 1;

    char * tivofile = NULL;
    char * outfile = NULL;
    char mak[12];

    int makgiven = 0;

    turing_state turing;

    FILE * ofh;

    memset(&turing, 0, sizeof(turing));

    fprintf(stderr, "Encryption by QUALCOMM ;)\n\n");

    while (1)
    {
        int c = getopt_long (argc, argv, "m:o:hv", long_options, 0);

        if (c == -1)
            break;

        switch (c)
        {
            case 'm':
                strncpy(mak, optarg, 11);
                mak[11] = '\0';
                makgiven = 1;
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'h':
                do_help(argv[0], 1);
                break;
            case 'v':
                o_verbose = 1;
                break;
            case '?':
                do_help(argv[0], 2);
                break;
            default:
                do_help(argv[0], 3);
                break;
        }
    }

    if (optind < argc)
    {
        tivofile=argv[optind++];
        if (optind < argc)
            do_help(argv[0], 4);
    }

    if (!makgiven || !tivofile)
    {
        do_help(argv[0], 5);
    }

    if (!strcmp(tivofile, "-"))
    {
        hfh=hattach(stdin);
    }
    else
    {
        if (!(hfh=hopen(tivofile, "rb")))
        {
            perror(tivofile);
            return 6;
        }
    }

    if (!outfile || !strcmp(outfile, "-"))
    {
        ofh = stdout;
    }
    else
    {
        if (!(ofh = fopen(outfile, "wb")))
        {
            perror("opening output file");
            return 7;
        }
    }

    if ((begin_at = setup_turing_key (&turing, hfh, mak)) < 0)
    {
        return 8;
    }

    if (hseek(hfh, begin_at, SEEK_SET) < 0)
    {
        perror ("seek");
        return 9;
    }

    marker = 0xFFFFFFFF;
    while (running)
    {
        if ((marker & 0xFFFFFF00) == 0x100)
        {
            int ret = process_frame(byte, &turing, ofh);
            if (ret == 1)
            {
                marker = 0xFFFFFFFF;
            }
            else if (ret == 0)
            {
                fwrite(&byte, 1, 1, ofh);
            }
            else if (ret < 0)
            {
                perror ("processing frame");
                return 10;
            }
        }
        else if (!first)
        {
            fwrite(&byte, 1, 1, ofh);
        }
        marker <<= 8;
        if (hread(&byte, 1, hfh) == 0)
        {
            fprintf(stderr, "End of File\n");
            running = 0;
        }
        else
            marker |= byte;
        first = 0;
    }

    destruct_turing (&turing);

    if (hfh->fh == stdin)
        hdetach(hfh);
    else
        hclose(hfh);

    if (ofh != stdout)
        fclose(ofh);

    return 0;
}

/* vi:set ai ts=4 sw=4 expandtab: */

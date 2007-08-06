/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 *
 * derived from mpegcat, copyright 2006 Kees Cook, used with permission
 */
#include "tivodecoder.h"
#include <stdio.h>
#include <stddef.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif
#include "getopt_long.h"
#include "happyfile.h"
#include "cli_common.h"

static int hread_wrapper (void * mem, int size, void * fh)
{
    return (int)hread (mem, size, (happy_file *)fh);
}

static struct option long_options[] = {
    {"mak", 1, 0, 'm'},
    {"out", 1, 0, 'o'},
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'V'},
    {"chunk-1", 0, 0, '1'},
    {"chunk-2", 0, 0, '2'},
    {0, 0, 0, 0}
};

static void do_help(char * arg0, int exitval)
{
    fprintf(stderr, "Usage: %s [--help] {--mak|-m} mak [{--out|-o} outfile] [{-2|--chunk-2}] <tivofile>\n\n", arg0);
#define ERROUT(s) fprintf(stderr, s)
    ERROUT ("  --mak, -m          media access key (required)\n");
    ERROUT ("  --out, -o          output file (default stdout)\n");
    ERROUT ("  --chunk-1, -1      output chunk 1 (default if neither chunk specified)\n");
    ERROUT ("  --chunk-2, -2      output chunk 2\n");
    ERROUT ("  --version, -V      print the version information and exit\n");
    ERROUT ("  --help, -h         print this help and exit\n\n");
    ERROUT ("The file names specified for the output file or the tivo file may be -, which\n");
    ERROUT ("means stdout or stdin respectively\n\n");
#undef ERROUT

    exit (exitval);
}

int main(int argc, char *argv[])
{
    int o_chunk_1 = 0;
    int o_chunk_2 = 0;

    char * tivofile = NULL;
    char * outfile = NULL;
    char mak[12];

    int makgiven = 0;

    FILE * ofh;
    happy_file * hfh=NULL;
    turing_state metaturing;

    tivo_stream_header head;
    tivo_stream_chunk *chunk;
    hoff_t current_meta_stream_pos = 0;
    int i;

    memset(mak, 0, sizeof(mak));
    memset(&metaturing, 0, sizeof(metaturing));

    while (1)
    {
        int c = getopt_long (argc, argv, "m:o:12hV", long_options, 0);

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
            case '1':
                o_chunk_1 = 1;
                break;
            case '2':
                o_chunk_2 = 1;
                break;
            case 'h':
            case '?':
                do_help(argv[0], 1);
                break;
            case 'V':
                do_version(10);
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

    if (!makgiven)
        makgiven = get_mak_from_conf_file(mak);

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

    if (!o_chunk_1 && !o_chunk_2)
        o_chunk_1 = 1;

    PRINT_QUALCOMM_MSG();

    /* parse the tivo headers manually here, since we care about more
     * than the init_turing_from_file function will get
     */

    if (read_tivo_header (hfh, &head, &hread_wrapper))
        return 8;

    for (i = 0; i < head.chunks; i++)
    {
        hoff_t chunk_start = htell(hfh) + SIZEOF_STREAM_CHUNK;

        if ((chunk = read_tivo_chunk (hfh, &hread_wrapper)) == NULL)
            return 8;

        if (chunk->data_size && chunk->type == TIVO_CHUNK_XML)
        {
            setup_metadata_key (&metaturing, chunk, mak);
            free (chunk);
            continue;
        }

        if ((o_chunk_1 && chunk->id == 1) || (o_chunk_2 && chunk->id == 2))
        {
            prepare_frame(&metaturing, 0, 0);
            skip_turing_data(&metaturing, (size_t)(chunk_start - current_meta_stream_pos));
            decrypt_buffer(&metaturing, chunk->data, chunk->data_size);
            current_meta_stream_pos = chunk_start + chunk->data_size;

            if (fwrite (chunk->data, 1, chunk->data_size, ofh) != chunk->data_size)
            {
                perror("write chunk");
                return 8;
            }
        }

        free(chunk);
    }

    destruct_turing(&metaturing);

    if (hfh->fh == stdin)
        hdetach(hfh);
    else
        hclose(hfh);

    if (ofh != stdout)
        fclose(ofh);

    return 0;
}

/* vi:set ai ts=4 sw=4 expandtab: */

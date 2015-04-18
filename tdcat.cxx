/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 *
 * derived from mpegcat, copyright 2006 Kees Cook, used with permission
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

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "getopt_long.h"
#include "happyfile.h"

#include "tivo_parse.hxx"
#include "tivo_decoder_ts.hxx"
#include "tivo_decoder_ps.hxx"

static struct option long_options[] = {
    {"mak", 1, 0, 'm'},
    {"out", 1, 0, 'o'},
    {"version", 0, 0, 'V'},
    {"help", 0, 0, 'h'},
    {"chunk-1", 0, 0, '1'},
    {"chunk-2", 0, 0, '2'},
    {0, 0, 0, 0}
};

static void do_version(int exitval)
{
    fprintf(stderr, "%s\n", PACKAGE_STRING);
    fprintf(stderr, "Copyright (c) 2006-2007, Jeremy Drake\n");
    fprintf(stderr, "See COPYING file in distribution for details\n\n");
    PRINT_QUALCOMM_MSG();
    exit (exitval);
}

#ifdef WIN32
#   define HOME_ENV_NAME "USERPROFILE"
#   define DEFAULT_EMPTY_HOME "C:"
#else
#   define HOME_ENV_NAME "HOME"
#   define DEFAULT_EMPTY_HOME ""
#endif

static const char MAK_DOTFILE_NAME[] = "/.tivodecode_mak";

int get_mak_from_conf_file(char * mak)
{
    char * mak_fname = NULL;
    FILE * mak_file = NULL;
    const char * home_dir = getenv(HOME_ENV_NAME);
    size_t home_dir_len;

    if (!home_dir)
        home_dir = DEFAULT_EMPTY_HOME;

    home_dir_len = strlen(home_dir);

    mak_fname = (char *) malloc(home_dir_len + sizeof(MAK_DOTFILE_NAME));
    if (!mak_fname)
    {
        fprintf(stderr, "error allocing string for mak config file name\n");
        goto fail;
    }

    memcpy (mak_fname, home_dir, home_dir_len);
    memcpy (mak_fname + home_dir_len, MAK_DOTFILE_NAME, sizeof(MAK_DOTFILE_NAME));

    if ((mak_file = fopen(mak_fname, "r")))
    {
        if (fread(mak, 1, 11, mak_file) >= 10)
        {
            int i;
            for (i = 11; i >= 10 && (mak[i] == '\0' || isspace((int)mak[i])); --i)
            {
                mak[i] = '\0';
            }
        }
        else if (ferror(mak_file))
        {
            perror ("reading mak config file");
            goto fail;
        }
        else
        {
            fprintf(stderr, "mak too short in mak config file\n");
            goto fail;
        }

        fclose (mak_file);
        mak_file = NULL;
    }
    else
        goto fail;

    free(mak_fname);
    mak_fname = NULL;
    return 1;

fail:
    if (mak_file)
        fclose(mak_file);
    if (mak_fname)
        free(mak_fname);
    return 0;
}


static void do_help(char * arg0, int exitval)
{
    fprintf(stderr, "Usage: %s [--help] {--mak|-m} mak [--chunk-1|-1] [--chunk-2|-2][{--out|-o} outfile] <tivofile>\n\n", arg0);
    fprintf(stderr, " -m, --mak         media access key (required)\n");
    fprintf(stderr, " -o, --out,        output file (default stdout)\n");
    fprintf(stderr, " -V, --version,    print the version information and exit\n");
    fprintf(stderr, " -h, --help,       print this help and exit\n\n");
    fprintf(stderr, " -1, --chunk-1     output chunk 1 (default if neither chunk specified)\n");
    fprintf(stderr, " -2, --chunk-2     output chunk 2\n");
    fprintf(stderr, "The file names specified for the output file or the tivo file may be -, which\n");
    fprintf(stderr, "means stdout or stdin respectively\n\n");
    exit(exitval);
}


int main(int argc, char *argv[])
{
    int o_chunk_1 = 1;
    int o_chunk_2 = 0;

    int ret = 0;
    int makgiven = 0;

    const char * tivofile = NULL;
    const char * outfile  = NULL;

    CHAR mak[12];
    memset(mak, 0, sizeof(mak));

    turing_state turing;
    memset(&turing, 0, sizeof(turing));

    turing_state metaturing;
    memset(&metaturing, 0, sizeof(metaturing));
    hoff_t current_meta_stream_pos = 0;

    FILE * ofh = NULL;
    happy_file * hfh=NULL;

    TiVoStreamHeader header;

    while(1)
    {
        int c = getopt_long(argc, argv, "m:o:Vh12", long_options, 0);

        if(c == -1)
            break;

        switch(c)
        {
            case 'm':
                strncpy(mak, optarg, 11);
                mak[11] = '\0';
                makgiven = 1;
                break;
            case 'o':
                //if the output file is to be stdout then the argv
                //will be null and the next argc will be "-"
                if (optarg == NULL && !strcmp(argv[optind+1], "-"))
                {
                    outfile = "-";
                    optind++;
                }
                else
                    outfile = optarg;
                break;
            case 'h':
                do_help(argv[0], 1);
                break;
            case '?':
                do_help(argv[0], 2);
                break;
            case 'V':
                do_version(10);
                break;
            case '1':
                o_chunk_1 = 1;
                o_chunk_2 = 0;
                break;
            case '2':
                o_chunk_1 = 0;
                o_chunk_2 = 1;
                break;
            default:
                do_help(argv[0], 3);
                break;
        }
    }

    if (!makgiven)
        makgiven = get_mak_from_conf_file(mak);
        
    if(optind < argc)
    {
        tivofile=argv[optind++];
        if(optind < argc)
            do_help(argv[0], 4);
    }

    if(!makgiven || !tivofile)
    {
        do_help(argv[0], 5);
    }

    if(!strcmp(tivofile, "-"))
    {
// JKOZEE-Make sure stdin is set to binary on Windows
#ifdef WIN32
        int result = _setmode(_fileno(stdin), _O_BINARY );
        if( result == -1 ) {
           perror( "Cannot set stdin to binary mode" );
           return 10;
        }
#endif
        hfh=hattach(stdin);
    }
    else
    {
        hfh=hopen(tivofile, "rb");
        if(NULL==hfh)
        {
            perror(tivofile);
            return 6;
        }
    }


    if (!outfile || !strcmp(outfile, "-"))
    {
        // JKOZEE-Make sure stdout is set to binary on Windows
        #ifdef WIN32
        int result = _setmode(_fileno(stdout), _O_BINARY );
        if( result == -1 ) {
           perror( "Cannot set stdout to binary mode" );
           return 10;
        }
        #endif
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

    if(FALSE==header.read(hfh, &hread_wrapper))
    {
        return(8);
    }

    header.dump();

    TiVoStreamChunk * pChunks = new TiVoStreamChunk[header.chunks];
    if(NULL == pChunks)
    {
        perror("allocate TiVoStreamChunks");
        return(9);
    }

    for(INT32 i=0; i<header.chunks; i++)
    {
        hoff_t chunk_start = htell(hfh) + pChunks[i].size();

        if(FALSE==pChunks[i].read(hfh, &hread_wrapper))
        {
            perror("chunk read fail");
            return(8);
        }

        if(TIVO_CHUNK_PLAINTEXT_XML==pChunks[i].type)
        {
            pChunks[i].setupTuringKey(&turing, (UINT8*)mak);
            pChunks[i].setupMetadataKey(&metaturing, (UINT8*)mak);
        }
        else if(TIVO_CHUNK_ENCRYPTED_XML==pChunks[i].type)
        {
            UINT16 offsetVal = chunk_start - current_meta_stream_pos;
            pChunks[i].decryptMetadata(&metaturing, offsetVal);
            current_meta_stream_pos = chunk_start + pChunks[i].dataSize;
        }
        else
        {
            perror("Unknown chunk type");
            return(8);
        }

        if ((o_chunk_1 && pChunks[i].id == 1) ||
            (o_chunk_2 && pChunks[i].id == 2))
        {
            pChunks[i].dump();
            if(FALSE==pChunks[i].write(ofh, fwrite_wrapper))
            {
                perror("write chunk");
                return 8;
            }
        }
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

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
#include <iostream>

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

#include "cli_common.hxx"
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

static void do_help(const char *arg0, int exitval)
{
    std::cerr << "Usage: " << arg0 << " [--help] {--mak|-m} mak [--chunk-1|-1]";
    std::cerr << " [--chunk-2|-2][{--out|-o} outfile] <tivofile>\n\n";
    std::cerr << " -m, --mak         media access key (required)\n";
    std::cerr << " -o, --out,        output file (default stdout)\n";
    std::cerr << " -V, --version,    print the version information and exit\n";
    std::cerr << " -h, --help,       print this help and exit\n\n";
    std::cerr << " -1, --chunk-1     output chunk 1 (default if unspecified)\n";
    std::cerr << " -2, --chunk-2     output chunk 2\n";
    std::cerr << "The file names specified for the output file or the tivo ";
    std::cerr << "file may be -, which\n";
    std::cerr << "means stdout or stdin respectively\n\n";
    exit(exitval);
}

int main(int argc, char *argv[])
{
    int o_chunk_1 = 1;
    int o_chunk_2 = 0;

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

    if(FALSE==header.read(hfh))
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

        if(FALSE==pChunks[i].read(hfh))
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
            if(FALSE==pChunks[i].write(ofh))
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

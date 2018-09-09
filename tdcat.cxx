/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 *
 * derived from mpegcat, copyright 2006 Kees Cook, used with permission
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "getopt_td.hxx"
#include "cli_common.hxx"
#include "tivo_parse.hxx"
#include "tivo_decoder_ts.hxx"
#include "tivo_decoder_ps.hxx"

static struct td_option long_options[] = {
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
    std::cerr << "Usage: " << arg0 << " [--help] {--mak|-m} mak [--chunk-1|-1]"
        " [--chunk-2|-2][{--out|-o} outfile] <tivofile>\n\n"
        " -m, --mak         media access key (required)\n"
        " -o, --out,        output file (default stdout)\n"
        " -V, --version,    print the version information and exit\n"
        " -h, --help,       print this help and exit\n\n"
        " -1, --chunk-1     output chunk 1 (default if unspecified)\n"
        " -2, --chunk-2     output chunk 2\n\n"
        "The file name specified for the tivo file may be -, which means "
        "stdin.\n\n";
    std::exit(exitval);
}

int main(int argc, const char **argv)
{
    int o_chunk_1 = 1;
    int o_chunk_2 = 0;

    int makgiven = 0;

    const char *tivofile = NULL;
    const char *outfile  = NULL;

    char mak[12];
    std::memset(mak, 0, sizeof(mak));

    TuringState turing;
    std::memset(&turing, 0, sizeof(turing));

    TuringState metaturing;
    std::memset(&metaturing, 0, sizeof(metaturing));
    hoff_t current_meta_stream_pos = 0;

    HappyFile *hfh = NULL, *ofh = NULL;

    TiVoStreamHeader header;

    while (1)
    {
        int c = getopt_td(argc, argv, "m:o:Vh12", long_options, 0);

        if (c == -1)
            break;

        switch (c)
        {
            case 'm':
                std::strncpy(mak, td_optarg, 11);
                mak[11] = '\0';
                makgiven = 1;
                break;
            case 'o':
                //if the output file is to be stdout then the argv
                //will be null and the next argc will be "-"
                if (td_optarg == NULL && !std::strcmp(argv[td_optind + 1], "-"))
                {
                    outfile = "-";
                    td_optind++;
                }
                else
                    outfile = td_optarg;
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

    if (td_optind < argc)
    {
        tivofile = argv[td_optind++];
        if (td_optind < argc)
            do_help(argv[0], 4);
    }

    if (!makgiven || !tivofile)
    {
        do_help(argv[0], 5);
    }

    hfh = new HappyFile;

    if (!std::strcmp(tivofile, "-"))
    {
        if (!hfh->attach(stdin))
            return 10;
    }
    else
    {
        if (!hfh->open(tivofile, "rb"))
        {
            std::perror(tivofile);
            return 6;
        }
    }

    ofh = new HappyFile;

    if (!outfile || !std::strcmp(outfile, "-"))
    {
        if (!ofh->attach(stdout))
            return 10;
    }
    else
    {
        if (!ofh->open(outfile, "wb"))
        {
            std::perror("opening output file");
            return 7;
        }
    }

    if (!o_chunk_1 && !o_chunk_2)
        o_chunk_1 = 1;

    print_qualcomm_msg();

    if (false == header.read(hfh))
    {
        return 8;
    }

    header.dump();

    TiVoStreamChunk *pChunks = new TiVoStreamChunk[header.chunks];
    if (NULL == pChunks)
    {
        std::perror("allocate TiVoStreamChunks");
        return 9;
    }

    for (int32_t i = 0; i < header.chunks; i++)
    {
        hoff_t chunk_start = hfh->tell() + 12;

        if (false == pChunks[i].read(hfh))
        {
            std::perror("chunk read fail");
            return 8;
        }

        if (TIVO_CHUNK_PLAINTEXT_XML == pChunks[i].type)
        {
            pChunks[i].setupTuringKey(&turing, (uint8_t*)mak);
            pChunks[i].setupMetadataKey(&metaturing, (uint8_t*)mak);
        }
        else if (TIVO_CHUNK_ENCRYPTED_XML == pChunks[i].type)
        {
            uint16_t offsetVal = chunk_start - current_meta_stream_pos;
            pChunks[i].decryptMetadata(&metaturing, offsetVal);
            current_meta_stream_pos = chunk_start + pChunks[i].dataSize;
        }
        else
        {
            std::perror("Unknown chunk type");
            return 8;
        }

        if ((o_chunk_1 && pChunks[i].id == 1) ||
            (o_chunk_2 && pChunks[i].id == 2))
        {
            pChunks[i].dump();
            if (false == pChunks[i].write(ofh))
            {
                std::perror("write chunk");
                return 8;
            }
        }
    }

    metaturing.destruct();

    hfh->close();
    delete hfh;

    ofh->close();
    delete ofh;

    return 0;
}

/* vi:set ai ts=4 sw=4 expandtab: */

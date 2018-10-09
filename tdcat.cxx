/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 *
 * derived from mpegcat, copyright 2006 Kees Cook, used with permission
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "getopt_td.hxx"
#include "cli_common.hxx"
#include "tivo_parse.hxx"

static struct td_option long_options[] = {
    {"mak", true, 'm'},
    {"out", true, 'o'},
    {"version", false, 'V'},
    {"help", false, 'h'},
    {"chunk-1", false, '1'},
    {"chunk-2", false, '2'},
    {0, false, 0}
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
    TuringState metaturing;
    TiVoStreamHeader header;

    bool o_chunk_1 = true;
    bool o_chunk_2 = false;

    const char *tivofile = NULL;
    const char *outfile  = NULL;

    std::string mak = "";

    int64_t current_meta_stream_pos = 0;

    while (true)
    {
        int c = getopt_td(argc, argv, "m:o:Vh12", long_options, 0);

        if (c == -1)
            break;

        switch (c)
        {
            case 'm':
                mak = td_optarg;
                break;
            case 'o':
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
                o_chunk_1 = true;
                o_chunk_2 = false;
                break;
            case '2':
                o_chunk_1 = false;
                o_chunk_2 = true;
                break;
            default:
                do_help(argv[0], 3);
                break;
        }
    }

    if ("" == mak)
        mak = get_mak_from_conf_file();

    if (td_optind < argc)
    {
        tivofile = argv[td_optind++];
        if (td_optind < argc)
            do_help(argv[0], 4);
    }

    if (("" == mak) || !tivofile)
        do_help(argv[0], 5);

    HappyFile *hfh = new HappyFile(tivofile, "rb");
    HappyFile *ofh = new HappyFile(outfile, "wb");

    if (!o_chunk_1 && !o_chunk_2)
        o_chunk_1 = true;

    print_qualcomm_msg();

    if (false == header.read(hfh))
        return 8;

    header.dump();

    TiVoStreamChunk *pChunks = new TiVoStreamChunk[header.chunks];

    for (uint16_t i = 0; i < header.chunks; i++)
    {
        int64_t chunk_start = hfh->tell() + 12;

        if (false == pChunks[i].read(hfh))
        {
            std::perror("chunk read fail");
            return 8;
        }

        if (TIVO_CHUNK_PLAINTEXT_XML == pChunks[i].type)
        {
            pChunks[i].setupMetadataKey(metaturing, mak);
        }
        else if (TIVO_CHUNK_ENCRYPTED_XML == pChunks[i].type)
        {
            uint16_t offsetVal = chunk_start - current_meta_stream_pos;
            pChunks[i].decryptMetadata(metaturing, offsetVal);
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

    delete hfh;
    delete ofh;

    return 0;
}

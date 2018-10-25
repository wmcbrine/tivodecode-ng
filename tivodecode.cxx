/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING.md for license terms
 *
 * derived from mpegcat, copyright 2006 Kees Cook, used with permission
 */

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "getopt_td.hxx"
#include "cli_common.hxx"
#include "tivo_parse.hxx"
#include "tivo_decoder_ts.hxx"
#include "tivo_decoder_ps.hxx"

bool o_no_verify = false;

static struct td_option long_options[] = {
    {"mak", true, 'm'},
    {"out", true, 'o'},
    {"verbose", false, 'v'},
    {"pkt-dump", true, 'p'},
    {"metadata", false, 'D'},
    {"no-verify", false, 'n'},
    {"no-video", false, 'x'},
    {"version", false, 'V'},
    {"help", false, 'h'},
    {"", false, 0}
};

static void do_help(const char *arg0, int exitval)
{
    std::cerr << "Usage: " << arg0 << " [--help] [--verbose|-v] "
        "[--no-verify|-n] [--pkt-dump|-p] pkt_num {--mak|-m} mak "
        "[--metadata|-D] [{--out|-o} outfile] <tivofile>\n\n"
        " -m, --mak         media access key (required)\n"
        " -o, --out,        output file (default stdout)\n"
        " -v, --verbose,    verbose\n"
        " -p, --pkt-dump,   verbose logging for specific TS packet number\n"
        " -D, --metadata,   dump TiVo recording metadata\n"
        " -n, --no-verify,  do not verify MAK while decoding\n"
        " -x, --no-video,   don't decode video, exit after metadata\n"
        " -V, --version,    print the version information and exit\n"
        " -h, --help,       print this help and exit\n\n"
        "The file name specified for the tivo file may be -, which means "
        "stdin.\n\n";
    std::exit(exitval);
}

int main(int argc, const char **argv)
{
    TuringState turing;
    TuringState metaturing;
    TiVoStreamHeader header;

    bool o_no_video = false;
    bool o_dump_metadata = false;
    uint32_t pktDump = 0;

    std::string tivofile = "";
    std::string outfile = "";
    std::string mak = "";

    int64_t current_meta_stream_pos = 0;

    pktDumpMap.clear();

    while (true)
    {
        int c = getopt_td(argc, argv, "m:o:hnDxvVp:", long_options, 0);

        if (c == -1)
            break;

        switch (c)
        {
            case 'm':
                mak = td_optarg;
                break;
            case 'p':
                pktDump = std::stoi(td_optarg);
                pktDumpMap[pktDump] = true;
                break;
            case 'o':
                outfile = td_optarg;
                break;
            case 'h':
                do_help(argv[0], 1);
                break;
            case 'v':
                o_verbose++;
                break;
            case 'n':
                o_no_verify = true;
                break;
            case 'D' :
                o_dump_metadata = true;
                break;
            case 'x':
                o_no_video = true;
                break;
            case '?':
                do_help(argv[0], 2);
                break;
            case 'V':
                do_version(10);
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

    if ("" == mak || "" == tivofile)
        do_help(argv[0], 5);

    HappyFile hfh(tivofile, "rb");
    HappyFile ofh(outfile, "wb");

    print_qualcomm_msg();

    if (false == header.read(hfh))
        return 8;

    header.dump();

    TiVoStreamChunk *pChunks = new TiVoStreamChunk[header.chunks];

    for (uint16_t i = 0; i < header.chunks; i++)
    {
        int64_t chunk_start = hfh.tell() + 12;

        if (false == pChunks[i].read(hfh))
        {
            std::perror("chunk read fail");
            return 8;
        }

        if (TIVO_CHUNK_PLAINTEXT_XML == pChunks[i].type)
        {
            pChunks[i].setupTuringKey(turing, mak);
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

        if (o_dump_metadata)
        {
            char buf[27];
            std::sprintf(buf, "chunk-%02u-%04x.xml", i, pChunks[i].id);

            HappyFile chunkfh(buf, "wb");

            pChunks[i].dump();
            if (false == pChunks[i].write(chunkfh))
            {
                std::perror("write chunk");
                return 8;
            }
        }
    }

    if (o_no_video)
        std::exit(0);

    if ((hfh.tell() > header.mpeg_offset) ||
        !hfh.seek(header.mpeg_offset))
    {
        std::perror("Error reading header");
        return 8; // I dunno
    }

    TiVoDecoder *pDecoder = NULL;
    if (TIVO_FORMAT_PS == header.getFormatType())
        pDecoder = new TiVoDecoderPS(turing, hfh, ofh);
    else if (TIVO_FORMAT_TS == header.getFormatType())
        pDecoder = new TiVoDecoderTS(turing, hfh, ofh);
    else
    {
        std::perror("Unable to create TiVo Decoder");
        return 9;
    }

    if (false == pDecoder->process())
    {
        std::perror("Failed to process file");
        return 9;
    }

    return 0;
}

/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 *
 * derived from mpegcat, copyright 2006 Kees Cook, used with permission
 */
#ifdef HAVE_CONFIG_H
#include "tdconfig.h"
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "getopt_long.h"

#include "cli_common.hxx"
#include "tivo_parse.hxx"
#include "tivo_decoder_ts.hxx"
#include "tivo_decoder_ps.hxx"

int o_no_verify;

static struct option long_options[] = {
    {"mak", 1, 0, 'm'},
    {"out", 1, 0, 'o'},
    {"verbose", 0, 0, 'v'},
    {"pkt-dump", 1, 0, 'p'},
    {"metadata", 0, 0, 'D'},
    {"no-verify", 0, 0, 'n'},
    {"no-video", 0, 0, 'x'},
    {"version", 0, 0, 'V'},
    {"help", 0, 0, 'h'},
    {0, 0, 0, 0}
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
        "The file names specified for the output file or the "
        "tivo file may be -, which\n"
        "means stdout or stdin respectively\n\n";
    std::exit(exitval);
}

int main(int argc, char *argv[])
{
    char rawbuf[RAWBUFSIZE];
    int o_no_video = 0;
    int o_dump_metadata = 0;
    int makgiven = 0;
    UINT32 pktDump = 0;

    const char *tivofile = NULL;
    const char *outfile  = NULL;

    CHAR mak[12];
    std::memset(mak, 0, sizeof(mak));

    TuringState turing;
    std::memset(&turing, 0, sizeof(turing));

    TuringState metaturing;
    std::memset(&metaturing, 0, sizeof(metaturing));
    hoff_t current_meta_stream_pos = 0;

    FILE *ofh = NULL;
    HappyFile *hfh = NULL;

    TiVoStreamHeader header;
    pktDumpMap.clear();

    while (1)
    {
        int c = getopt_long(argc, argv, "m:o:hnDxvVp:", long_options, 0);

        if (c == -1)
            break;

        switch (c)
        {
            case 'm':
                std::strncpy(mak, optarg, 11);
                mak[11] = '\0';
                makgiven = 1;
                break;
            case 'p':
                std::sscanf(optarg, "%d", &pktDump);
                pktDumpMap[pktDump] = TRUE;
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'h':
                do_help(argv[0], 1);
                break;
            case 'v':
                o_verbose++;
                break;
            case 'n':
                o_no_verify = 1;
                break;
            case 'D' :
                o_dump_metadata = 1;
                break;
            case 'x':
                o_no_video = 1;
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

    if (!makgiven)
        makgiven = get_mak_from_conf_file(mak);
        
    if (optind < argc)
    {
        tivofile = argv[optind++];
        if (optind < argc)
            do_help(argv[0], 4);
    }

    if (!makgiven || !tivofile)
    {
        do_help(argv[0], 5);
    }

    hfh = new HappyFile;

    if (!std::strcmp(tivofile, "-"))
    {
// JKOZEE-Make sure stdin is set to binary on Windows
#ifdef WIN32
        int result = _setmode(_fileno(stdin), _O_BINARY);
        if (result == -1) {
            std::perror("Cannot set stdin to binary mode");
            return 10;
        }
#endif
        hfh->attach(stdin);
    }
    else
    {
        if (!hfh->open(tivofile, "rb"))
        {
            std::perror(tivofile);
            return 6;
        }
    }

    if (!outfile || !std::strcmp(outfile, "-"))
    {
// JKOZEE-Make sure stdout is set to binary on Windows
#ifdef WIN32
        int result = _setmode(_fileno(stdout), _O_BINARY);
        if (result == -1) {
            std::perror("Cannot set stdout to binary mode");
            return 10;
        }
#endif
        ofh = stdout;
    }
    else
    {
        ofh = std::fopen(outfile, "wb");
        if (NULL == ofh)
        {
            std::perror("opening output file");
            return 7;
        }
    }

    std::setvbuf(ofh, rawbuf, _IOFBF, RAWBUFSIZE);

    PRINT_QUALCOMM_MSG();

    if (FALSE == header.read(hfh))
    {
        return(8);
    }

    header.dump();

    TiVoStreamChunk *pChunks = new TiVoStreamChunk[header.chunks];
    if (NULL == pChunks)
    {
        std::perror("allocate TiVoStreamChunks");
        return(9);
    }

    for (INT32 i = 0; i < header.chunks; i++)
    {
        hoff_t chunk_start = hfh->tell() + pChunks[i].size();

        if (FALSE == pChunks[i].read(hfh))
        {
            std::perror("chunk read fail");
            return(8);
        }

        if (TIVO_CHUNK_PLAINTEXT_XML == pChunks[i].type)
        {
            pChunks[i].setupTuringKey(&turing, (UINT8*)mak);
            pChunks[i].setupMetadataKey(&metaturing, (UINT8*)mak);
        }
        else if (TIVO_CHUNK_ENCRYPTED_XML == pChunks[i].type)
        {
            UINT16 offsetVal = chunk_start - current_meta_stream_pos;
            pChunks[i].decryptMetadata(&metaturing, offsetVal);
            current_meta_stream_pos = chunk_start + pChunks[i].dataSize;
        }
        else
        {
            std::perror("Unknown chunk type");
            return(8);
        }

        if (o_dump_metadata)
        {
            CHAR buf[25];
            std::sprintf(buf, "%s-%02d-%04x.xml", "chunk", i, pChunks[i].id);

            FILE *chunkfh = std::fopen(buf, "wb");
            if (!chunkfh)
            {
                std::perror("create metadata file");
                return 8;
            }

            pChunks[i].dump();
            if (FALSE == pChunks[i].write(chunkfh))
            {
                std::perror("write chunk");
                return 8;
            }

            std::fclose(chunkfh);
        }
    }

//    metaturing.destruct();

    if (o_no_video)
        std::exit(0);

    if ((hfh->tell() > header.mpeg_offset) ||
        (hfh->seek(header.mpeg_offset) < 0))
    {
        std::perror("Error reading header");
        return 8; // I dunno       
    }

    TiVoDecoder *pDecoder = NULL;
    if (TIVO_FORMAT_PS == header.getFormatType())
    {
        pDecoder = new TiVoDecoderPS(&turing, hfh, ofh);
    }
    else if (TIVO_FORMAT_TS == header.getFormatType())
    {
        pDecoder = new TiVoDecoderTS(&turing, hfh, ofh);
    }

    if (NULL == pDecoder)
    {
        std::perror("Unable to create TiVo Decoder");
        return 9;
    }
    
    if (FALSE == pDecoder->process())
    {
        std::perror("Failed to process file");
        return 9;
    }

    turing.destruct();

    hfh->close();
    delete hfh;

    if (ofh != stdout)
        std::fclose(ofh);

    return 0;
}

/* vi:set ai ts=4 sw=4 expandtab: */

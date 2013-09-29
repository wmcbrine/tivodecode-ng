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
#include "tivo-parse.h"

int o_ts_pkt_dump = 0;

typedef enum
{
	TIVO_FORMAT_NONE,
    TIVO_FORMAT_PS,
    TIVO_FORMAT_TS,
    TIVO_FORMAT_MAX
}
tivo_format_type;

static int hread_wrapper (void * mem, int size, void * fh)
{
    return (int)hread (mem, size, (happy_file *)fh);
}

static int fwrite_wrapper (void * mem, int size, void * fh)
{
    return (int)fwrite (mem, 1, size, (FILE *)fh);
}

static struct option long_options[] = {
    {"mak", 1, 0, 'm'},
    {"out", 1, 0, 'o'},
    {"help", 0, 0, 'h'},
    {"verbose", 0, 0, 'v'},
    {"pkt-dump", 1, 0, 'p'},
    {"version", 0, 0, 'V'},
    {"no-verify", 0, 0, 'n'},
    {"metadata", 0, 0, 'D'},
    {"no-video", 0, 0, 'x'},
    {0, 0, 0, 0}
};

static void do_help(char * arg0, int exitval)
{
    fprintf(stderr, "Usage: %s [--help] [--verbose|-v] [--no-verify|-n] [--pkt-dump|-p] pkt_num {--mak|-m} mak [--metadata|-D] [{--out|-o} outfile] <tivofile>\n\n", arg0);
#define ERROUT(s) fprintf(stderr, s)
    ERROUT ("  --mak, -m        media access key (required)\n");
    ERROUT ("  --out, -o        output file (default stdout)\n");
    ERROUT ("  --verbose, -v    verbose\n");
    ERROUT ("  --pkt-dump, -p   verbose logging for specific TS packet number\n");
    ERROUT ("  --metadata, -D   dump TiVo recording metadata\n");
    ERROUT ("  --no-verify, -n  do not verify MAK while decoding\n");
    ERROUT ("  --no-video, -x   don't decode video, exit after metadata\n");
    ERROUT ("  --version, -V    print the version information and exit\n");
    ERROUT ("  --help, -h       print this help and exit\n\n");
    ERROUT ("The file names specified for the output file or the tivo file may be -, which\n");
    ERROUT ("means stdout or stdin respectively\n\n");
#undef ERROUT

    exit (exitval);
}


int main(int argc, char *argv[])
{
    int o_no_video = 0;
    int o_dump_chunks = 1;
    int o_dump_metadata = 0;
    unsigned int marker;
    unsigned char byte;
    char first = 1;
    
    tivo_format_type format = TIVO_FORMAT_NONE;

    int running = 1;
	int ret = 0;

    char * tivofile = NULL;
    char * outfile = NULL;
    char mak[12];

    int makgiven = 0;

    turing_state turing;

    FILE * ofh;
    happy_file * hfh=NULL;
    // file position options
    hoff_t begin_at = 0;

    memset(&turing, 0, sizeof(turing));
    memset(mak, 0, sizeof(mak));

    while (1)
    {
        int c = getopt_long (argc, argv, "m:o:hnDxvV", long_options, 0);

        if (c == -1)
            break;

        switch (c)
        {
            case 'm':
                strncpy(mak, optarg, 11);
                mak[11] = '\0';
                makgiven = 1;
                break;
			case 'p':
                sscanf(optarg, "%d", &o_ts_pkt_dump);
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
        if (!(hfh=hopen(tivofile, "rb")))
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

    PRINT_QUALCOMM_MSG();

    if (o_dump_chunks)
    {
        /* parse the tivo headers manually here, since we care about more
         * than the init_turing_from_file function will get
         */
        tivo_stream_header head;
        tivo_stream_chunk *chunk;
        turing_state metaturing;
        hoff_t current_meta_stream_pos = 0;
        int i;

        memset(&metaturing, 0, sizeof(metaturing));

        if (read_tivo_header (hfh, &head, &hread_wrapper))
            return 8;

		VVVERBOSE("TiVo Head\n");
		if ( IS_VVVERBOSE )
			dump_tivo_header(&head);
		
        begin_at = head.mpeg_offset;
        
        if ( head.dummy_0006 & 0x20 )
        {
        	format = TIVO_FORMAT_TS;
        }
        else
        {
        	format = TIVO_FORMAT_PS;
        }

        for (i=0; i<head.chunks; i++)
        {
            /* TODO: find a better way to present the chunks */
            /* maybe a simple tar format writer */
            char buf[4096];
            hoff_t chunk_start = htell(hfh) + SIZEOF_STREAM_CHUNK;

            if ((chunk = read_tivo_chunk (hfh, &hread_wrapper)) == NULL)
                return 8;

            if (chunk->data_size && chunk->type == TIVO_CHUNK_PLAINTEXT_XML )
            {
				if ( IS_VVVERBOSE )
					dump_tivo_chunk(chunk);
				            	
                if (!o_no_video)
                    setup_turing_key (&turing, chunk, mak);
                setup_metadata_key (&metaturing, chunk, mak);
                free (chunk);
                continue;
            }


			if ( o_dump_metadata )
			{            
	            FILE * chunkfh;

	            sprintf(buf, "%s-%02d-%04x.xml", "chunk", i, chunk->id);
	
	            chunkfh = fopen(buf, "wb");
	            if (!chunkfh)
	            {
	                perror("create metadata file");
	                return 8;
	            }
	
	            prepare_frame(&metaturing, 0, 0);
	            skip_turing_data(&metaturing, (size_t)(chunk_start - current_meta_stream_pos));
	            decrypt_buffer(&metaturing, chunk->data, chunk->data_size);
	            current_meta_stream_pos = chunk_start + chunk->data_size;

				if (IS_VVVERBOSE)
					dump_tivo_chunk(chunk);
	
	            if (fwrite (chunk->data, 1, chunk->data_size, chunkfh) != chunk->data_size)
	            {
	                perror("write chunk");
	                return 8;
	            }

	            fclose(chunkfh);
        	}

            free(chunk);
        }

        destruct_turing(&metaturing);
        if (o_no_video)
            exit(0);
    }
    else
    {
        if (o_no_video)
            exit(0);

        if ((begin_at = init_turing_from_file (&turing, hfh, &hread_wrapper, mak)) < 0)
            return 8;
    }

    if (hseek(hfh, begin_at, SEEK_SET) < 0)
    {
        perror ("seek");
        return 9;
    }

	if ( format == TIVO_FORMAT_TS )
	{
		running = 1;
		while ( running )
		{
			ret = process_ts_frame(&turing, htell(hfh), hfh, &hread_wrapper, ofh, &fwrite_wrapper);
			if ( ret < 0 )
            {
                perror ("processing frame");
                return 10;
            }			
            else if ( ret == 0 )
           	{
	            fprintf(stderr, "End of File\n");
           		running = 0;
           	}
        }		
	}
	else if ( format == TIVO_FORMAT_PS )
	{
	    marker = 0xFFFFFFFF;
	    while (running)
	    {
	        if ((marker & 0xFFFFFF00) == 0x100)
	        {
				ret = process_ps_frame(byte, &turing, htell(hfh), hfh, &hread_wrapper, ofh, &fwrite_wrapper);
					
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
    }
    else
    {
		perror ("invalid TiVo format");
		return 10;
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

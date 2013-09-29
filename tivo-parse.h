/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */
#include <stddef.h>

#ifndef TIVO_PARSE_H_
#define TIVO_PARSE_H_
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initial header formats lifted from ezrec's posting:
 * http://www.dealdatabase.com/forum/showthread.php?t=41132
 */

/* All elements are in big-endian format and are packed */

#define SIZEOF_STREAM_HEADER 16
typedef struct tivo_stream_header_s {
	char           filetype[4];       /* the string 'TiVo' */
	unsigned short dummy_0004;
	unsigned short dummy_0006;
	unsigned short dummy_0008;
	unsigned int   mpeg_offset;   /* 0-based offset of MPEG stream */
	unsigned short chunks;        /* Number of metadata chunks */
} tivo_stream_header;

#define TIVO_CHUNK_PLAINTEXT_XML  0
#define TIVO_CHUNK_ENCRYPTED_XML  1

#define SIZEOF_STREAM_CHUNK 12
typedef struct tivo_stream_chunk_s {
	unsigned int   chunk_size;    /* Size of chunk */
	unsigned int   data_size;     /* Length of the payload */
	unsigned short id;            /* Chunk ID */
	unsigned short type;          /* Subtype */
	unsigned char  data[1];       /* Variable length data */
} tivo_stream_chunk;

/* genericized read function so that different underlying implementations can
 * be swapped out for more of a library setup
 */
typedef int (*read_func_t) (void * mem, int size, void * fh);

int read_tivo_header(void * file, tivo_stream_header * head, read_func_t read_handler);
tivo_stream_chunk * read_tivo_chunk(void * file, read_func_t read_handler);

void dump_tivo_header(tivo_stream_header * head);
void dump_tivo_chunk(tivo_stream_chunk * chunk);

extern int isBigEndian();
extern unsigned short portable_ntohs( unsigned char * pVal );
extern unsigned long  portable_ntohl( unsigned char * pVal );

#define VERBOSE(...)		if ( o_verbose >= 1 ) { printf(__VA_ARGS__); }
#define VVERBOSE(...)		if ( o_verbose >= 2 ) { printf(__VA_ARGS__); }
#define VVVERBOSE(...)		if ( o_verbose >= 3 ) { printf(__VA_ARGS__); }
	
#define IS_VERBOSE			( o_verbose >= 1 )
#define IS_VVERBOSE			( o_verbose >= 2 )
#define IS_VVVERBOSE		( o_verbose >= 3 )

#ifdef __cplusplus
}
#endif
#endif

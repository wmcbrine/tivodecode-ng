#include <stddef.h>
#include "happyfile.h"

#ifndef TIVO_PARSE_H_
#define TIVO_PARSE_H_

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

#define TIVO_CHUNK_XML  0
#define TIVO_CHUNK_BLOB 1

#define SIZEOF_STREAM_CHUNK 12
typedef struct tivo_stream_chunk_s {
	unsigned int   chunk_size;    /* Size of chunk */
	unsigned int   data_size;     /* Length of the payload */
	unsigned short id;            /* Chunk ID */
	unsigned short type;          /* Subtype */
} tivo_stream_chunk;


typedef struct {
	size_t size;
	unsigned char * data;
} blob;

size_t parse_tivo(happy_file * file, blob * xml);

#endif

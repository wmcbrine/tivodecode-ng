#include <stdint.h>
#include <stddef.h>
#include "happyfile.h"

#ifndef TIVO_PARSE_H_
#define TIVO_PARSE_H_

/*
 * Initial header formats lifted from ezrec's posting:
 * http://www.dealdatabase.com/forum/showthread.php?t=41132
 */

/* All elements are in big-endian format */
typedef struct tivo_stream_header_s {
	char filetype[4];       /* the string 'TiVo' */
	uint16_t dummy_0004;
	uint16_t dummy_0006;
	uint16_t dummy_0008;
	uint32_t mpeg_offset;   /* 0-based offset of MPEG stream */
	uint16_t chunks;        /* Number of metadata chunks */
} __attribute__((packed)) tivo_stream_header;

#define TIVO_CHUNK_XML  0
#define TIVO_CHUNK_BLOB 1

typedef struct tivo_stream_chunk_s {
	uint32_t chunk_size;    /* Size of chunk */
	uint32_t data_size;     /* Length of the payload */
	uint16_t id;            /* Chunk ID */
	uint16_t type;          /* Subtype */
} __attribute__((packed)) tivo_stream_chunk;


typedef struct {
	size_t size;
	char * data;
} blob;

uint32_t parse_tivo(happy_file * file, blob * xml);

#endif

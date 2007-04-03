/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */
#ifdef HAVE_CONFIG_H
# include "tdconfig.h"
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef WIN32
# include <windows.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "sha1.h"
#include "tivo-parse.h"

#define LOAD_NET_UNALIGNED_HELP(ptr, offset, len) ((unsigned int)ptr[offset] << (((len - 1) << 3) - (offset << 3)))

#define NETLONG_UNALIGNED(ptr) ( \
		LOAD_NET_UNALIGNED_HELP(ptr, 0, 4) | \
		LOAD_NET_UNALIGNED_HELP(ptr, 1, 4) | \
		LOAD_NET_UNALIGNED_HELP(ptr, 2, 4) | \
		LOAD_NET_UNALIGNED_HELP(ptr, 3, 4) \
	)

int read_tivo_header(void * file, tivo_stream_header * head, read_func_t read_handler)
{
	struct {
		char           filetype[4];
		/* all fields are in network byte order */
		unsigned short dummy_0004;
		unsigned short dummy_0006;
		unsigned short dummy_0008;
		unsigned char  mpeg_offset[4]; /* unsigned int */
		unsigned short chunks;
	} raw_tivo_header;


	if (read_handler (&raw_tivo_header, SIZEOF_STREAM_HEADER, file) != SIZEOF_STREAM_HEADER)
	{
		perror ("read head");
		return -1;
	}
#if 1
	// these are unused
	memcpy(head->filetype, raw_tivo_header.filetype, 4);
	head->dummy_0004 = ntohs(raw_tivo_header.dummy_0004);
	head->dummy_0006 = ntohs(raw_tivo_header.dummy_0006);
	head->dummy_0008 = ntohs(raw_tivo_header.dummy_0008);
#endif
	head->mpeg_offset = NETLONG_UNALIGNED(raw_tivo_header.mpeg_offset);
	head->chunks = ntohs(raw_tivo_header.chunks);

	return 0;
}

tivo_stream_chunk * read_tivo_chunk(void * file, read_func_t read_handler)
{
	tivo_stream_chunk * chunk;
	unsigned int        chunk_sz;
	tivo_stream_chunk   raw_chunk_header;

	if (read_handler (&raw_chunk_header, SIZEOF_STREAM_CHUNK, file) != SIZEOF_STREAM_CHUNK)
	{
		perror("read chunk head");
		return NULL;
	}

	chunk_sz = ntohl(raw_chunk_header.chunk_size);

	/* allocate the memory for the chunk */
	if (!(chunk = (tivo_stream_chunk *)malloc(chunk_sz)))
	{
		perror("malloc");
		exit(1);
	}

	// network byte order conversion
	chunk->chunk_size = chunk_sz;
	chunk->data_size  = ntohl(raw_chunk_header.data_size);
	chunk->id         = ntohs(raw_chunk_header.id       );
	chunk->type       = ntohs(raw_chunk_header.type     );

	/* read the rest of the chunk */
	if (read_handler(chunk->data, (int)chunk_sz - SIZEOF_STREAM_CHUNK, file)
			!= chunk_sz - SIZEOF_STREAM_CHUNK)
	{
		perror("read chunk data");
		free(chunk);
		return NULL;
	}

	return chunk;
}


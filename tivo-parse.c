/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#	include <windows.h>
#else
#	include <netinet/in.h>
#endif
#include "sha1.h"
#include "tivo-parse.h"

#define FILL_NETGNRC(var, buf, offset, size, sz) do { \
	memcpy (&var, buf + offset, size); \
	var = ntoh ## sz (var); \
} while(0)

#define FILL_NETLONG(var, buf, offset) FILL_NETGNRC(var, buf, offset, sizeof(unsigned int), l)

#define FILL_NETSHRT(var, buf, offset) FILL_NETGNRC(var, buf, offset, sizeof(unsigned short), s)

unsigned int parse_tivo(void * file, blob * xml, read_func_t read_handler)
{
	char buf[16];
	tivo_stream_header head;
	tivo_stream_chunk  chunk;

	if (read_handler (buf, SIZEOF_STREAM_HEADER, file) != SIZEOF_STREAM_HEADER)
	{
		perror ("read head");
		return -1;
	}
	// network byte order conversion
#if 0
	head.dummy_0004=ntohs(head.dummy_0004);
	head.dummy_0006=ntohs(head.dummy_0006);
	head.dummy_0008=ntohs(head.dummy_0008);
#endif
	FILL_NETLONG(head.mpeg_offset, buf, 10);
	FILL_NETSHRT(head.chunks, buf, 14);

	if (read_handler (buf, SIZEOF_STREAM_CHUNK, file) != SIZEOF_STREAM_CHUNK)
	{
		perror("read chunk head");
		return -1;
	}

	// network byte order conversion
	FILL_NETLONG (chunk.chunk_size, buf, 0);
	FILL_NETLONG (chunk.data_size, buf, 4);
	FILL_NETSHRT (chunk.id, buf, 8);
	FILL_NETSHRT (chunk.type, buf, 10);

	if (chunk.data_size && chunk.type == TIVO_CHUNK_XML)
	{
		xml->size = chunk.data_size;
		if (!(xml->data = (unsigned char *)malloc(chunk.data_size + 1)))
		{
			perror("malloc");
			exit(1);
		}
		if (read_handler (xml->data, (int)xml->size, file) != xml->size)
		{
			perror("read chunk data");
			free(xml->data);
			return -1;
		}
	}

	return head.mpeg_offset;
}


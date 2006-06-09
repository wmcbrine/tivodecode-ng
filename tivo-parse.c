#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#	include <windows.h>
#else
#	include <netinet/in.h>
#endif
#include "sha1.h"
#include "hexlib.h"
#include "tivo-parse.h"

unsigned int parse_tivo(happy_file * file, blob * xml)
{
	char buf[16];
	tivo_stream_header head;
	tivo_stream_chunk  chunk;

	if (hread (buf, SIZEOF_STREAM_HEADER, file) != SIZEOF_STREAM_HEADER)
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
	head.mpeg_offset = ntohl (*(unsigned int *)(buf + 10));
	head.chunks = ntohs (*(unsigned short *)(buf + 14));

	if (hread (buf, SIZEOF_STREAM_CHUNK, file) != SIZEOF_STREAM_CHUNK)
	{
		perror("read chunk head");
		return -1;
	}

	// network byte order conversion
	chunk.chunk_size=ntohl(*(int *)buf);
	chunk.data_size=ntohl(*(int *)(buf + 4));
	chunk.id=ntohs(*(short *)(buf + 8));
	chunk.type=ntohs(*(short *)(buf + 10));

	if (chunk.data_size && chunk.type == TIVO_CHUNK_XML)
	{
		xml->size = chunk.data_size;
		if (!(xml->data = (char *)malloc(chunk. data_size + 1)))
		{
			perror("malloc");
			exit(1);
		}
		if (hread (xml->data, xml->size, file) != xml->size)
		{
			perror("read chunk data");
			free(xml->data);
			return -1;
		}
	}

	return head.mpeg_offset;
}


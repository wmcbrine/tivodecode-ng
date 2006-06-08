#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "sha1.h"
#include "hexlib.h"
#include "tivo-parse.h"

uint32_t parse_tivo(char * filename, blob * xml)
{
	tivo_stream_header head;
	tivo_stream_chunk  chunk;
	FILE * tivo;

       	if (!(tivo = fopen (filename, "r")))
	{
		perror("open");
		return -1;
	}

	if (fread (&head, sizeof(head), 1, tivo) != 1)
	{
		perror ("read head");
		fclose (tivo);
		return -1;
	}
	// network byte order conversion
#if 0
	head.dummy_0004=ntohs(head.dummy_0004);
	head.dummy_0006=ntohs(head.dummy_0006);
	head.dummy_0008=ntohs(head.dummy_0008);
#endif
	head.mpeg_offset = ntohl (head.mpeg_offset);
	head.chunks = ntohs (head.chunks);

	if (fread (&chunk, sizeof(chunk), 1, tivo) != 1)
	{
		perror("read chunk head");
		fclose(tivo);
		return -1;
	}

	// network byte order conversion
	chunk.chunk_size=ntohl(chunk.chunk_size);
	chunk.data_size=ntohl(chunk.data_size);
	chunk.id=ntohs(chunk.id);
	chunk.type=ntohs(chunk.type);

	if (chunk.data_size && chunk.type == TIVO_CHUNK_XML)
	{
		xml->size = chunk.data_size;
		if (!(xml->data = (char *)malloc(chunk. data_size + 1)))
		{
			perror("malloc");
			exit(1);
		}
		if (fread (xml->data, xml->size, 1, tivo) != 1)
		{
			perror("read chunk data");
			free(xml->data);
			fclose(tivo);
			return -1;
		}
	}

	fclose(tivo);
	return head.mpeg_offset;
}


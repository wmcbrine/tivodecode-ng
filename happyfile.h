/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */

#ifndef HAPPY_FILE_H_
#define HAPPY_FILE_H_

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef BUFFERSIZE
#define BUFFERSIZE 4096
#endif

typedef struct
{
	FILE * fh;
	off_t  pos;

	/* buffer stuff */
	off_t  buffer_start;
	off_t  buffer_fill;

	char buffer[BUFFERSIZE];
} happy_file;

happy_file * hopen (char * filename, char * mode);
happy_file * hattach (FILE * fh);

int hclose(happy_file * fh);
int hdetach(happy_file * fh);

size_t hread (void * ptr, size_t size, happy_file * fh);
size_t hwrite (const void * ptr, size_t size, happy_file * fh);

off_t htell (happy_file * fh);
int hseek (happy_file * fh, off_t offset, int whence);

#endif

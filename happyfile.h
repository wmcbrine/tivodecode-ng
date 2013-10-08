/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */

#ifndef HAPPY_FILE_H_
#define HAPPY_FILE_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "tdconfig.h"

#include <stdio.h>
#include <stddef.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H 
# include <unistd.h>
#endif

#if defined(WIN32)
# include <io.h>
#endif

#ifndef BUFFERSIZE
#define BUFFERSIZE 4096
#endif

#if SIZEOF_OFF_T == 8
typedef off_t hoff_t;
#elif defined (WIN32)
typedef __int64 hoff_t;
#else
#warning Large file support is questionable on this platform
typedef off_t hoff_t;
#endif

typedef struct
{
	FILE * fh;
	hoff_t  pos;

	/* buffer stuff */
	hoff_t  buffer_start;
	hoff_t  buffer_fill;

	char buffer[BUFFERSIZE];
} happy_file;

happy_file * hopen (char * filename, char * mode);
happy_file * hattach (FILE * fh);

int hclose(happy_file * fh);
int hdetach(happy_file * fh);

size_t hread (void * ptr, size_t size, happy_file * fh);
size_t hwrite (const void * ptr, size_t size, happy_file * fh);

hoff_t htell (happy_file * fh);
int hseek (happy_file * fh, hoff_t offset, int whence);

#ifdef __cplusplus
}
#endif
#endif

/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */

#ifndef HAPPY_FILE_H_
#define HAPPY_FILE_H_

#include "tdconfig.h"

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

#include <cstdio>

#ifndef RAWBUFSIZE
#define RAWBUFSIZE 65536
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

class HappyFile
{
    private:
        FILE *fh;
        hoff_t pos;

        /* buffer stuff */
        hoff_t buffer_start;
        hoff_t buffer_fill;

        char rawbuf[RAWBUFSIZE];
        char buffer[BUFFERSIZE];

        void init();

    public:
        int open(const char *filename, const char *mode);
        int attach(FILE *fh);

        int close();

        size_t read(void *ptr, size_t size);

        hoff_t tell();
        int seek(hoff_t offset);
};

#endif

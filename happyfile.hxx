/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#ifndef HAPPY_FILE_H_
#define HAPPY_FILE_H_

#include "tdconfig.h"

#include <cstdio>
#include <cstdint>

#ifndef RAWBUFSIZE
#define RAWBUFSIZE 65536
#endif

#ifndef BUFFERSIZE
#define BUFFERSIZE 4096
#endif

typedef int64_t hoff_t;

class HappyFile
{
    private:
        FILE *fh;
        bool attached;
        hoff_t pos;

        /* buffer stuff */
        hoff_t buffer_start;
        hoff_t buffer_fill;

        char rawbuf[RAWBUFSIZE];
        char buffer[BUFFERSIZE];

        void init();

    public:
        bool open(const char *filename, const char *mode);
        bool attach(FILE *fh);

        void close();

        size_t read(void *ptr, size_t size);
        size_t write(void *ptr, size_t size);

        hoff_t tell();
        int seek(hoff_t offset);

        HappyFile();
        ~HappyFile();
};

#endif

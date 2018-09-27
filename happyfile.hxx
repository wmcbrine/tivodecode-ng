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

const size_t TD_RAWBUFSIZE = 65536;
const size_t TD_BUFFERSIZE = 4096;

class HappyFile
{
    private:
        FILE *fh;
        bool attached;
        int64_t pos;

        /* buffer stuff */
        int64_t buffer_start;
        int64_t buffer_fill;

        char rawbuf[TD_RAWBUFSIZE];
        char buffer[TD_BUFFERSIZE];

        void init();

    public:
        bool open(const char *filename, const char *mode);
        bool attach(FILE *fh);

        void close();

        size_t read(void *ptr, size_t size);
        size_t write(void *ptr, size_t size);

        int64_t tell();
        bool seek(int64_t offset);

        HappyFile();
        ~HappyFile();
};

#endif

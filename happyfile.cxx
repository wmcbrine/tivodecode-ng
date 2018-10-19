/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING.md for license terms
 */

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>

#ifdef WIN32
# include <fcntl.h>
# include <io.h>
#endif

#include "happyfile.hxx"

HappyFile::HappyFile(std::string filename, std::string mode)
{
    bool writemode = ("wb" == mode);

    rawbuf = new char[TD_RAWBUFSIZE];
    buffer = new char[TD_BUFFERSIZE];

    if ("" == filename || "-" == filename)
    {
        if (!attach(writemode ? stdout : stdin))
            std::exit(10);
    }
    else
    {
        if (!open(filename, mode))
        {
            std::perror(writemode ? "opening output file" : filename.c_str());
            std::exit(6 + writemode);
        }
    }

    init();
}

HappyFile::~HappyFile()
{
    close();

    delete buffer;
    delete rawbuf;
}

void HappyFile::init()
{
    std::setvbuf(fh, rawbuf, _IOFBF, TD_RAWBUFSIZE);
    pos = 0;
    buffer_start = 0;
    buffer_fill = 0;
}

bool HappyFile::open(std::string &filename, std::string &mode)
{
    fh = std::fopen(filename.c_str(), mode.c_str());
    if (!fh)
        return false;
    attached = false;
    return true;
}

bool HappyFile::attach(FILE *handle)
{
    fh = handle;
    attached = true;

// JKOZEE-Make sure pipe is set to binary on Windows
#ifdef WIN32
    if (-1 == _setmode(_fileno(handle), _O_BINARY))
    {
        std::perror("Cannot set pipe to binary mode");
        return false;
    }
#endif

    return true;
}

void HappyFile::close()
{
    if (!attached)
        std::fclose(fh);
    else if (fh == stdout)
        std::fflush(fh);
}

size_t HappyFile::read(void *ptr, size_t size)
{
    size_t nbytes = 0;

    if (size == 0)
        return 0;

    char *bstart = buffer + (pos - buffer_start);
    if ((pos + (int64_t)size) - buffer_start <= buffer_fill)
    {
        std::copy(bstart, bstart + size, (char *)ptr);
        pos += (int64_t)size;
        return size;
    }
    else if (pos < buffer_start + buffer_fill)
    {
        size_t offset = buffer_fill - (pos - buffer_start);
        std::copy(bstart, bstart + offset, (char *)ptr);
        nbytes += offset;
    }

    do
    {
        buffer_start += buffer_fill;
        buffer_fill = (int64_t)std::fread(buffer, 1, TD_BUFFERSIZE, fh);

        if (buffer_fill == 0)
            break;

        size_t offset = std::min(size - nbytes, (size_t)buffer_fill);
        std::copy(buffer, buffer + offset, (char *)ptr + nbytes);
        nbytes += offset;

    } while (nbytes < size);

    pos += (int64_t)nbytes;
    return nbytes;
}

size_t HappyFile::write(void *ptr, size_t size)
{
    return std::fwrite(ptr, 1, size, fh);
}

int64_t HappyFile::tell()
{
    return pos;
}

bool HappyFile::seek(int64_t offset)
{
    static char junk_buf[4096];

    int t = (int)((offset - pos) & 0xfff);
    int64_t u = (offset - pos) >> 12;
    int64_t s;
    for (s = 0; s < u; s++)
    {
        if (read(junk_buf, 4096) != 4096)
            return false;
    }

    if (read(junk_buf, t) != (size_t)t)
        return false;

    return true;
}

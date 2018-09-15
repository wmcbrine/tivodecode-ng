/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#include <cerrno>
#include <cstdio>
#include <cstring>

#ifdef WIN32
# include <fcntl.h>
# include <io.h>
#endif

#include "happyfile.hxx"

HappyFile::HappyFile()
{
}

HappyFile::~HappyFile()
{
    close();
}

void HappyFile::init()
{
    std::setvbuf(fh, rawbuf, _IOFBF, RAWBUFSIZE);
    pos = 0;
    buffer_start = 0;
    buffer_fill = 0;
}

bool HappyFile::open(const char *filename, const char *mode)
{
    fh = std::fopen(filename, mode);
    if (!fh)
        return false;
    attached = false;
    init();
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

    init();
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

    if ((pos + (hoff_t)size) - buffer_start <= buffer_fill)
    {
        std::memcpy(ptr, buffer + (pos - buffer_start), size);
        pos += (hoff_t)size;
        return size;
    }
    else if (pos < buffer_start + buffer_fill)
    {
        std::memcpy(ptr, buffer + (pos - buffer_start),
                    (size_t)(buffer_fill - (pos - buffer_start)));
        nbytes += (size_t)(buffer_fill - (pos - buffer_start));
    }

    do
    {
        buffer_start += buffer_fill;
        buffer_fill = (hoff_t)std::fread(buffer, 1, BUFFERSIZE, fh);

        if (buffer_fill == 0)
            break;

        std::memcpy((char *)ptr + nbytes, buffer,
                    (hoff_t)(size - nbytes) < buffer_fill ?
                    (size - nbytes) : (size_t)buffer_fill);
        nbytes += (hoff_t)(size - nbytes) < buffer_fill ?
                  (size - nbytes) : (size_t)buffer_fill;
    } while (nbytes < size);

    pos += (hoff_t)nbytes;
    return nbytes;
}

size_t HappyFile::write(void *ptr, size_t size)
{
    return std::fwrite(ptr, 1, size, fh);
}

hoff_t HappyFile::tell()
{
    return pos;
}

int HappyFile::seek(hoff_t offset)
{
    static char junk_buf[4096];

    int t = (int)((offset - pos) & 0xfff);
    hoff_t u = (offset - pos) >> 12;
    hoff_t s;
    for (s = 0; s < u; s++)
    {
        if (read(junk_buf, 4096) != 4096)
            return -1;
    }

    if (read(junk_buf, t) != (size_t)t)
        return -1;

    return 0;
}

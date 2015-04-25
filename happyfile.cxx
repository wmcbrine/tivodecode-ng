/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */
#ifdef HAVE_CONFIG_H
# include "tdconfig.h"
#endif

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "happyfile.hxx"

void HappyFile::init()
{
    std::setvbuf(fh, rawbuf, _IOFBF, RAWBUFSIZE);
    pos = 0;
    buffer_start = 0;
    buffer_fill = 0;
}

int HappyFile::open(const char *filename, const char *mode)
{
    fh = std::fopen(filename, mode);
    if (!fh)
        return 0;
    init();
    return 1;
}

int HappyFile::attach(FILE *infile)
{
    fh = infile;
    init();
    return 1;
}

int HappyFile::close()
{
    if (fh != stdin)
        return std::fclose(fh);
    else
        return 0;
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
    {
        return -1;
    }

    return 0;
}

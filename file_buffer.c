/*
#
# Utilities for seeking around in a file stream
#
# $Id: file_buffer.c,v 1.1 2006-06-08 04:48:47 jeremyd Exp $
#
# Copyright (C) 2001-2003 Kees Cook
# kees@outflux.net, http://outflux.net/
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
# http://www.gnu.org/copyleft/gpl.html
#
*/

/*
 * buffer management inspired by mplayer
 */
#include "file_buffer.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct file_buf_t
{
    char * filename;
    FILE *fp;
    uint8_t * buffer;
    off_t offset;
    size_t buffer_min;
    size_t buffer_max;
    size_t buffer_malloc;
};

off_t buffer_tell(struct file_buf_t *fb)
{
    if (!fb) return 0;
    return fb->offset - fb->buffer_max + fb->buffer_min;
}

off_t buffer_file_size(struct file_buf_t *fb)
{
    struct stat info;

    if (!fb) return 0;
    if (!(fstat(fileno(fb->fp), &info))<0) {
        perror("fstat");
        exit(1);
    }
    return info.st_size;
}

// can point to a file offset to refill from
int buffer_refill(struct file_buf_t *fb)
{
    size_t got;

    if (!fb)
        return FILE_BUF_ERR_NULL_STRUCT;

    //fprintf(stderr,"filling...\n");

    if (fb->buffer_min < fb->buffer_max)
    {
        memmove(fb->buffer, fb->buffer + fb->buffer_min,
                            fb->buffer_max - fb->buffer_min);
    }
    fb->buffer_max -= fb->buffer_min;
    fb->buffer_min = 0;

    got =
        fread(fb->buffer + fb->buffer_max, sizeof(uint8_t),
              fb->buffer_malloc - fb->buffer_max, fb->fp);
    if (got < 0)
        return FILE_BUF_ERR_READ;

    fb->offset += got;
    fb->buffer_max += got;

    //fprintf(stderr,"bump %d more\n",got);
    return FILE_BUF_OKAY;
}

int buffer_seek(struct file_buf_t *fb, off_t location)
{
    if (!fb)
        return FILE_BUF_ERR_NULL_STRUCT;

    // is this location within our current buffer?
    if (location < fb->offset && location >= buffer_tell(fb)) {
        fb->buffer_min += location - buffer_tell(fb);
    }
    else {
        // outside: clear the buffer 
        fb->buffer_min = fb->buffer_max = 0;    // clear buffer
        fb->offset = location;
        if (FSEEK(fb->fp, fb->offset, SEEK_SET) < 0) {
            return FILE_BUF_ERR_SEEK;
        }
    }
    //don't do this... way too slow
    //buffer_refill();
    return FILE_BUF_OKAY;
}

// A null return from "buffer_start" indicates that errno has been set,
// and calls to "perror" and the like are valid.
struct file_buf_t * buffer_start(struct file_buf_t *fb, char *filename,
                                 size_t size)
{
    FILE * fp=NULL;
    uint8_t * buffer=NULL;

    // check for fopen first, before dealing with memory allocation or
    // making changes to the passed-in structure
    if (!(fp = fopen(filename, "r"))) {
        goto err_out;
    }
    if (!fb || fb->buffer_malloc != size) {
        if (!(buffer=(uint8_t*)calloc(size,sizeof(uint8_t)))) {
            goto err_out;
        }
    }
    // create a buffer if it doesn't already exist
    if (!fb) {
        if (!(fb=(struct file_buf_t*)calloc(1,sizeof(*fb)))) {
            goto err_out;
        }
    }
    // swap out the new file handle
    if (fb->fp)
        fclose(fb->fp);
    fb->fp = fp;

    // swap out the new buffer
    if (buffer) {
        // drop the old buffer if we have a new one
        if (fb->buffer)
            free(fb->buffer);
        fb->buffer = buffer;
    }
    // record our buffer size
    fb->buffer_malloc = size;

    // clear our counters
    fb->offset = 0;
    fb->buffer_min = 0;
    fb->buffer_max = 0;

    return fb;

err_out:
    if (buffer)
        free(buffer);
    if (fp)
        fclose(fp);
    buffer_end(fb); // hm: well I guess I *will* muck with the passed in one
    return NULL;
}

int buffer_end(struct file_buf_t *fb)
{
    if (!fb)
        return FILE_BUF_ERR_NULL_STRUCT;

    if (fb->fp)
        fclose(fb->fp);
    if (fb->buffer)
        free(fb->buffer);
    free(fb);

    return FILE_BUF_OKAY;
}

int buffer_get_byte(struct file_buf_t *fb, uint8_t * byte)
{
    int ret;

    if (!fb)
        return FILE_BUF_ERR_NULL_STRUCT;
    if (!byte)
        return FILE_BUF_ERR_NULL_ARG;

    if (fb->buffer_min == fb->buffer_max) {
        if ((ret=buffer_refill(fb))!=FILE_BUF_OKAY)
            return ret;
    }
    if (fb->buffer_min == fb->buffer_max)
        return FILE_BUF_ERR_PAST_END;

    //fprintf(stderr,"getting %" OFF_T_FORMAT "\n",buffer_tell(fb));
    *byte = fb->buffer[fb->buffer_min++];
    return FILE_BUF_OKAY;
}

int buffer_look_ahead(struct file_buf_t *fb, uint8_t * bytes, size_t count)
{
    if (!fb)
        return FILE_BUF_ERR_NULL_STRUCT;
    if (!bytes)
        return FILE_BUF_ERR_NULL_ARG;
    if (count >= fb->buffer_malloc)
        return FILE_BUF_ERR_COUNT_EXCEEDS_BUFFER_SIZE;

    if (fb->buffer_min + count > fb->buffer_max)
        buffer_refill(fb);
    if (fb->buffer_min + count > fb->buffer_max)
        return FILE_BUF_ERR_PAST_END;

    memcpy(bytes,fb->buffer+fb->buffer_min,count);
    /*
     * for debugging:
    for (i = 0; i < count; i++)
    {
        *(bytes++) = fb->buffer[fb->buffer_min + i];
        //fprintf(stderr,"\tsaw: 0x%02X\n",buffer[buffer_min+i]);
    }
    */

    return FILE_BUF_OKAY;
}

/* vi:set ai ts=4 sw=4 expandtab: */

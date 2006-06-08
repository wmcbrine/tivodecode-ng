/*
#
# Header for Utilities for seeking around in a file stream
#
# $Id: file_buffer.h,v 1.1 2006-06-08 04:48:47 jeremyd Exp $
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
#ifndef _FILE_BUFFER_H_
#define _FILE_BUFFER_H_


#ifdef HAVE_CONFIG_H
# include "config.h"
# ifdef HAVE_STDINT_H
#  include <stdint.h>
# endif
# ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
# endif
# ifdef HAVE_STRING_H
#  include <string.h> /* memmove, memcpy */
# endif
# ifdef HAVE_STDLIB_H
#  include <stdlib.h> /* off_t */
# endif
#else
# include <stdint.h>
# include <string.h>
# include <stdlib.h> /* off_t */
#endif

// This seems to be needed to catch the largefile functions in stdio.h?
#define __USE_LARGEFILE

#include <stdio.h> /* fopen, fread, fseek*, fclose */

/* figure out which fseek/ftell we need */
#undef FSEEK
#undef FTELL
#ifdef HAVE_FSEEKO
# define FSEEK      fseeko
# define FSEEK_NAME "fseeko"
# define FTELL      ftello
# define FTELL_NAME "ftello"
#else
# define FSEEK      fseek
# define FSEEK_NAME "fseek"
# define FTELL      ftell
# define FTELL_NAME "ftell"
#endif

/* figure out off_t formatting */
#undef OFF_T_FORMAT
#undef ATOL
#if _FILE_OFFSET_BITS==64 || defined(__NetBSD__)
# define OFF_T_FORMAT  "llu"
# define ATOL(arg)     atoll(arg)
#else
# warning "Not compiling for large file (>2G) support!"
# define OFF_T_FORMAT  "lu"
# define ATOL(arg)     atol(arg)
#endif

#define DEFAULT_FILE_BUFFER_SIZE    	(1024*512)

#define FILE_BUF_OKAY                            0
#define FILE_BUF_ERR_NULL_STRUCT                -1
#define FILE_BUF_ERR_NULL_ARG                   -2
#define FILE_BUF_ERR_READ                       -3
#define FILE_BUF_ERR_SEEK                       -4
#define FILE_BUF_ERR_OPEN                       -5
#define FILE_BUF_ERR_PAST_END                   -6
#define FILE_BUF_ERR_NEED_POSITIVE_COUNT        -7
#define FILE_BUF_ERR_COUNT_EXCEEDS_BUFFER_SIZE  -8

typedef struct file_buf_t file_buf;

// A NULL return from "buffer_start" indicates that errno has been set,
// and calls to "perror" and the like are valid.
// Passing a previously-return file_buf into buffer_start will re-allocate it
file_buf * buffer_start(file_buf *fb, char *filename, size_t size);
int buffer_look_ahead(file_buf *fb, uint8_t * bytes, size_t count);
int buffer_get_byte(file_buf *fb, uint8_t * byte);
off_t buffer_tell(file_buf *fb);
off_t buffer_file_size(file_buf *fb);
int buffer_seek(file_buf *fb, off_t location);
int buffer_end(file_buf *fb);

// can point to a file offset to refill from
int buffer_refill(file_buf *fb);

#endif /* _FILE_BUFFER_H_ */

/* vi:set ai ts=4 sw=4 expandtab: */

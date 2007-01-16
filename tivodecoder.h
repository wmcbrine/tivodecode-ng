/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */

#ifndef TIVO_DECODER_H_
#define TIVO_DECODER_H_
#include <stddef.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "turing_stream.h"

#if _FILE_OFFSET_BITS==64 || defined(__NetBSD__) || defined(__APPLE__)
# define OFF_T_TYPE off_t
# define OFF_T_FORMAT  "llu"
#elif defined(WIN32)
# define OFF_T_TYPE __int64
# define OFF_T_FORMAT  "llu"
#else
# warning "Not compiling for large file (>2G) support!"
# define OFF_T_TYPE off_t
# define OFF_T_FORMAT  "lu"
#endif

typedef int (*write_func_t) (void * mem, int size, void * fh);

/*
 * called for each frame
 */
int process_frame(unsigned char code, turing_state * turing, OFF_T_TYPE packet_start, void * packet_stream, read_func_t read_handler, void * ofh, write_func_t write_handler);

#endif

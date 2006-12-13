/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */
#include <stddef.h>
#include "tivo-parse.h"

#ifndef TIVO_DECODER_H_
#define TIVO_DECODER_H_

#if _FILE_OFFSET_BITS==64 || defined(__NetBSD__) || defined(__APPLE__)
# define OFF_T_FORMAT  "llu"
# define ATOL(arg)     atoll(arg)
#else
# warning "Not compiling for large file (>2G) support!"
# define OFF_T_FORMAT  "lu"
# define ATOL(arg)     atol(arg)
#endif

typedef int (*write_func_t) (void * mem, int size, void * fh);

/*
 * called for each frame
 */
int process_frame(unsigned char code, turing_state * turing, off_t packet_start, void * packet_stream, read_func_t read_handler, void * ofh, write_func_t write_handler);

#endif

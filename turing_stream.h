/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */

#ifndef TURING_STREAM_H_
#define TURING_STREAM_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "Turing.h"
#include "tivo-parse.h"

#if defined(HAVE_SIZE_T) && SIZEOF_SIZE_T == 8
typedef size_t td_uint64_t;
#define HAVE_TD_UINT64_T 1
#elif defined(HAVE_UNSIGNED_LONG) && SIZEOF_UNSIGNED_LONG == 8
typedef unsigned long td_uint64_t;
#define HAVE_TD_UINT64_T 1
#elif defined(HAVE_UNSIGNED_LONG_LONG) && SIZEOF_UNSIGNED_LONG_LONG == 8
typedef unsigned long long td_uint64_t;
#define HAVE_TD_UINT64_T 1
#else
typedef unsigned long td_uint64_t;
/* #undef HAVE_TD_UINT64_T */
#endif

typedef struct turing_state_stream
{
    unsigned int cipher_pos;
    unsigned int cipher_len;

    unsigned int block_id;
    unsigned char stream_id;

    struct turing_state_stream * next;

    void * internal;
    unsigned char cipher_data[MAXSTREAM + sizeof(td_uint64_t)];
} turing_state_stream;

typedef struct
{
	unsigned char turingkey[20];

	turing_state_stream * active;
} turing_state;

unsigned int init_turing_from_file(turing_state * turing, void * tivofile, read_func_t read_handler, char * mak);
void setup_metadata_key(turing_state * turing, tivo_stream_chunk * xml, char * mak);
void setup_turing_key(turing_state * turing, tivo_stream_chunk * xml, char * mak);
void prepare_frame(turing_state * turing, unsigned char stream_id, int block_id);
void decrypt_buffer(turing_state * turing, unsigned char * buffer, size_t buffer_length);
void skip_turing_data(turing_state * turing, size_t bytes_to_skip);
void destruct_turing(turing_state * turing);

#ifdef __cplusplus
}
#endif
#endif // TURING_STREAM_H_


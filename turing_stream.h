/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */

#ifndef TURING_STREAM_H_
#define TURING_STREAM_H_

#include <stdio.h>
#include "Turing.h"
#include "happyfile.h"

typedef struct turing_state_stream
{
    int cipher_pos;
    int cipher_len;

    int block_id;
    unsigned char stream_id;

    struct turing_state_stream * next;

    void * internal;
    unsigned char cipher_data[MAXSTREAM];
} turing_state_stream;

typedef struct
{
	unsigned char turingkey[20];

	turing_state_stream * active;
} turing_state;

off_t setup_turing_key(turing_state * turing, happy_file * tivofile, char * mak);
void prepare_frame(turing_state * turing, unsigned char stream_id, int block_id);
void decrypt_buffer(turing_state * turing, unsigned char * buffer, size_t buffer_length);
void destruct_turing(turing_state * turing);

#endif // TURING_STREAM_H_


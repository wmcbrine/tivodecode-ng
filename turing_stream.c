/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */

/*
 * Decoder for Turing
 *
 * Copyright C 2002, Qualcomm Inc. Written by Greg Rose
 * Hacked into use for TiVoToGo decoding by FDM, 2005.
 * Majorly re-hacked into use for TiVoToGo decoding by Jeremy Drake, 2005.
 */

/*
This software is free for commercial and non-commercial use subject to
the following conditions:

1.  Copyright remains vested in QUALCOMM Incorporated, and Copyright
notices in the code are not to be removed.  If this package is used in
a product, QUALCOMM should be given attribution as the author of the
Turing encryption algorithm. This can be in the form of a textual
message at program startup or in documentation (online or textual)
provided with the package.

2.  Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

a. Redistributions of source code must retain the copyright notice,
   this list of conditions and the following disclaimer.

b. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

c. All advertising materials mentioning features or use of this
   software must display the following acknowledgement:  This product
   includes software developed by QUALCOMM Incorporated.

3.  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND AGAINST
INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

4.  The license and distribution terms for any publically available version
or derivative of this code cannot be changed, that is, this code cannot
simply be copied and put under another distribution license including
the GNU Public License.

5.  The Turing family of encryption algorithms are covered by patents in
the United States of America and other countries. A free and
irrevocable license is hereby granted for the use of such patents to
the extent required to utilize the Turing family of encryption
algorithms for any purpose, subject to the condition that any
commercial product utilising any of the Turing family of encryption
algorithms should show the words "Encryption by QUALCOMM" either on the
product or in the associated documentation.
*/

#include "Turing.h"		/* interface definitions */

/* testing and timing harness */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "hexlib.h"
#include "sha1.h"
#include "tivo-parse.h"
#include "turing_stream.h"

// from tivodecode.c, verbose option
extern int o_verbose;

unsigned int setup_turing_key(turing_state * turing, void * tivofile, read_func_t read_handler, char * mak)
{
    blob xml;
    SHA1_CTX context;
    unsigned int mpeg_off;

    sha1_init(&context);
    sha1_update(&context, (unsigned char *)mak, strlen(mak));

    mpeg_off = parse_tivo(tivofile, &xml, read_handler);

    sha1_update(&context, xml.data, xml.size);
    sha1_final(turing->turingkey, &context);

    free (xml.data);

    return mpeg_off;
}

static void prepare_frame_helper(turing_state * turing, unsigned char stream_id, int block_id)
{
    SHA1_CTX context;
    unsigned char turkey[20];
    unsigned char turiv [20];

    if (stream_id != turing->active->stream_id || block_id != turing->active->block_id)
    {
        turing->active->stream_id = stream_id;
        turing->active->block_id = block_id;

        turing->turingkey[16] = stream_id;
        turing->turingkey[17] = (block_id & 0xFF0000) >> 16;
        turing->turingkey[18] = (block_id & 0x00FF00) >> 8;
        turing->turingkey[19] = (block_id & 0x0000FF) >> 0;

        sha1_init(&context);
        sha1_update(&context, turing->turingkey, 17);
        sha1_final(turkey, &context);
        //hexbulk(turkey, 20);

        sha1_init(&context);
        sha1_update(&context, turing->turingkey, 20);
        sha1_final(turiv, &context);
        //hexbulk(turiv , 20);

        turing->active->cipher_pos = 0;

        TuringKey(turing->active->internal, turkey, 20);
        TuringIV(turing->active->internal, turiv, 20);

        memset(turing->active->cipher_data, 0, MAXSTREAM);

        turing->active->cipher_len = TuringGen(turing->active->internal, turing->active->cipher_data);

        //hexbulk(turing->active->cipher_data, turing->active->cipher_len);
    }
}

void prepare_frame(turing_state * turing, unsigned char stream_id, int block_id)
{
    if (turing->active)
    {
        if (turing->active->stream_id != stream_id)
        {
            turing_state_stream * start = turing->active;
            do
            {
                turing->active = turing->active->next;
            }
            while (turing->active->stream_id != stream_id && turing->active != start);

            if (turing->active->stream_id != stream_id)
            {
                // did not find a state for this stream type
                turing->active = calloc (1, sizeof (turing_state_stream));
                turing->active->next = start->next;
                start->next = turing->active;
                turing->active->internal = TuringAlloc();
                if (o_verbose)
                    fprintf(stderr, "Creating turing stream for packet type %02x\n", stream_id);
            }
        }
    }
    else
    {
        turing->active = calloc (1, sizeof (turing_state_stream));
        turing->active->next = turing->active;
        turing->active->internal = TuringAlloc();
        if (o_verbose)
            fprintf(stderr, "Creating turing stream for packet type %02x\n", stream_id);
    }

    prepare_frame_helper(turing, stream_id, block_id);
}

void decrypt_buffer(turing_state * turing, unsigned char * buffer, size_t buffer_length)
{
    unsigned int i;

    for (i = 0; i < buffer_length; ++i)
    {
        if (turing->active->cipher_pos >= turing->active->cipher_len)
        {
            turing->active->cipher_len = TuringGen(turing->active->internal, turing->active->cipher_data);
            turing->active->cipher_pos = 0;
        }

        buffer[i] ^= turing->active->cipher_data[turing->active->cipher_pos++];
    }
}

void destruct_turing(turing_state * turing)
{
    if (turing->active)
    {
        turing_state_stream * start = turing->active;
        do
        {
            turing_state_stream * prev = turing->active;
            TuringFree(turing->active->internal);
            turing->active = turing->active->next;
            free (prev);
        }
        while (turing->active != start);
    }

    turing->active = NULL;
}

/* vi:set ai ts=4 sw=4 expandtab: */

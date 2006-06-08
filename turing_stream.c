/*
 * Decoder for Turing
 *
 * Copyright C 2002, Qualcomm Inc. Written by Greg Rose
 * Hacked into use for TiVoToGo decoding by FDM, 2005.
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

off_t setup_turing_key(turing_state * turing, char * tivofile, char * mak)
{
    blob xml;
    SHA1_CTX context;
    off_t mpeg_off;

    sha1_init(&context);
    sha1_update(&context, mak, strlen(mak));

    mpeg_off = parse_tivo(tivofile, &xml);

    sha1_update(&context, xml.data, xml.size);
    sha1_final(turing->turingkey, &context);

    turing->state_e0.internal = TuringAlloc();
    turing->state_c0.internal = TuringAlloc();

    return mpeg_off;
}

void prepare_frame_helper(turing_state * turing, char stream_id, int block_id)
{
    SHA1_CTX context;
    char turkey[20];
    char turiv [20];

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

        /* basic test */
        TuringKey(turing->active->internal, turkey, 20);
        TuringIV(turing->active->internal, turiv, 20);

        /* start stream */
        memset(turing->active->cipher_data, 0, MAXSTREAM);

        turing->active->cipher_len = TuringGen(turing->active->internal, turing->active->cipher_data);

        //hexbulk(turing->active->cipher_data, turing->active->cipher_len);
    }
}

void prepare_frame(turing_state * turing, unsigned char stream_id, int block_id)
{
    if (stream_id == 0xe0)
    {
        turing->active = &turing->state_e0;
        prepare_frame_helper(turing, stream_id, block_id);
    }
    else if (stream_id == 0xc0)
    {
        turing->active = &turing->state_c0;
        prepare_frame_helper(turing, stream_id, block_id);
    }
    else
    {
        fprintf(stderr, "Unexpected crypted frame type: %02x\n", stream_id);
        exit(42);
    }
}

void decrypt_buffer(turing_state * turing, char * buffer, size_t buffer_length)
{
    int i;

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

/* vi:set ai ts=4 sw=4 expandtab: */

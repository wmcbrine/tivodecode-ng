/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
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

#include <iostream>
#include <cstring>

#include "hexlib.hxx"
#include "md5.hxx"
#include "sha1.hxx"
#include "Turing.hxx"		/* interface definitions */
#include "turing_stream.hxx"

void TuringState::setup_key(uint8_t *buffer, size_t buffer_length,
                            char *mak)
{
    SHA1 context;

    context.init();
    context.update((uint8_t *)mak, strlen(mak));
    context.update(buffer, buffer_length);
    context.final(turingkey);
}

#define static_strlen(str) (sizeof(str) - 1)

void TuringState::setup_metadata_key(uint8_t *buffer,
                                     size_t buffer_length, char *mak)
{
    static const char lookup[] = "0123456789abcdef";
    static const uint8_t media_mak_prefix[] = "tivo:TiVo DVR:";
    MD5 md5;
    int i;
    uint8_t md5result[16];
    char metakey[33];

    md5.init();
    md5.loop(media_mak_prefix, static_strlen(media_mak_prefix));
    md5.loop((uint8_t *)mak, strlen(mak));

    md5.pad();
    md5.result(md5result);

    for (i = 0; i < 16; ++i)
    {
        metakey[2*i + 0] = lookup[(md5result[i] >> 4) & 0xf];
        metakey[2*i + 1] = lookup[ md5result[i]       & 0xf];
    }
    metakey[32] = '\0';

    /* this is done the same as the normal one, only replacing the mak
     * with the metakey
     */
    setup_key(buffer, buffer_length, metakey);
}

void TuringState::prepare_frame_helper(uint8_t stream_id, int block_id)
{
    SHA1 context;
    uint8_t turkey[20];
    uint8_t turiv [20];

    active->stream_id = stream_id;
    active->block_id = block_id;

    turingkey[16] = stream_id;
    turingkey[17] = (block_id & 0xFF0000) >> 16;
    turingkey[18] = (block_id & 0x00FF00) >> 8;
    turingkey[19] = (block_id & 0x0000FF);

    context.init();
    context.update(turingkey, 17);
    context.final(turkey);
    //hexbulk(turkey, 20);

    context.init();
    context.update(turingkey, 20);
    context.final(turiv);
    //hexbulk(turiv, 20);

    active->cipher_pos = 0;

    active->internal->key(turkey, 20);
    active->internal->IV(turiv, 20);

    std::memset(active->cipher_data, 0, MAXSTREAM);

    active->cipher_len = 0;
}

#define CREATE_TURING_LISTITM(nxt, stream_id, block_id) \
    do { \
        active = new turing_state_stream; \
        active->next = (nxt); \
        (nxt) = active; \
        active->internal = new Turing; \
        prepare_frame_helper((stream_id), (block_id)); \
    } while(0)

void TuringState::prepare_frame(uint8_t stream_id, int block_id)
{
    if (active)
    {
        if (active->stream_id != stream_id)
        {
            turing_state_stream *start = active;
            do
            {
                active = active->next;
            }
            while (active->stream_id != stream_id && active != start);

            if (active->stream_id != stream_id)
            {
                /* did not find a state for this stream type */
                CREATE_TURING_LISTITM ((start->next), stream_id, block_id);
            }
        }

        if (active->block_id != (unsigned int) block_id)
            prepare_frame_helper(stream_id, block_id);
    }
    else
    {
        /* first stream type seen */
        CREATE_TURING_LISTITM (active, stream_id, block_id);
    }
}

void TuringState::decrypt_buffer(uint8_t *buffer, size_t buffer_length)
{
    unsigned int i;

    for (i = 0; i < buffer_length; ++i)
    {
        if (active->cipher_pos >= active->cipher_len)
        {
            active->cipher_len = active->internal->gen(active->cipher_data);
            active->cipher_pos = 0;
            //hexbulk(active->cipher_data, active->cipher_len);
        }

        buffer[i] ^= active->cipher_data[active->cipher_pos++];
    }
}

void TuringState::skip_data(size_t bytes_to_skip)
{
    if (active->cipher_pos + bytes_to_skip < (size_t)active->cipher_len)
        active->cipher_pos += (int)bytes_to_skip;
    else
    {
        do
        {
            bytes_to_skip -= active->cipher_len - active->cipher_pos;
            active->cipher_len = active->internal->gen(active->cipher_data);
            active->cipher_pos = 0;
        } while (bytes_to_skip >= (size_t)active->cipher_len);

        active->cipher_pos = (int)bytes_to_skip;
    }
}

void TuringState::destruct()
{
    if (active)
    {
        turing_state_stream *start = active;
        do
        {
            turing_state_stream *prev = active;
            delete active->internal;
            active = active->next;
            if (prev)
                delete prev;
        }
        while (active != start);
    }

    active = NULL;
}

void TuringState::dump()
{
    std::cerr << "turing dump :\n";
    std::cerr << "turingKey   :\n";
    hexbulk(turingkey, 20);

    std::cerr << "active      : " << &active << "\n";

    if (active)
    {
        std::cerr << "cipher_pos  : " << active->cipher_pos << "\n"
            "cipher_len  : " << active->cipher_len << "\n"
            "block_id    : " << active->block_id << "\n"
            "stream_id   : " << active->stream_id << "\n"
            "next        : " << active->next << "\n"
            "internal    : " << active->internal << "\n"
            "cipher_data :\n";
        hexbulk(active->cipher_data, MAXSTREAM + sizeof(uint64_t));
    }

    std::cerr << "\n\n";
}

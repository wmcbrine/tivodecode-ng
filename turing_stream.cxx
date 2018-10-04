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

#include "hexlib.hxx"
#include "md5.hxx"
#include "sha1.hxx"
#include "Turing.hxx"		/* interface definitions */
#include "turing_stream.hxx"

void TuringStateStream::decrypt_buffer(uint8_t *buffer, size_t buffer_length)
{
    for (size_t i = 0; i < buffer_length; ++i)
    {
        if (cipher_pos >= cipher_len)
        {
            cipher_len = internal.gen(cipher_data);
            cipher_pos = 0;
            //hexbulk(cipher_data, cipher_len);
        }

        buffer[i] ^= cipher_data[cipher_pos++];
    }
}

void TuringStateStream::skip_data(size_t bytes_to_skip)
{
    if (cipher_pos + bytes_to_skip < (size_t)cipher_len)
        cipher_pos += (int)bytes_to_skip;
    else
    {
        do
        {
            bytes_to_skip -= cipher_len - cipher_pos;
            cipher_len = internal.gen(cipher_data);
            cipher_pos = 0;
        } while (bytes_to_skip >= (size_t)cipher_len);

        cipher_pos = (int)bytes_to_skip;
    }
}

TuringState::TuringState()
{
    active = NULL;
}

TuringState::~TuringState()
{
    if (active)
    {
        TuringStateStream *start = active;
        do
        {
            TuringStateStream *prev = active;
            active = active->next;
            if (prev)
                delete prev;
        }
        while (active != start);
    }

    active = NULL;
}

void TuringStateStream::dump()
{
    std::cerr << "cipher_pos  : " << cipher_pos << "\n"
                 "cipher_len  : " << cipher_len << "\n"
                 "block_id    : " << block_id << "\n"
                 "stream_id   : " << stream_id << "\n"
                 "next        : " << next << "\n"
                 "cipher_data :\n";
    hexbulk(cipher_data, MAXSTREAM + sizeof(uint64_t));
}

void TuringState::setup_key(uint8_t *buffer, size_t buffer_length,
                            const std::string &mak)
{
    SHA1 context;

    context.init();
    context.update((const uint8_t *)mak.data(), mak.size());
    context.update(buffer, buffer_length);
    context.final(turingkey);
}

void TuringState::setup_metadata_key(uint8_t *buffer,
                                     size_t buffer_length,
                                     const std::string &mak)
{
    static const char lookup[] = "0123456789abcdef";
    MD5 md5;
    int i;
    uint8_t md5result[16];
    std::string metakey = "";

    md5.init();
    md5.loop("tivo:TiVo DVR:");
    md5.loop(mak);

    md5.pad();
    md5.result(md5result);

    for (i = 0; i < 16; ++i)
    {
        metakey += lookup[(md5result[i] >> 4) & 0xf];
        metakey += lookup[ md5result[i]       & 0xf];
    }

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

    active->internal.key(turkey, 20);
    active->internal.IV(turiv, 20);

    active->cipher_len = 0;
}

void TuringState::prepare_frame(uint8_t stream_id, int block_id)
{
    if (active)
    {
        if (active->stream_id != stream_id)
        {
            TuringStateStream *start = active;
            do
            {
                active = active->next;
            }
            while (active->stream_id != stream_id && active != start);

            if (active->stream_id != stream_id)
            {
                /* did not find a state for this stream type */

                active = new TuringStateStream;
                active->next = start->next;
                start->next = active;
                prepare_frame_helper(stream_id, block_id);
            }
        }

        if (active->block_id != (unsigned int) block_id)
            prepare_frame_helper(stream_id, block_id);
    }
    else
    {
        /* first stream type seen */

        active = new TuringStateStream;
        active->next = active;            // hmm
        prepare_frame_helper(stream_id, block_id);
    }
}

void TuringState::dump()
{
    std::cerr << "turing dump :\n";
    std::cerr << "turingKey   :\n";
    hexbulk(turingkey, 20);

    std::cerr << "active      : " << &active << "\n";

    if (active)
        active->dump();

    std::cerr << "\n\n";
}

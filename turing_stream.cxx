/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

/*
 * Decoder for Turing
 *
 * Copyright 2002, Qualcomm Inc. Written by Greg Rose
 * Hacked into use for TiVoToGo decoding by FDM, 2005.
 * Majorly re-hacked into use for TiVoToGo decoding by Jeremy Drake, 2005.
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

    context.init();
    context.update(turingkey, 20);
    context.final(turiv);

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

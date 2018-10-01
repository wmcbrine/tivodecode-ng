/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */

#ifndef TURING_STREAM_H_
#define TURING_STREAM_H_

#include <string>

/*following copied from Turing.h, so we can avoid including it here */
#define MAXSTREAM   340 /* bytes, maximum stream generated by one call */

class TuringStateStream
{
    public:
        unsigned int cipher_pos;
        unsigned int cipher_len;

        unsigned int block_id;
        uint8_t stream_id;

        TuringStateStream *next;

        Turing *internal;
        uint8_t cipher_data[MAXSTREAM + sizeof(uint64_t)];

        void decrypt_buffer(uint8_t *buffer, size_t buffer_length);
        void skip_data(size_t bytes_to_skip);
};

class TuringState
{
    private:
        uint8_t turingkey[20];

    public:
        TuringStateStream *active;

        void setup_key(uint8_t *buffer, size_t buffer_length,
                       const std::string &mak);
        void setup_metadata_key(uint8_t *buffer, size_t buffer_length,
                                const std::string &mak);
        void prepare_frame_helper(uint8_t stream_id, int block_id);
        void prepare_frame(uint8_t stream_id, int block_id);
        void destruct();
        void dump();
};

#endif // TURING_STREAM_H_

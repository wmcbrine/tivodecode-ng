/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING.md for license terms
 */

#ifndef TIVO_DECODER_PS_HXX_
#define TIVO_DECODER_PS_HXX_

#include "tivo_decoder_base.hxx"

//============================
// PS Specific data structures
//============================

typedef enum
{
    PACK_NONE,
    PACK_SPECIAL,
    PACK_PES_SIMPLE,            // packet length == data length
    PACK_PES_COMPLEX            // crazy headers need skipping
}
packet_type;

typedef struct
{
    // the byte value match for the packet tags
    uint8_t code_match_lo;      // low end of the range of matches
    uint8_t code_match_hi;      // high end of the range of matches

    // what kind of PES is it?
    packet_type packet;
}
packet_tag_info;

class TiVoDecoderPS : public TiVoDecoder
{
    private:
        uint32_t marker;

        void frame_start(const uint8_t *bytes, uint8_t code);
        int process_frame(uint8_t code);
    public:
        virtual bool process();

        TiVoDecoderPS(TuringState &pTuringState, HappyFile &pInfile,
                      HappyFile &pOutfile);
        ~TiVoDecoderPS();
};

#endif /* TIVO_DECODER_PS_HXX_ */

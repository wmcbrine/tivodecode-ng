/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#ifndef TIVO_DECODER_BASE_HXX_
#define TIVO_DECODER_BASE_HXX_

#include "tivo_parse.hxx"
#include "turing_stream.hxx"

class TiVoDecoder
{
    protected:
        bool         running;
        bool         isValid;

    public:
        TuringState &pTuring;
        HappyFile   *pFileIn;
        HappyFile   *pFileOut;

        int do_header(const uint8_t *arg_0, int &block_no, int &crypted);

        virtual bool process() = 0;

        TiVoDecoder(TuringState &pTuringState, HappyFile *pInfile,
                    HappyFile *pOutfile);
        virtual ~TiVoDecoder();
};

#endif /* TIVO_DECODER_BASE_HXX_ */

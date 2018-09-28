#ifndef TIVO_DECODER_HXX_
#define TIVO_DECODER_HXX_

#include "tivo_parse.hxx"
#include "turing_stream.hxx"

class TiVoDecoder
{
    public:

        bool         running;
        bool         isValid;
        TuringState *pTuring;
        HappyFile   *pFileIn;
        HappyFile   *pFileOut;

        int do_header(uint8_t *arg_0, int *block_no, int *arg_8,
                      int *crypted, int *arg_10, int *arg_14);

        virtual bool process() = 0;

        TiVoDecoder(TuringState *pTuringState, HappyFile *pInfile,
                    HappyFile *pOutfile);
        virtual ~TiVoDecoder();
};

#endif /* TIVO_DECODER_HXX_ */

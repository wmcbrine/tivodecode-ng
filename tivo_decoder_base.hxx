#ifndef TIVO_DECODER_HXX_
#define TIVO_DECODER_HXX_

#include <cstdio>

#include "tivo_parse.hxx"
#include "turing_stream.hxx"

#define LOOK_AHEAD(fh, bytes, n) do {\
    int retval = fh->read((bytes) + looked_ahead, (n) - looked_ahead);\
    if ( retval == 0 )\
    {\
        return 0;  \
    }\
    else if ( retval != (n) - looked_ahead) { \
        perror ("read"); \
        return -1; \
    } else { \
        looked_ahead = (n); \
    } \
} while (0)

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

/* vi:set ai ts=4 sw=4 expandtab: */

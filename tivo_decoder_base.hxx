#ifndef TIVO_DECODER_HXX_
#define TIVO_DECODER_HXX_

#ifdef HAVE_CONFIG_H
#include "tdconfig.h"
#endif

#include <cstdio>

#include "tivo_types.hxx"
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

/* All elements are in big-endian format and are packed */

class TiVoDecoder
{
    public:

        BOOL         running;
        BOOL         isValid;
        TuringState *pTuring;
        HappyFile   *pFileIn;
        FILE        *pFileOut;

        int do_header(UINT8 *arg_0, int *block_no, int *arg_8,
                      int *crypted, int *arg_10, int *arg_14);

        virtual BOOL process() = 0;

        TiVoDecoder(TuringState *pTuringState, HappyFile *pInfile,
                    FILE *pOutfile);
        virtual ~TiVoDecoder();

} __attribute__((packed));

#endif /* TIVO_DECODER_HXX_ */

/* vi:set ai ts=4 sw=4 expandtab: */

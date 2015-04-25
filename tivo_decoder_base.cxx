/*
* tivodecode, (c) 2006, Jeremy Drake
* See COPYING file for license terms
*/

#ifdef HAVE_CONFIG_H
#include "tdconfig.h"
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "tivo_decoder_base.hxx"


TiVoDecoder::TiVoDecoder(TuringState *pTuringState, happy_file *pInfile,
                         hoff_t fileOffset, FILE *pOutfile)
{
    isValid = FALSE;

    if(!pTuringState || !pInfile || !pOutfile )
        return;
        
    if(hseek(pInfile, fileOffset, SEEK_SET) < 0)
    {
        perror("seek");
        return;
    }    
    
    running       = TRUE;
    pFileIn       = pInfile;
    pFileOut      = pOutfile;
    pTuring       = pTuringState;
    isValid       = TRUE;
}

TiVoDecoder::~TiVoDecoder()
{
    
}


/**
 * This is from analyzing the TiVo directshow dll.  Most of the parameters I have no idea what they are for.
 *
 * @param arg_0     pointer to the 16 byte private data section of the packet header.
 * @param block_no  pointer to an integer to contain the block number used in the turing key
 * @param arg_8     no clue
 * @param crypted   pointer to an integer to contain 4 bytes of data encrypted
 *                  with the same turing cipher as the video.  No idea what to do with it once
 *                  it is decrypted, tho, but the turing needs to have 4 bytes
 *                  consumed in order to line up with the video/audio data.  My
 *                  guess is it is a checksum of some sort.
 * @param arg_10    no clue
 * @param arg_14    no clue
 *
 * @return count of particular bits which are zero.  They should all be 1, so the return value should be zero.
 *         I would consider a non-zero return an error.
 */
int TiVoDecoder::do_header(UINT8 * arg_0, int * block_no, int * arg_8, int * crypted, int * arg_10, int * arg_14)
{
    int var_4 = 0;

    if (!(arg_0[0] & 0x80))
        var_4++;

    if (arg_10)
    {
        *arg_10 = (arg_0[0x0] & 0x78) >> 3;
    }

    if (arg_14)
    {
        *arg_14  = (arg_0[0x0] & 0x07) << 1;
        *arg_14 |= (arg_0[0x1] & 0x80) >> 7;
    }

    if (!(arg_0[1] & 0x40))
        var_4++;

    if (block_no)
    {
        *block_no  = (arg_0[0x1] & 0x3f) << 0x12;
        *block_no |= (arg_0[0x2] & 0xff) << 0xa;
        *block_no |= (arg_0[0x3] & 0xc0) << 0x2;

        if (!(arg_0[3] & 0x20))
            var_4++;

        *block_no |= (arg_0[0x3] & 0x1f) << 0x3;
        *block_no |= (arg_0[0x4] & 0xe0) >> 0x5;
    }

    if (!(arg_0[4] & 0x10))
        var_4++;

    if (arg_8)
    {
        *arg_8  = (arg_0[0x4] & 0x0f) << 0x14;
        *arg_8 |= (arg_0[0x5] & 0xff) << 0xc;
        *arg_8 |= (arg_0[0x6] & 0xf0) << 0x4;

        if (!(arg_0[6] & 0x8))
            var_4++;

        *arg_8 |= (arg_0[0x6] & 0x07) << 0x5;
        *arg_8 |= (arg_0[0x7] & 0xf8) >> 0x3;
    }

    if (crypted)
    {
        *crypted  = (arg_0[0xb] & 0x03) << 0x1e;
        *crypted |= (arg_0[0xc] & 0xff) << 0x16;
        *crypted |= (arg_0[0xd] & 0xfc) << 0xe;

        if (!(arg_0[0xd] & 0x2))
            var_4++;

        *crypted |= (arg_0[0xd] & 0x01) << 0xf;
        *crypted |= (arg_0[0xe] & 0xff) << 0x7;
        *crypted |= (arg_0[0xf] & 0xfe) >> 0x1;
    }

    if (!(arg_0[0xf] & 0x1))
        var_4++;

    return var_4;
}


/* vi:set ai ts=4 sw=4 expandtab: */


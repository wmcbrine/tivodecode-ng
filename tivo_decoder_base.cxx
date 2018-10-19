/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING.md for license terms
 */

#include "tivo_decoder_base.hxx"

TiVoDecoder::TiVoDecoder(TuringState &pTuringState, HappyFile &pInfile,
                         HappyFile &pOutfile) : pTuring(pTuringState),
                         pFileIn(pInfile), pFileOut(pOutfile)
{
    running  = true;
    isValid  = true;
}

TiVoDecoder::~TiVoDecoder()
{
}

/**
 * This is from analyzing the TiVo directshow dll.  Most of the
 * parameters I have no idea what they are for.
 *
 * @param arg_0     pointer to the 16 byte private data section of the
 *                  packet header.
 * @param block_no  pointer to an integer to contain the block number used
 *                  in the turing key
 * @param crypted   pointer to an integer to contain 4 bytes of data encrypted
 *                  with the same turing cipher as the video.  No idea what to
 *                  do with it once it is decrypted, tho, but the turing needs
 *                  to have 4 bytes consumed in order to line up with the
 *                  video/audio data.  My guess is it is a checksum of some
 *                  sort.
 *
 * @return count of particular bits which are zero.  They should all be 1,
 * so the return value should be zero. I would consider a non-zero
 * return an error.
 */

int TiVoDecoder::do_header(const uint8_t *arg_0, int &block_no, int &crypted)
{
    block_no = ((arg_0[0x1] & 0x3f) << 0x12)
             | ((arg_0[0x2] & 0xff) << 0xa)
             | ((arg_0[0x3] & 0xc0) << 0x2)
             | ((arg_0[0x3] & 0x1f) << 0x3)
             | ((arg_0[0x4] & 0xe0) >> 0x5);

    crypted = ((arg_0[0xb] & 0x03) << 0x1e)
            | ((arg_0[0xc] & 0xff) << 0x16)
            | ((arg_0[0xd] & 0xfc) << 0xe)
            | ((arg_0[0xd] & 0x01) << 0xf)
            | ((arg_0[0xe] & 0xff) << 0x7)
            | ((arg_0[0xf] & 0xfe) >> 0x1);

    return !(arg_0[0] & 0x80) + !(arg_0[1] & 0x40) + !(arg_0[3] & 0x20) +
           !(arg_0[4] & 0x10) + !(arg_0[0xd] & 0x2) + !(arg_0[0xf] & 0x1);
}

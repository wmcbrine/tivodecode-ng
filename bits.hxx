/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING.md for license terms
 */

// Some common bitwise operations

#ifndef BITS_HXX_
#define BITS_HXX_

#include <cstdint>

// Rotate left

inline uint32_t ROTL(uint32_t value, int bits)
{
    return (value << bits) | (value >> (32 - bits));
}

// Convert big-endian byte sequences to native integers (either endian)

inline uint32_t GET32(const uint8_t *pVal)
{
    return (pVal[0] << 24) | (pVal[1] << 16) | (pVal[2] << 8) | pVal[3];
}

inline uint16_t GET16(const uint8_t *pVal)
{
    return (pVal[0] << 8) | pVal[1];
}

#endif /* BITS_HXX_ */

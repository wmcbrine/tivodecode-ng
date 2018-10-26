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

// The i-th least-significant byte of x

inline uint8_t BYTE(uint32_t x, int i)
{
    return (x >> (24 - 8 * i)) & 0xFF;
}

// Convert native integers to big-endian byte sequences

inline void PUT32(uint32_t w, uint8_t *b)
{
    b[0] = BYTE(w, 0);
    b[1] = BYTE(w, 1);
    b[2] = BYTE(w, 2);
    b[3] = BYTE(w, 3);
}

#endif /* BITS_HXX_ */

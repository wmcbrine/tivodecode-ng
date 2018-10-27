/*
 * Copyright 1995, 1996, 1997, and 1998 WIDE Project.
 * Adapted for tivodecode-ng
 * See COPYING.md for license terms
 */

#include "tdconfig.h"

#include <algorithm>

#include "bits.hxx"
#include "md5.hxx"

/* Integer part of 4294967296 times abs(sin(i)), where i is in radians. */
static const uint32_t T[65] = {
    0,
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,

    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,

    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,

    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static const uint8_t md5_paddat[MD5_BUFLEN] = {
    0x80, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

void MD5::init()
{
    md5_count.lsw = 0;
    md5_count.msw = 0;
    md5_i = 0;
    md5_st[0] = 0x67452301;
    md5_st[1] = 0xefcdab89;
    md5_st[2] = 0x98badcfe;
    md5_st[3] = 0x10325476;
}

void MD5::loop(const std::string &blk)
{
    size_t gap, i;
    const uint8_t *input = (const uint8_t *)blk.data();
    size_t len = blk.size();

    if (md5_count.lsw + len * 8 < md5_count.lsw)
        md5_count.msw++;
    md5_count.lsw += len * 8;         /* byte to bit */
    gap = MD5_BUFLEN - md5_i;

    if (len >= gap)
    {
        std::copy(input, input + gap, md5_buf + md5_i);
        calc(md5_buf);

        for (i = gap; i + MD5_BUFLEN <= len; i += MD5_BUFLEN)
            calc(input + i);

        md5_i = len - i;
        std::copy(input + i, input + i + md5_i, md5_buf);
    }
    else
    {
        std::copy(input, input + len, md5_buf + md5_i);
        md5_i += len;
    }
}

void MD5::pad()
{
    size_t gap;

    /* Don't count up padding. Keep md5_n. */
    gap = MD5_BUFLEN - md5_i;
    if (gap > 8)
        std::copy(md5_paddat, md5_paddat + gap - 8, md5_buf + md5_i);
    else
    {
        /* including gap == 8 */
        std::copy(md5_paddat, md5_paddat + gap, md5_buf + md5_i);
        calc(md5_buf);
        std::copy(md5_paddat + gap,
                  md5_paddat + gap + MD5_BUFLEN - 8, md5_buf);
    }

    /* 8 byte word */
    PUTL32(md5_count.lsw, md5_buf + 56);
    PUTL32(md5_count.msw, md5_buf + 60);

    calc(md5_buf);
}

void MD5::result(uint8_t *digest)
{
    /* 4 byte words */
    PUTL32(md5_st[0], digest);
    PUTL32(md5_st[1], digest + 4);
    PUTL32(md5_st[2], digest + 8);
    PUTL32(md5_st[3], digest + 12);
}

void MD5::calc(const uint8_t *b64)
{
    uint32_t X[16], A, B, C, D, tmp;
    int i, j;

    A = md5_st[0];
    B = md5_st[1];
    C = md5_st[2];
    D = md5_st[3];

    for (i = 0; i < 16; i++)                    // Round 1
    {
        X[i] = GETL32(&b64[i << 2]);
        A = ROTL(A + ((B & C) | (~B & D)) + X[i] + T[i + 1],
                 (i % 4) * 5 + 7) + B;
        tmp = D; D = C; C = B; B = A; A = tmp;
    }

    for (j = 1; i < 32; i++, j = (j + 5) % 16)  // Round 2
    {
        static const int r2[] = {5, 9, 14, 20};
        A = ROTL(A + ((B & D) | (C & ~D)) + X[j] + T[i + 1], r2[i % 4]) + B;
        tmp = D; D = C; C = B; B = A; A = tmp;
    }

    for (j = 5; i < 48; i++, j = (j + 3) % 16)  // Round 3
    {
        static const int r3[] = {4, 11, 16, 23};
        A = ROTL(A + (B ^ C ^ D) + X[j] + T[i + 1], r3[i % 4]) + B;
        tmp = D; D = C; C = B; B = A; A = tmp;
    }

    for (j = 0; i < 64; i++, j = (j + 7) % 16)  // Round 4
    {
        static const int r4[] = {6, 10, 15, 21};
        A = ROTL(A + (C ^ (B | ~D)) + X[j] + T[i + 1], r4[i % 4]) + B;
        tmp = D; D = C; C = B; B = A; A = tmp;
    }

    md5_st[0] += A;
    md5_st[1] += B;
    md5_st[2] += C;
    md5_st[3] += D;
}

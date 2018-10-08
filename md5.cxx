/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
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

inline void ROUND1(uint32_t *X, uint32_t &a, uint32_t b, uint32_t c,
                   uint32_t d, int k, int s, int i)
{
    a = ROTL(a + ((b & c) | (~b & d)) + X[k] + T[i], s) + b;
}

inline void ROUND2(uint32_t *X, uint32_t &a, uint32_t b, uint32_t c,
                   uint32_t d, int k, int s, int i)
{
    a = ROTL(a + ((b & d) | (c & ~d)) + X[k] + T[i], s) + b;
}

inline void ROUND3(uint32_t *X, uint32_t &a, uint32_t b, uint32_t c,
                   uint32_t d, int k, int s, int i)
{
    a = ROTL(a + (b ^ c ^ d) + X[k] + T[i], s) + b;
}

inline void ROUND4(uint32_t *X, uint32_t &a, uint32_t b, uint32_t c,
                   uint32_t d, int k, int s, int i)
{
    a = ROTL(a + (c ^ (b | ~d)) + X[k] + T[i], s) + b;
}

void MD5::init()
{
    md5_c.t64.lsw = 0;
    md5_c.t64.msw = 0;
    md5_i = 0;
    md5_s.t32[0] = 0x67452301;
    md5_s.t32[1] = 0xefcdab89;
    md5_s.t32[2] = 0x98badcfe;
    md5_s.t32[3] = 0x10325476;
}

void MD5::loop(const std::string &blk)
{
    size_t gap, i;
    const uint8_t *input = (const uint8_t *)blk.data();
    size_t len = blk.size();

    if (md5_c.t64.lsw + len * 8 < md5_c.t64.lsw)
        md5_c.t64.msw++;
    md5_c.t64.lsw += len * 8;         /* byte to bit */
    gap = MD5_BUFLEN - md5_i;

    if (len >= gap)
    {
        std::copy(input, input + gap, md5_buf + md5_i);
        calc(md5_buf);

        for (i = gap; i + MD5_BUFLEN <= len; i += MD5_BUFLEN)
            calc((uint8_t *) (input + i));

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
#ifndef WORDS_BIGENDIAN
    std::copy(md5_c.t8, md5_c.t8 + 8, md5_buf + 56);
#else
    md5_buf[56] = md5_c.t8[3];
    md5_buf[57] = md5_c.t8[2];
    md5_buf[58] = md5_c.t8[1];
    md5_buf[59] = md5_c.t8[0];
    md5_buf[60] = md5_c.t8[7];
    md5_buf[61] = md5_c.t8[6];
    md5_buf[62] = md5_c.t8[5];
    md5_buf[63] = md5_c.t8[4];
#endif

    calc(md5_buf);
}

void MD5::result(uint8_t *digest)
{
    /* 4 byte words */
#ifndef WORDS_BIGENDIAN
    std::copy(md5_s.t8, md5_s.t8 + 16, digest);
#else
    digest[0] = md5_s.t8[3];
    digest[1] = md5_s.t8[2];
    digest[2] = md5_s.t8[1];
    digest[3] = md5_s.t8[0];
    digest[4] = md5_s.t8[7];
    digest[5] = md5_s.t8[6];
    digest[6] = md5_s.t8[5];
    digest[7] = md5_s.t8[4];
    digest[8] = md5_s.t8[11];
    digest[9] = md5_s.t8[10];
    digest[10] = md5_s.t8[9];
    digest[11] = md5_s.t8[8];
    digest[12] = md5_s.t8[15];
    digest[13] = md5_s.t8[14];
    digest[14] = md5_s.t8[13];
    digest[15] = md5_s.t8[12];
#endif
}

#ifdef WORDS_BIGENDIAN
static uint32_t X[16];
#endif

void MD5::calc(uint8_t *b64)
{
    uint32_t A = md5_s.t32[0];
    uint32_t B = md5_s.t32[1];
    uint32_t C = md5_s.t32[2];
    uint32_t D = md5_s.t32[3];

#ifndef WORDS_BIGENDIAN
    uint32_t *X = (uint32_t *)b64;
#else
    /* 4 byte words */
    /* what a brute force but fast! */
    uint8_t *y = (uint8_t *)X;

    y[0] = b64[3];
    y[1] = b64[2];
    y[2] = b64[1];
    y[3] = b64[0];
    y[4] = b64[7];
    y[5] = b64[6];
    y[6] = b64[5];
    y[7] = b64[4];
    y[8] = b64[11];
    y[9] = b64[10];
    y[10] = b64[9];
    y[11] = b64[8];
    y[12] = b64[15];
    y[13] = b64[14];
    y[14] = b64[13];
    y[15] = b64[12];
    y[16] = b64[19];
    y[17] = b64[18];
    y[18] = b64[17];
    y[19] = b64[16];
    y[20] = b64[23];
    y[21] = b64[22];
    y[22] = b64[21];
    y[23] = b64[20];
    y[24] = b64[27];
    y[25] = b64[26];
    y[26] = b64[25];
    y[27] = b64[24];
    y[28] = b64[31];
    y[29] = b64[30];
    y[30] = b64[29];
    y[31] = b64[28];
    y[32] = b64[35];
    y[33] = b64[34];
    y[34] = b64[33];
    y[35] = b64[32];
    y[36] = b64[39];
    y[37] = b64[38];
    y[38] = b64[37];
    y[39] = b64[36];
    y[40] = b64[43];
    y[41] = b64[42];
    y[42] = b64[41];
    y[43] = b64[40];
    y[44] = b64[47];
    y[45] = b64[46];
    y[46] = b64[45];
    y[47] = b64[44];
    y[48] = b64[51];
    y[49] = b64[50];
    y[50] = b64[49];
    y[51] = b64[48];
    y[52] = b64[55];
    y[53] = b64[54];
    y[54] = b64[53];
    y[55] = b64[52];
    y[56] = b64[59];
    y[57] = b64[58];
    y[58] = b64[57];
    y[59] = b64[56];
    y[60] = b64[63];
    y[61] = b64[62];
    y[62] = b64[61];
    y[63] = b64[60];
#endif

    ROUND1(X, A, B, C, D, 0, 7, 1);
    ROUND1(X, D, A, B, C, 1, 12, 2);
    ROUND1(X, C, D, A, B, 2, 17, 3);
    ROUND1(X, B, C, D, A, 3, 22, 4);
    ROUND1(X, A, B, C, D, 4, 7, 5);
    ROUND1(X, D, A, B, C, 5, 12, 6);
    ROUND1(X, C, D, A, B, 6, 17, 7);
    ROUND1(X, B, C, D, A, 7, 22, 8);
    ROUND1(X, A, B, C, D, 8, 7, 9);
    ROUND1(X, D, A, B, C, 9, 12, 10);
    ROUND1(X, C, D, A, B, 10, 17, 11);
    ROUND1(X, B, C, D, A, 11, 22, 12);
    ROUND1(X, A, B, C, D, 12, 7, 13);
    ROUND1(X, D, A, B, C, 13, 12, 14);
    ROUND1(X, C, D, A, B, 14, 17, 15);
    ROUND1(X, B, C, D, A, 15, 22, 16);

    ROUND2(X, A, B, C, D, 1, 5, 17);
    ROUND2(X, D, A, B, C, 6, 9, 18);
    ROUND2(X, C, D, A, B, 11, 14, 19);
    ROUND2(X, B, C, D, A, 0, 20, 20);
    ROUND2(X, A, B, C, D, 5, 5, 21);
    ROUND2(X, D, A, B, C, 10, 9, 22);
    ROUND2(X, C, D, A, B, 15, 14, 23);
    ROUND2(X, B, C, D, A, 4, 20, 24);
    ROUND2(X, A, B, C, D, 9, 5, 25);
    ROUND2(X, D, A, B, C, 14, 9, 26);
    ROUND2(X, C, D, A, B, 3, 14, 27);
    ROUND2(X, B, C, D, A, 8, 20, 28);
    ROUND2(X, A, B, C, D, 13, 5, 29);
    ROUND2(X, D, A, B, C, 2, 9, 30);
    ROUND2(X, C, D, A, B, 7, 14, 31);
    ROUND2(X, B, C, D, A, 12, 20, 32);

    ROUND3(X, A, B, C, D, 5, 4, 33);
    ROUND3(X, D, A, B, C, 8, 11, 34);
    ROUND3(X, C, D, A, B, 11, 16, 35);
    ROUND3(X, B, C, D, A, 14, 23, 36);
    ROUND3(X, A, B, C, D, 1, 4, 37);
    ROUND3(X, D, A, B, C, 4, 11, 38);
    ROUND3(X, C, D, A, B, 7, 16, 39);
    ROUND3(X, B, C, D, A, 10, 23, 40);
    ROUND3(X, A, B, C, D, 13, 4, 41);
    ROUND3(X, D, A, B, C, 0, 11, 42);
    ROUND3(X, C, D, A, B, 3, 16, 43);
    ROUND3(X, B, C, D, A, 6, 23, 44);
    ROUND3(X, A, B, C, D, 9, 4, 45);
    ROUND3(X, D, A, B, C, 12, 11, 46);
    ROUND3(X, C, D, A, B, 15, 16, 47);
    ROUND3(X, B, C, D, A, 2, 23, 48);

    ROUND4(X, A, B, C, D, 0, 6, 49);
    ROUND4(X, D, A, B, C, 7, 10, 50);
    ROUND4(X, C, D, A, B, 14, 15, 51);
    ROUND4(X, B, C, D, A, 5, 21, 52);
    ROUND4(X, A, B, C, D, 12, 6, 53);
    ROUND4(X, D, A, B, C, 3, 10, 54);
    ROUND4(X, C, D, A, B, 10, 15, 55);
    ROUND4(X, B, C, D, A, 1, 21, 56);
    ROUND4(X, A, B, C, D, 8, 6, 57);
    ROUND4(X, D, A, B, C, 15, 10, 58);
    ROUND4(X, C, D, A, B, 6, 15, 59);
    ROUND4(X, B, C, D, A, 13, 21, 60);
    ROUND4(X, A, B, C, D, 4, 6, 61);
    ROUND4(X, D, A, B, C, 11, 10, 62);
    ROUND4(X, C, D, A, B, 2, 15, 63);
    ROUND4(X, B, C, D, A, 9, 21, 64);

    md5_s.t32[0] += A;
    md5_s.t32[1] += B;
    md5_s.t32[2] += C;
    md5_s.t32[3] += D;
}

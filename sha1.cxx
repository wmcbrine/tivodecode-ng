/*
 * SHA-1 in C[++]
 * Originally By Steve Reid <steve@edmweb.com>
 * Adapted for tivodecode-ng
 * 100% Public Domain
 */

/* Test Vectors (from FIPS PUB 180-1)
   "abc"
   A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
   "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
   84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
   A million repetitions of "a"
   34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
*/

#include <algorithm>

#include "bits.hxx"
#include "sha1.hxx"

/* blk() performs the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */

uint32_t blk(uint32_t *block, int i)
{
    block[i & 15] = ROTL(block[(i + 13) & 15] ^ block[(i + 8) & 15] ^
                         block[(i + 2) & 15] ^ block[i & 15], 1);

    return block[i & 15];
}

/* (R0 + R1), R2, R3, R4 are the different operations used in SHA1 */

void R0(uint32_t *block, uint32_t v, uint32_t &w, uint32_t x,
        uint32_t y, uint32_t &z, int i)
{
    z += ((w & (x ^ y)) ^ y) + block[i] + 0x5A827999 + ROTL(v, 5);
    w = ROTL(w, 30);
}

void R1(uint32_t *block, uint32_t v, uint32_t &w, uint32_t x,
        uint32_t y, uint32_t &z, int i)
{
    z += ((w & (x ^ y)) ^ y) + blk(block, i) + 0x5A827999 + ROTL(v, 5);
    w = ROTL(w, 30);
}

void R2(uint32_t *block, uint32_t v, uint32_t &w, uint32_t x,
        uint32_t y, uint32_t &z, int i)
{
    z += (w ^ x ^ y) + blk(block, i) + 0x6ED9EBA1 + ROTL(v, 5);
    w = ROTL(w, 30);
}

void R3(uint32_t *block, uint32_t v, uint32_t &w, uint32_t x,
        uint32_t y, uint32_t &z, int i)
{
    z += (((w | x) & y) | (w & x)) + blk(block, i) + 0x8F1BBCDC + ROTL(v, 5);
    w = ROTL(w, 30);
}

void R4(uint32_t *block, uint32_t v, uint32_t &w, uint32_t x,
        uint32_t y, uint32_t &z, int i)
{
    z += (w ^ x ^ y) + blk(block, i) + 0xCA62C1D6 + ROTL(v, 5);
    w = ROTL(w, 30);
}

/* Hash a single 512-bit block. This is the core of the algorithm. */

static void transform(uint32_t state[5], const uint8_t buffer[64])
{
    uint32_t a, b, c, d, e;
    uint32_t block[16];

    for (int i = 0; i < 16; i++)
        block[i] = GET32(&buffer[i << 2]);

    /* Copy state[] to working vars */
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(block, a, b, c, d, e,  0); R0(block, e, a, b, c, d,  1);
    R0(block, d, e, a, b, c,  2); R0(block, c, d, e, a, b,  3);
    R0(block, b, c, d, e, a,  4); R0(block, a, b, c, d, e,  5);
    R0(block, e, a, b, c, d,  6); R0(block, d, e, a, b, c,  7);
    R0(block, c, d, e, a, b,  8); R0(block, b, c, d, e, a,  9);
    R0(block, a, b, c, d, e, 10); R0(block, e, a, b, c, d, 11);
    R0(block, d, e, a, b, c, 12); R0(block, c, d, e, a, b, 13);
    R0(block, b, c, d, e, a, 14); R0(block, a, b, c, d, e, 15);

    R1(block, e, a, b, c, d, 16); R1(block, d, e, a, b, c, 17);
    R1(block, c, d, e, a, b, 18); R1(block, b, c, d, e, a, 19);

    R2(block, a, b, c, d, e, 20); R2(block, e, a, b, c, d, 21);
    R2(block, d, e, a, b, c, 22); R2(block, c, d, e, a, b, 23);
    R2(block, b, c, d, e, a, 24); R2(block, a, b, c, d, e, 25);
    R2(block, e, a, b, c, d, 26); R2(block, d, e, a, b, c, 27);
    R2(block, c, d, e, a, b, 28); R2(block, b, c, d, e, a, 29);
    R2(block, a, b, c, d, e, 30); R2(block, e, a, b, c, d, 31);
    R2(block, d, e, a, b, c, 32); R2(block, c, d, e, a, b, 33);
    R2(block, b, c, d, e, a, 34); R2(block, a, b, c, d, e, 35);
    R2(block, e, a, b, c, d, 36); R2(block, d, e, a, b, c, 37);
    R2(block, c, d, e, a, b, 38); R2(block, b, c, d, e, a, 39);

    R3(block, a, b, c, d, e, 40); R3(block, e, a, b, c, d, 41);
    R3(block, d, e, a, b, c, 42); R3(block, c, d, e, a, b, 43);
    R3(block, b, c, d, e, a, 44); R3(block, a, b, c, d, e, 45);
    R3(block, e, a, b, c, d, 46); R3(block, d, e, a, b, c, 47);
    R3(block, c, d, e, a, b, 48); R3(block, b, c, d, e, a, 49);
    R3(block, a, b, c, d, e, 50); R3(block, e, a, b, c, d, 51);
    R3(block, d, e, a, b, c, 52); R3(block, c, d, e, a, b, 53);
    R3(block, b, c, d, e, a, 54); R3(block, a, b, c, d, e, 55);
    R3(block, e, a, b, c, d, 56); R3(block, d, e, a, b, c, 57);
    R3(block, c, d, e, a, b, 58); R3(block, b, c, d, e, a, 59);

    R4(block, a, b, c, d, e, 60); R4(block, e, a, b, c, d, 61);
    R4(block, d, e, a, b, c, 62); R4(block, c, d, e, a, b, 63);
    R4(block, b, c, d, e, a, 64); R4(block, a, b, c, d, e, 65);
    R4(block, e, a, b, c, d, 66); R4(block, d, e, a, b, c, 67);
    R4(block, c, d, e, a, b, 68); R4(block, b, c, d, e, a, 69);
    R4(block, a, b, c, d, e, 70); R4(block, e, a, b, c, d, 71);
    R4(block, d, e, a, b, c, 72); R4(block, c, d, e, a, b, 73);
    R4(block, b, c, d, e, a, 74); R4(block, a, b, c, d, e, 75);
    R4(block, e, a, b, c, d, 76); R4(block, d, e, a, b, c, 77);
    R4(block, c, d, e, a, b, 78); R4(block, b, c, d, e, a, 79);

    /* Add the working vars back into context.state[] */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

/* sha1_init - Initialize new context */

void SHA1::init()
{
    /* SHA1 initialization constants */
    state[0] = 0x67452301;
    state[1] = 0xEFCDAB89;
    state[2] = 0x98BADCFE;
    state[3] = 0x10325476;
    state[4] = 0xC3D2E1F0;
    count[0] = count[1] = 0;
}

/* Run your data through this. */

void SHA1::update(const uint8_t *data, size_t len)
{
    unsigned int i, j;

    j = (count[0] >> 3) & 63;

    if ((count[0] += (unsigned int)len << 3) < (len << 3))
        count[1]++;

    count[1] += (unsigned int)(len >> 29);

    if ((j + len) > 63)
    {
        i = 64 - j;
        std::copy(data, data + i, buffer + j);
        transform(state, buffer);

        for ( ; i + 63 < len; i += 64)
        {
            std::copy(data + i, data + i + 64, buffer);
            transform(state, buffer);
        }

        j = 0;
    }
    else
        i = 0;

    std::copy(data + i, data + len, buffer + j);
}

/* Add padding and return the message digest. */

void SHA1::final(uint8_t digest[20])
{
    unsigned int i;
    uint8_t finalcount[8];

    for (i = 0; i < 8; i++)
    {
        finalcount[i] = (uint8_t)((count[(i >= 4 ? 0 : 1)]
                        >> ((3 - (i & 3)) * 8)) & 255);  /* Either-endian */
    }

    update((const uint8_t *)"\200", 1);

    while ((count[0] & 504) != 448)
        update((const uint8_t *)"\0", 1);

    update(finalcount, 8);  /* Should cause a transform() */

    for (i = 0; i < 20; i++)
    {
        digest[i] = (uint8_t)
            ((state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
    }
}

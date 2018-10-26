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

/* Hash a single 512-bit block. This is the core of the algorithm. */

void SHA1::transform()
{
    uint32_t a, b, c, d, e, tmp;
    uint32_t block[16];
    int i;

    /* Copy state[] to working vars */
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    for (i = 0; i < 80; i++)
    {
        if (i < 16)           // R0
        {
            block[i] = GET32(&buffer[i << 2]);
            e += ((b & (c ^ d)) ^ d) + block[i] + 0x5A827999;
        }
        else
        {
            if (i < 20)       // R1
                e += ((b & (c ^ d)) ^ d) + 0x5A827999;
            else if (i < 40)  // R2
                e += (b ^ c ^ d) + 0x6ED9EBA1;
            else if (i < 60)  // R3
                e += (((b | c) & d) | (b & c)) + 0x8F1BBCDC;
            else              // R4
                e += (b ^ c ^ d) + 0xCA62C1D6;

            block[i & 15] = ROTL(block[(i + 13) & 15] ^ block[(i + 8) & 15] ^
                                 block[(i + 2) & 15] ^ block[i & 15], 1);
            e += block[i & 15];
        }

        tmp = e + ROTL(a, 5); e = d; d = c; c = ROTL(b, 30); b = a; a = tmp;
    }

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
        transform();

        for ( ; i + 63 < len; i += 64)
        {
            std::copy(data + i, data + i + 64, buffer);
            transform();
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

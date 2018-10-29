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

SHA1::SHA1()
{
    init();
}

/* Hash a single 512-bit block. This is the core of the algorithm. */

void SHA1::transform(const uint8_t *buffer)
{
    uint32_t block[16], a, b, c, d, e, tmp;

    /* Copy state[] to working vars */
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    for (int i = 0; i < 80; i++)
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

    /* Add the working vars back into state[] */
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
    count.lsw = count.msw = 0;
    index = 0;
}

/* Run your data through this. */

void SHA1::update(const uint8_t *data, size_t len)
{
    size_t gap, i;

    if (count.lsw + len * 8 < count.lsw)
        count.msw++;
    count.lsw += len * 8;
    gap = 64 - index;

    if (len >= gap)
    {
        std::copy(data, data + gap, buffer + index);
        transform(buffer);

        for (i = gap; i + 64 <= len; i += 64)
            transform(data + i);

        index = len - i;
        std::copy(data + i, data + i + 64, buffer);
    }
    else
    {
        std::copy(data, data + len, buffer + index);
        index += len;
    }
}

/* Add padding and return the message digest. */

void SHA1::final(uint8_t *digest)
{
    size_t gap;

    buffer[index] = 0x80;

    gap = 64 - index;
    if (gap > 8)
        std::fill(buffer + index + 1, buffer + 56, 0);
    else
    {
        std::fill(buffer + index + 1, buffer + 64, 0);
        transform(buffer);
        std::fill(buffer, buffer + 56, 0);
    }

    PUT32(count.msw, buffer + 56);
    PUT32(count.lsw, buffer + 60);

    transform(buffer);

    for (int i = 0; i < 5; i++)
        PUT32(state[i], &digest[i << 2]);
}

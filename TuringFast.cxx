/*
 * Fast (unrolled) implementation of Turing
 *
 * Copyright 2002, Qualcomm Inc. Originally by Greg Rose
 * Adapted for tivodecode-ng
 * See COPYING.md for license terms
 */

#include <cstdlib>

#include "bits.hxx"
#include "Turing.hxx"		/* interface definitions */
#include "TuringBoxes.hxx"

/*
 * This does a reversible transformation of a word, based on the S-boxes.
 * The reversibility isn't used, but it guarantees no loss of information,
 * and hence no equivalent keys or IVs.
 */
static uint32_t fixedS(uint32_t w)
{
    uint32_t b;

    b = Sbox[w >> 24];
    w = ((w ^      Qbox[b])      & 0x00FFFFFF) | (b << 24);
    b = Sbox[(w >> 16) & 0xFF];
    w = ((w ^ ROTL(Qbox[b], 8))  & 0xFF00FFFF) | (b << 16);
    b = Sbox[(w >> 8) & 0xFF];
    w = ((w ^ ROTL(Qbox[b], 16)) & 0xFFFF00FF) | (b << 8);
    b = Sbox[w & 0xFF];
    w = ((w ^ ROTL(Qbox[b], 24)) & 0xFFFFFF00) | b;

    return w;
}

/* two variants of the Pseudo-Hadamard Transform */

/* Mix 5 words in place */
void PHT(uint32_t &A, uint32_t &B, uint32_t &C, uint32_t &D, uint32_t &E)
{
    E += A + B + C + D;
    A += E;
    B += E;
    C += E;
    D += E;
}

/* General word-wide n-PHT */
void mixwords(uint32_t *w, int n)
{
    uint32_t sum;
    int i;

    n--;
    for (sum = i = 0; i < n; ++i)
	sum += w[i];
    w[n] += sum;
    sum = w[n];
    for (i = 0; i < n; ++i)
	w[i] += sum;
}

/*
 * Key the cipher.
 * Table version; gathers words, mixes them, saves them.
 * Then compiles lookup tables for the keyed S-boxes.
 */
void Turing::key(const uint8_t *key, int len)
{
    int i, j, k, l;
    uint32_t w;

    if ((len & 0x03) != 0 || len > MAXKEY)
	std::abort();
    keylen = 0;
    for (i = 0; i < len; i += 4)
	K[keylen++] = fixedS(GET32(&key[i]));
    mixwords(K, keylen);

    /* build S-box lookup tables */
    for (l = 0; l < 4; ++l) {
        int sh1 = l << 3;
        int sh2 = 24 - sh1;
        uint32_t mask = (0xff << sh2) ^ 0xffffffffUL;
        for (j = 0; j < 256; ++j) {
            w = 0;
            k = j;
            for (i = 0; i < keylen; ++i) {
                k = Sbox[BYTE(K[i], l) ^ k];
                w ^= ROTL(Qbox[k], i + sh1);
            }
            Sb[l][j] = (w & mask) | (k << sh2);
        }
    }
}

/*
 * Load the Initialization Vector.
 * Actually, this fills the LFSR, with IV, key, length, and more.
 * IV goes through the fixed S-box, key is premixed, the rest go through
 * the keyed S-boxes. The reason for this is to avoid nasty interactions
 * between the mixed key and the S-boxes that depend on them, and also
 * to avoid any "chosen-IV" interactions with the keyed S-boxes, not that I
 * can think of any.
 */
void Turing::IV(const uint8_t *iv, int len)
{
    int i, j;

    /* check args */
    if ((len & 0x03) != 0 || (len + 4 * keylen) > MAXKIV)
	std::abort();
    /* first copy in the IV, mixing as we go */
    for (i = j = 0; j < len; j += 4)
	R[i++] = fixedS(GET32(&iv[j]));
    /* now continue with the premixed key */
    for (j = 0 /* i continues */; j < keylen; ++j)
	R[i++] = K[j];
    /* now the length-dependent word */
    R[i++] = (keylen << 4) | (len >> 2) | 0x01020300UL;
    /* ... and fill the rest of the register */
    for (j = 0 /* i continues */; i < LFSRLEN; ++i, ++j)
	R[i] = S(R[j] + R[i - 1], 0);
    /* finally mix all the words */
    mixwords(R, LFSRLEN);
}

/* step the LFSR */
/* After stepping, "zero" moves right one place
 */
void Turing::STEP(int z)
{
    R[z % 17] = R[(z + 15) % 17] ^ R[(z + 4) % 17] ^
	(R[z % 17] << 8) ^ Multab[(R[z % 17] >> 24)];
}

/*
 * Push a word through the keyed S-boxes.
 */
uint32_t Turing::S(uint32_t w, int b)
{
    return Sb[0][BYTE(w, (0 + b) & 3)]
         ^ Sb[1][BYTE(w, (1 + b) & 3)]
         ^ Sb[2][BYTE(w, (2 + b) & 3)]
         ^ Sb[3][BYTE(w, (3 + b) & 3)];
}

/* a single round */
void Turing::ROUND(int z, uint8_t *buf)
{
    uint32_t A, B, C, D, E;

    STEP(z);
    A = R[z % 17];
        B = R[(z + 14) % 17];
            C = R[(z + 7) % 17];
                D = R[(z + 2) % 17];
                    E = R[(z + 1) % 17];
    PHT(A, B, C, D, E);
    A = S(A, 0); B = S(B, 1); C = S(C, 2); D = S(D, 3); E = S(E, 0);
    PHT(A, B, C, D, E);
    STEP(z + 1);
    STEP(z + 2);
    STEP(z + 3);
    A += R[(z + 1) % 17];
        B += R[(z + 16) % 17];
            C += R[(z + 12) % 17];
                D += R[(z + 5) % 17];
                    E += R[(z + 4) % 17];
    PUT32(A, buf);
        PUT32(B, buf + 4);
            PUT32(C, buf + 8);
                PUT32(D, buf + 12);
                    PUT32(E, buf + 16);
    STEP(z + 4);
}

/*
 * Generate 17 5-word blocks of output.
 * This ensures that the register is resynchronised and avoids state.
 * Buffering issues are outside the scope of this implementation.
 * Returns the number of bytes of stream generated.
 */
int Turing::gen(uint8_t *buf)
{
    for (int i = 0; i < 81; i += 5)
        ROUND(i, buf + (i << 2));

    return 17 * 20;
}

/*
 * Fast (unrolled) implementation of Turing
 *
 * Copyright C 2002, Qualcomm Inc. Written by Greg Rose
 */

/*
This software is free for commercial and non-commercial use subject to
the following conditions:

1.  Copyright remains vested in QUALCOMM Incorporated, and Copyright
notices in the code are not to be removed.  If this package is used in
a product, QUALCOMM should be given attribution as the author of the
Turing encryption algorithm. This can be in the form of a textual
message at program startup or in documentation (online or textual)
provided with the package.

2.  Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

a. Redistributions of source code must retain the copyright notice,
   this list of conditions and the following disclaimer.

b. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

c. All advertising materials mentioning features or use of this
   software must display the following acknowledgement:  This product
   includes software developed by QUALCOMM Incorporated.

3.  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND AGAINST
INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

4.  The license and distribution terms for any publically available version
or derivative of this code cannot be changed, that is, this code cannot
simply be copied and put under another distribution license including
the GNU Public License.

5.  The Turing family of encryption algorithms are covered by patents in
the United States of America and other countries. A free and
irrevocable license is hereby granted for the use of such patents to
the extent required to utilize the Turing family of encryption
algorithms for any purpose, subject to the condition that any
commercial product utilising any of the Turing family of encryption
algorithms should show the words "Encryption by QUALCOMM" either on the
product or in the associated documentation.
*/

#include <cstdlib>

#include "bits.hxx"
#include "Turing.hxx"		/* interface definitions */
#include "TuringBoxes.hxx"

inline uint8_t BYTE(uint32_t x, int i)
{
    return (x >> (24 - 8 * i)) & 0xFF;
}

inline void WORD2BYTE(uint32_t w, uint8_t *b)
{
    b[3] = BYTE(w, 3);
    b[2] = BYTE(w, 2);
    b[1] = BYTE(w, 1);
    b[0] = BYTE(w, 0);
}

/*
 * This does a reversible transformation of a word, based on the S-boxes.
 * The reversibility isn't used, but it guarantees no loss of information,
 * and hence no equivalent keys or IVs.
 */
static uint32_t fixedS(uint32_t w)
{
    uint32_t b;

    b = Sbox[BYTE(w, 0)];
    w = ((w ^      Qbox[b])      & 0x00FFFFFF) | (b << 24);
    b = Sbox[BYTE(w, 1)];
    w = ((w ^ ROTL(Qbox[b], 8))  & 0xFF00FFFF) | (b << 16);
    b = Sbox[BYTE(w, 2)];
    w = ((w ^ ROTL(Qbox[b], 16)) & 0xFFFF00FF) | (b << 8);
    b = Sbox[BYTE(w, 3)];
    w = ((w ^ ROTL(Qbox[b], 24)) & 0xFFFFFF00) | b;

    return w;
}

/* two variants of the Pseudo-Hadamard Transform */

/* Mix 5 words in place */
inline void PHT(uint32_t &A, uint32_t &B, uint32_t &C,
                uint32_t &D, uint32_t &E)
{
    E += A + B + C + D;
    A += E;
    B += E;
    C += E;
    D += E;
}

/* General word-wide n-PHT */
void mixwords(uint32_t w[], int n)
{
    uint32_t sum;
    int i;

    for (sum = i = 0; i < n - 1; ++i)
	sum += w[i];
    w[n - 1] += sum;
    sum = w[n - 1];
    for (i = 0; i < n - 1; ++i)
	w[i] += sum;
}

/*
 * Key the cipher.
 * Table version; gathers words, mixes them, saves them.
 * Then compiles lookup tables for the keyed S-boxes.
 */
void Turing::key(const uint8_t key[], const int keylength)
{
    int i, j, k, l;
    uint32_t w;

    if ((keylength & 0x03) != 0 || keylength > MAXKEY)
	std::abort();
    keylen = 0;
    for (i = 0; i < keylength; i += 4)
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
void Turing::IV(const uint8_t iv[], const int ivlength)
{
    int i, j;

    /* check args */
    if ((ivlength & 0x03) != 0 || (ivlength + 4 * keylen) > MAXKIV)
	std::abort();
    /* first copy in the IV, mixing as we go */
    for (i = j = 0; j < ivlength; j += 4)
	R[i++] = fixedS(GET32(&iv[j]));
    /* now continue with the premixed key */
    for (j = 0 /* i continues */; j < keylen; ++j)
	R[i++] = K[j];
    /* now the length-dependent word */
    R[i++] = (keylen << 4) | (ivlength >> 2) | 0x01020300UL;
    /* ... and fill the rest of the register */
    for (j = 0 /* i continues */; i < LFSRLEN; ++i, ++j)
	R[i] = S(R[j] + R[i - 1], 0);
    /* finally mix all the words */
    mixwords(R, LFSRLEN);
}

/* give correct offset for the current position of the register,
 * where logically R[0] is at position "zero".
 */
inline int OFF(int zero, int i)
{
    return (zero + i) % LFSRLEN;
}

/* step the LFSR */
/* After stepping, "zero" moves right one place
 */
inline void Turing::STEP(int z)
{
    R[OFF(z, 0)] = R[OFF(z, 15)] ^ R[OFF(z, 4)] ^
	(R[OFF(z, 0)] << 8) ^ Multab[(R[OFF(z, 0)] >> 24) & 0xFF];
}

/*
 * Push a word through the keyed S-boxes.
 */
inline uint32_t Turing::S(uint32_t w, int b)
{
    return Sb[0][BYTE(w, (0 + b) & 0x3)]
         ^ Sb[1][BYTE(w, (1 + b) & 0x3)]
         ^ Sb[2][BYTE(w, (2 + b) & 0x3)]
         ^ Sb[3][BYTE(w, (3 + b) & 0x3)];
}

/* a single round */
inline void Turing::ROUND(int z, uint8_t *b)
{
    uint32_t A, B, C, D, E;

    STEP(z);
    A = R[OFF(z + 1, 16)];
        B = R[OFF(z + 1, 13)];
            C = R[OFF(z + 1, 6)];
                D = R[OFF(z + 1, 1)];
                    E = R[OFF(z + 1, 0)];
    PHT(A, B, C, D, E);
    A = S(A, 0); B = S(B, 1); C = S(C, 2); D = S(D, 3); E = S(E, 0);
    PHT(A, B, C, D, E);
    STEP(z + 1);
    STEP(z + 2);
    STEP(z + 3);
    A += R[OFF(z + 4, 14)];
        B += R[OFF(z + 4, 12)];
            C += R[OFF(z + 4, 8)];
                D += R[OFF(z + 4, 1)];
                    E += R[OFF(z + 4, 0)];
    WORD2BYTE(A, b);
        WORD2BYTE(B, b + 4);
            WORD2BYTE(C, b + 8);
                WORD2BYTE(D, b + 12);
                    WORD2BYTE(E, b + 16);
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
    ROUND(0, buf);
    ROUND(5, buf + 20);
    ROUND(10, buf + 40);
    ROUND(15, buf + 60);
    ROUND(3, buf + 80);
    ROUND(8, buf + 100);
    ROUND(13, buf + 120);
    ROUND(1, buf + 140);
    ROUND(6, buf + 160);
    ROUND(11, buf + 180);
    ROUND(16, buf + 200);
    ROUND(4, buf + 220);
    ROUND(9, buf + 240);
    ROUND(14, buf + 260);
    ROUND(2, buf + 280);
    ROUND(7, buf + 300);
    ROUND(12, buf + 320);

    return 17 * 20;
}

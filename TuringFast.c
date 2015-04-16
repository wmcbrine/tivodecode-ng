/* @(#)TuringFast.c	1.6 (QUALCOMM Turing) 03/02/24 */
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
#ifdef HAVE_CONFIG_H
#include "tdconfig.h"
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "Turing.h"		/* interface definitions */
#include "TuringSbox.h"
#include "QUTsbox.h"
#include "TuringMultab.h"

struct turing_internal
{
	int	keylen;		/* adjusted to count WORDs */
	WORD	K[MAXKEY/4];	/* storage for mixed key */
	WORD	R[LFSRLEN];	/* the shift register */
	/* precalculated S-boxes */
	WORD	S0[256], S1[256], S2[256], S3[256];
};

/* give correct offset for the current position of the register,
 * where logically turing->R[0] is at position "zero".
 */
#define OFF(zero, i) (((zero)+(i)) % LFSRLEN)

/* step the LFSR */
/* After stepping, "zero" moves right one place */
#define STEP(z) \
    turing->R[OFF(z,0)] = turing->R[OFF(z,15)] ^ turing->R[OFF(z,4)] ^ \
	(turing->R[OFF(z,0)] << 8) ^ Multab[(turing->R[OFF(z,0)] >> 24) & 0xFF]

/*
 * This does a reversible transformation of a word, based on the S-boxes.
 * The reversibility isn't used, but it guarantees no loss of information,
 * and hence no equivalent keys or IVs.
 */
static WORD
fixedS(WORD w)
{
    WORD    b;

    b = Sbox[B(w, 0)]; w = ((w ^      Qbox[b])     & 0x00FFFFFF) | (b << 24);
    b = Sbox[B(w, 1)]; w = ((w ^ ROTL(Qbox[b],8))  & 0xFF00FFFF) | (b << 16);
    b = Sbox[B(w, 2)]; w = ((w ^ ROTL(Qbox[b],16)) & 0xFFFF00FF) | (b << 8);
    b = Sbox[B(w, 3)]; w = ((w ^ ROTL(Qbox[b],24)) & 0xFFFFFF00) | b;
    return w;
}

/*
 * Push a word through the keyed S-boxes.
 */
#define S(w,b) (turing->S0[B((w), ((0+b)&0x3))] \
		^ turing->S1[B((w), ((1+b)&0x3))] \
		^ turing->S2[B((w), ((2+b)&0x3))] \
		^ turing->S3[B((w), ((3+b)&0x3))])

/* two variants of the Pseudo-Hadamard Transform */

/* Mix 5 words in place */
#define PHT(A,B,C,D,E) { \
	(E) += (A) + (B) + (C) + (D); \
	(A) += (E); \
	(B) += (E); \
	(C) += (E); \
	(D) += (E); \
}

/* General word-wide n-PHT */
void
mixwords(WORD w[], int n)
{
    WORD sum;
    int i;

    for (sum = i = 0; i < n-1; ++i)
	sum += w[i];
    w[n-1] += sum;
    sum = w[n-1];
    for (i = 0; i < n-1; ++i)
	w[i] += sum;
}

/*
 * Key the cipher.
 * Table version; gathers words, mixes them, saves them.
 * Then compiles lookup tables for the keyed S-boxes.
 */
void
TuringKey(void * internal, const BYTE key[], const int keylength)
{
    struct turing_internal * turing = (struct turing_internal*)internal;
    int i, j, k;
    WORD w;

    if ((keylength & 0x03) != 0 || keylength > MAXKEY)
	abort();
    turing->keylen = 0;
    for (i = 0; i < keylength; i += 4)
	turing->K[turing->keylen++] = fixedS(BYTE2WORD(&key[i]));
    mixwords(turing->K, turing->keylen);

    /* build S-box lookup tables */
    for (j = 0; j < 256; ++j) {
	w = 0;
	k = j;
	for (i = 0; i < turing->keylen; ++i) {
	    k = Sbox[B(turing->K[i], 0) ^ k];
	    w ^= ROTL(Qbox[k], i + 0);
	}
	turing->S0[j] = (w & 0x00FFFFFFUL) | (k << 24);
    }
    for (j = 0; j < 256; ++j) {
	w = 0;
	k = j;
	for (i = 0; i < turing->keylen; ++i) {
	    k = Sbox[B(turing->K[i], 1) ^ k];
	    w ^= ROTL(Qbox[k], i + 8);
	}
	turing->S1[j] = (w & 0xFF00FFFFUL) | (k << 16);
    }
    for (j = 0; j < 256; ++j) {
	w = 0;
	k = j;
	for (i = 0; i < turing->keylen; ++i) {
	    k = Sbox[B(turing->K[i], 2) ^ k];
	    w ^= ROTL(Qbox[k], i + 16);
	}
	turing->S2[j] = (w & 0xFFFF00FFUL) | (k << 8);
    }
    for (j = 0; j < 256; ++j) {
	w = 0;
	k = j;
	for (i = 0; i < turing->keylen; ++i) {
	    k = Sbox[B(turing->K[i], 3) ^ k];
	    w ^= ROTL(Qbox[k], i + 24);
	}
	turing->S3[j] = (w & 0xFFFFFF00UL) | k;
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
void
TuringIV(void * internal, const BYTE iv[], const int ivlength)
{
    struct turing_internal * turing = (struct turing_internal*)internal;
    int i, j;

    /* check args */
    if ((ivlength & 0x03) != 0 || (ivlength + 4*turing->keylen) > MAXKIV)
	abort();
    /* first copy in the IV, mixing as we go */
    for (i = j = 0; j < ivlength; j +=4)
	turing->R[i++] = fixedS(BYTE2WORD(&iv[j]));
    /* now continue with the premixed key */
    for (j = 0 /* i continues */; j < turing->keylen; ++j)
	turing->R[i++] = turing->K[j];
    /* now the length-dependent word */
    turing->R[i++] = (turing->keylen << 4) | (ivlength >> 2) | 0x01020300UL;
    /* ... and fill the rest of the register */
    for (j = 0 /* i continues */; i < LFSRLEN; ++i, ++j)
	turing->R[i] = S(turing->R[j] + turing->R[i-1], 0);
    /* finally mix all the words */
    mixwords(turing->R, LFSRLEN);
}

/* a single round */
#define ROUND(z,b) \
{ \
    STEP(z); \
    A = turing->R[OFF(z+1,16)]; \
		B = turing->R[OFF(z+1,13)]; \
			    C = turing->R[OFF(z+1,6)]; \
					D = turing->R[OFF(z+1,1)]; \
						    E = turing->R[OFF(z+1,0)]; \
    PHT(A,	B,	    C,		D,	    E); \
    A = S(A,0);	B = S(B,1); C = S(C,2);	D = S(D,3); E = S(E,0); \
    PHT(A,	B,	    C,		D,	    E); \
    STEP(z+1); \
    STEP(z+2); \
    STEP(z+3); \
    A += turing->R[OFF(z+4,14)]; \
		B += turing->R[OFF(z+4,12)]; \
			    C += turing->R[OFF(z+4,8)]; \
					D += turing->R[OFF(z+4,1)]; \
						    E += turing->R[OFF(z+4,0)]; \
    WORD2BYTE(A, b); \
		WORD2BYTE(B, b+4); \
			    WORD2BYTE(C, b+8); \
					WORD2BYTE(D, b+12); \
						    WORD2BYTE(E, b+16); \
    STEP(z+4); \
}

/*
 * Generate 17 5-word blocks of output.
 * This ensures that the register is resynchronised and avoids state.
 * Buffering issues are outside the scope of this implementation.
 * Returns the number of bytes of stream generated.
 */
int
TuringGen(void * internal, BYTE *buf)
{
    struct turing_internal * turing = (struct turing_internal*)internal;
    WORD	    A, B, C, D, E;

    ROUND(0,buf);
    ROUND(5,buf+20);
    ROUND(10,buf+40);
    ROUND(15,buf+60);
    ROUND(3,buf+80);
    ROUND(8,buf+100);
    ROUND(13,buf+120);
    ROUND(1,buf+140);
    ROUND(6,buf+160);
    ROUND(11,buf+180);
    ROUND(16,buf+200);
    ROUND(4,buf+220);
    ROUND(9,buf+240);
    ROUND(14,buf+260);
    ROUND(2,buf+280);
    ROUND(7,buf+300);
    ROUND(12,buf+320);
    return 17*20;
}

void * TuringAlloc()
{
//	return calloc(1, sizeof(struct turing_internal));
    void * pVoid = malloc(sizeof(struct turing_internal));
    memset(pVoid, 0, sizeof(struct turing_internal) );
	return malloc(sizeof(struct turing_internal));
}

void TuringFree(void * internal)
{
    if(internal)
    	free(internal);
}

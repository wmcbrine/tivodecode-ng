/* @(#)Turing.h	1.4 (QUALCOMM) 03/02/24 */
/*
 * Interface definition of Turing
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

#ifndef TURING_H
#define TURING_H 1

typedef unsigned char	BYTE;
typedef unsigned long	WORD;

#define MAXKEY	    32 /*bytes*/
#define MAXKIV	    48 /*bytes*/
#define MAXSTREAM   340 /* bytes, maximum stream generated by one call */
#define LFSRLEN	    17 /*words*/
#define OUTLEN	    20 /*bytes*/
#define POLY	    0x4D	/* x^8 + x^6 + x^3 + x^2 + 1 */

void * TuringAlloc();
void TuringFree(void * internal);
void TuringKey(void * internal, const BYTE key[], const int keylength);
void TuringIV(void * internal, const BYTE iv[], const int ivlength);
int  TuringGen(void * internal, BYTE *buf);  /* returns number of bytes of mask generated */

/* some useful macros -- big-endian */
#define B(x,i) ((BYTE)(((x) >> (24 - 8*i)) & 0xFF))

#define WORD2BYTE(w, b) { \
	(b)[3] = B(w,3); \
	(b)[2] = B(w,2); \
	(b)[1] = B(w,1); \
	(b)[0] = B(w,0); \
}

#define BYTE2WORD(b) ( \
	(((WORD)(b)[0] & 0xFF)<<24) | \
	(((WORD)(b)[1] & 0xFF)<<16) | \
	(((WORD)(b)[2] & 0xFF)<<8) | \
	(((WORD)(b)[3] & 0xFF)) \
)

#define ROTL(w,x) (((w) << (x))|((w) >> (32 - (x))))

#endif

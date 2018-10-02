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

const int MAXKEY = 32;  /* bytes*/
const int MAXKIV = 48;  /* bytes*/
const int LFSRLEN = 17; /* words*/

#include <cstdint>

class Turing
{
    private:
        int      keylen;         /* adjusted to count WORDs */
        uint32_t K[MAXKEY / 4];  /* storage for mixed key */
        uint32_t R[LFSRLEN];     /* the shift register */
        /* precalculated S-boxes */
        uint32_t S0[256], S1[256], S2[256], S3[256];

        void STEP(int z);
        uint32_t S(uint32_t w, int b);
        void ROUND(int z, uint8_t *b);

    public:
        void key(const uint8_t key[], const int keylength);
        void IV(const uint8_t iv[], const int ivlength);
        int  gen(uint8_t *buf);  /* returns number of bytes of mask generated */
};

#endif

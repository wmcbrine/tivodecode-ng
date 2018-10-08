/*
 * Interface definition of Turing
 *
 * Copyright 2002, Qualcomm Inc. Originally by Greg Rose
 * See COPYING file for license terms
 */

#ifndef TURING_HXX_
#define TURING_HXX_

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
        uint32_t Sb[4][256];

        void STEP(int z);
        uint32_t S(uint32_t w, int b);
        void ROUND(int z, uint8_t *b);

    public:
        void key(const uint8_t key[], const int keylength);
        void IV(const uint8_t iv[], const int ivlength);
        int  gen(uint8_t *buf);  /* returns number of bytes of mask generated */
};

#endif /* TURING_HXX_ */

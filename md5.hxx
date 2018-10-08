/*
 * Copyright 1995, 1996, 1997, and 1998 WIDE Project.
 * See COPYING file for license terms
 *
 */

#ifndef MD5_HXX_
#define MD5_HXX_

#include <cstdint>
#include <string>

const size_t MD5_BUFLEN = 64;

class MD5
{
    private:
        union
        {
            uint32_t t32[4];
            uint8_t  t8[16];
        } md5_s;

        union
        {
            struct {
                uint32_t lsw;
                uint32_t msw;
            } t64;
            uint8_t t8[8];
        } md5_c;

        size_t md5_i;
        uint8_t md5_buf[MD5_BUFLEN];

        void calc(uint8_t *);

    public:
        void init();
        void loop(const std::string &);
        void pad();
        void result(uint8_t *);
};

#endif /* MD5_HXX_ */

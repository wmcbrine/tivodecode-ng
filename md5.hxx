/*
 * Copyright 1995, 1996, 1997, and 1998 WIDE Project.
 * Adapted for tivodecode-ng
 * See COPYING.md for license terms
 */

#ifndef MD5_HXX_
#define MD5_HXX_

#include <cstdint>
#include <string>

const size_t MD5_BUFLEN = 64;

class MD5
{
    private:
        uint32_t md5_st[4];

        struct {
            uint32_t lsw;
            uint32_t msw;
        } md5_count;

        size_t md5_i;
        uint8_t md5_buf[MD5_BUFLEN];

        void calc(const uint8_t *);

    public:
        void init();
        void loop(const std::string &);
        void pad();
        void result(uint8_t *);
};

#endif /* MD5_HXX_ */

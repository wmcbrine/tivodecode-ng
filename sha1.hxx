/*
 * tivodecode-ng
 * Copyright 2006-2015, Jeremy Drake et al.
 * See COPYING file for license terms
 */
#ifndef SHA1_H_
#define SHA1_H_

class SHA1
{
    private:
        uint32_t state[5];
        unsigned int count[2];
        uint8_t buffer[64];

    public:
        void init();
        void update(uint8_t *data, size_t len);
        void final(uint8_t digest[20]);
};

#endif

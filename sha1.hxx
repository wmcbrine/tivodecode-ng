/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */
#ifndef SHA1_H_
#define SHA1_H_

class SHA1
{
    private:
        unsigned int state[5];
        unsigned int count[2];
        unsigned char buffer[64];

    public:
        void init();
        void update(unsigned char *data, size_t len);
        void final(unsigned char digest[20]);
};

#endif

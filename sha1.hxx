/*
 * SHA-1 in C[++]
 * Originally By Steve Reid <steve@edmweb.com>
 * 100% Public Domain
 */

#ifndef SHA1_HXX_
#define SHA1_HXX_

class SHA1
{
    private:
        uint32_t state[5];
        unsigned int count[2];
        uint8_t buffer[64];

    public:
        void init();
        void update(const uint8_t *data, size_t len);
        void final(uint8_t digest[20]);
};

#endif /* SHA1_HXX_ */

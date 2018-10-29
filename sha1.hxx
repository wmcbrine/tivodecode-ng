/*
 * SHA-1 in C[++]
 * Originally By Steve Reid <steve@edmweb.com>
 * Adapted for tivodecode-ng
 * 100% Public Domain
 */

#ifndef SHA1_HXX_
#define SHA1_HXX_

#include <cstdint>

class SHA1
{
    private:
        uint32_t state[5];

        struct {
            uint32_t lsw;
            uint32_t msw;
        } count;

        size_t index;
        uint8_t buffer[64];

        void transform(const uint8_t *);

    public:
        SHA1();
        void init();
        void update(const uint8_t *, size_t);
        void final(uint8_t *);
};

#endif /* SHA1_HXX_ */

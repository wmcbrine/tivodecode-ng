#include <cstdint>

inline uint32_t ROTL(uint32_t value, int bits)
{
    return (value << bits) | (value >> (32 - bits));
}

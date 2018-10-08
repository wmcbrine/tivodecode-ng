// Some common bitwise operations

#include <cstdint>

inline uint32_t ROTL(uint32_t value, int bits)
{
    return (value << bits) | (value >> (32 - bits));
}

inline uint32_t GET32(const uint8_t *pVal)
{
    return (pVal[0] << 24) | (pVal[1] << 16) | (pVal[2] << 8) | pVal[3];
}

inline uint16_t GET16(const uint8_t *pVal)
{
    return (pVal[0] << 8) | pVal[1];
}

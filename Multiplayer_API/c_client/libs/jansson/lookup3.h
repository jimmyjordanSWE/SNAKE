#include <stdint.h>
#include <string.h>

#define hashsize(n) ((size_t)1 << (n))
#define hashmask(n) (hashsize(n) - 1)

static inline uint32_t rot(uint32_t x, uint32_t k) {
    return (x << k) | (x >> (32 - k));
}

#define MIX(a,b,c) do { \
    a -= c;  a ^= rot(c, 4);  c += b; \
    b -= a;  b ^= rot(a, 6);  a += c; \
    c -= b;  c ^= rot(b, 8);  b += a; \
    a -= c;  a ^= rot(c,16);  c += b; \
    b -= a;  b ^= rot(a,19);  a += c; \
    c -= b;  c ^= rot(b, 4);  b += a; \
} while (0)

#define FINAL(a,b,c) do { \
    c ^= b; c -= rot(b,14); \
    a ^= c; a -= rot(c,11); \
    b ^= a; b -= rot(a,25); \
    c ^= b; c -= rot(b,16); \
    a ^= c; a -= rot(c, 4); \
    b ^= a; b -= rot(a,14); \
    c ^= b; c -= rot(b,24); \
} while (0)

static uint32_t hashlittle(const void *key, size_t length, uint32_t initval)
{
    const uint8_t *p = (const uint8_t *)key;
    uint32_t a, b, c;

    a = b = 0x9e3779b9;
    c = initval;


    while (length >= 12) {
        uint32_t x;

        memcpy(&x, p + 0, 4);  a += x;
        memcpy(&x, p + 4, 4);  b += x;
        memcpy(&x, p + 8, 4);  c += x;

        MIX(a, b, c);
        p      += 12;
        length -= 12;
    }


    switch (length) {
    case 11: c += (uint32_t)p[10] << 16;
    case 10: c += (uint32_t)p[9]  << 8;
    case 9:  c += (uint32_t)p[8];
    case 8:  b += (uint32_t)p[7]  << 24;
    case 7:  b += (uint32_t)p[6]  << 16;
    case 6:  b += (uint32_t)p[5]  << 8;
    case 5:  b += (uint32_t)p[4];
    case 4:  a += (uint32_t)p[3]  << 24;
    case 3:  a += (uint32_t)p[2]  << 16;
    case 2:  a += (uint32_t)p[1]  << 8;
    case 1:  a += (uint32_t)p[0];
             break;
    case 0:   break;
    }

    FINAL(a, b, c);
    return c;
}

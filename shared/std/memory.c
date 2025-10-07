#include "memory.h"

int memcmp(const void *s1, const void *s2, unsigned long count) {
    const unsigned char *a = s1;
    const unsigned char *b = s2;
    for (unsigned long i = 0; i < count; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
    }
    return 0;
}

void* memset(void* dest, uint32_t val, size_t count) {
    uint8_t *d8 = (uint8_t *)dest;
    uint64_t pattern = ((uint64_t)val << 32) | val;

    while (((uintptr_t)d8 & 7) % 8 != 0 && count > 0) {
        *d8++ = (uint8_t)(val & 0xFF);
        count--;
    }

    size_t blocks = count / 128;
    uint64_t *d64 = (uint64_t *)d8;
    for (size_t i = 0; i < blocks; i++) {
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
        *(d64++) = pattern;
    }

    count %= 128;
    if (count >= 8) {
        for (size_t i = 0; i < count / 8; i++) {
            *d64++ = pattern;
        }
        count %= 8;
    } 
    d8 = (uint8_t *)d64;
    if (count >= 4) {
        *((uint32_t *)d8) = val;
        d8 += 4;
        count -= 4;
    }
    if (count >= 2) {
        *((uint16_t *)d8) = ((val & 0xFF) << 8) | (val & 0xFF);
        d8 += 2;
        count -= 2;
    }
    if (count == 1)
        *d8 = val & 0xFF;

    return dest;
}

void* memcpy(void *dest, const void *src, uint64_t count) {
    uint8_t *d8 = (uint8_t *)dest;
    const uint8_t *s8 = (const uint8_t *)src;

    while (count > 0 && (((uintptr_t)d8 & 7) != 0 || ((uintptr_t)s8 & 7) != 0)) {
        *d8++ = *s8++;
        count--;
    }

    size_t blocks = count / 128;
    uint64_t *d64 = (uint64_t *)d8;
    const uint64_t *s64 = (const uint64_t *)s8;
    for (size_t i = 0; i < blocks; i++) {
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
        *d64++ = *s64++;
    }

    count %= 128;
    if (count >= 8) {
        for (size_t i = 0; i < count / 8; i++) {
            *d64++ = *s64++;
        }
        count %= 8;
    }
    d8 = (uint8_t *)d64;
    s8 = (uint8_t *)s64;
    if (count >= 4) {
        *(uint32_t *)d8 = *(uint32_t *)s8;
        d8 += 4;
        s8 += 4;
        count -= 4;
    }
    if (count >= 2) {
        *(uint16_t *)d8 = *(uint16_t *)s8;
        d8 += 2;
        s8 += 2;
        count -= 2;
    }
    if (count == 1)
        *d8 = *s8;

    return dest;
}

void memreverse(void *ptr, size_t n) {
    if (n <= 1) return;

    uint8_t *l = (uint8_t*)ptr;
    uint8_t *r = l + n - 1;

    while (((uintptr_t)l & 7) &&l < r) {
        uint8_t t = *l;
        *l++ = *r;
        *r-- = t;
    }

    while (((uintptr_t)r & 7) && l < r) {
        uint8_t t = *l;
        *l++ = *r;
        *r-- = t;
    }

    while ((size_t)(r - l + 1) >= 16) {
        uint64_t *pl = (uint64_t*)l;
        uint64_t *pr = (uint64_t*)(r-7);

        uint64_t vl = *pl;
        uint64_t vr = *pr;

        *pl = bswap64(vr);
        *pr = bswap64(vl);

        l += 8;
        r -= 8;
    }

    while (l < r) {
        uint8_t t = *l;
        *l++ = *r;
        *r-- = t;
    }
}
void* memmove(void *dest, const void *src, uint64_t count) {
    if (dest == src || count == 0) return dest;

    uint8_t *d8 = (uint8_t *)dest;
    const uint8_t *s8 = (const uint8_t *)src;

    if (d8 < s8 || d8 >= s8 + count) {
        return memcpy(dest, src, count);
    } else {
        d8 += count;
        s8 += count;

        while (count > 0 && (((uintptr_t)d8 & 7) != 0 || ((uintptr_t)s8 & 7) != 0)) {
            --d8; --s8;
            *d8 = *s8;
            --count;
        }

        size_t blocks = count / 128;
        uint64_t *d64 = (uint64_t *)d8;
        const uint64_t *s64 = (const uint64_t *)s8;
        for (size_t i = 0; i < blocks * 16; i++) {
            --d64; --s64;
            *d64 = *s64;
        }

        count %= 128;
        if (count >= 8) {
            size_t q = count / 8;
            for (size_t i = 0; i < q; i++) {
                --d64; --s64;
                *d64 = *s64;
            }
            count %= 8;
        }

        d8 = (uint8_t *)d64;
        s8 = (const uint8_t *)s64;

        if (count >= 4) {
            d8 -= 4; s8 -= 4;
            *(uint32_t *)d8 = *(const uint32_t *)s8;
            count -= 4;
        }
        if (count >= 2) {
            d8 -= 2; s8 -= 2;
            *(uint16_t *)d8 = *(const uint16_t *)s8;
            count -= 2;
        }
        if (count == 1) {
            --d8; --s8;
            *d8 = *s8;
        }

        return dest;
    }
}

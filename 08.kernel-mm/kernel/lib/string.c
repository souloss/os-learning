#include "string.h"

void *memset(void *dest, int val, size_t len)
{
    unsigned char *ptr = dest;
    while (len--)
        *ptr++ = (unsigned char)val;
    return dest;
}

void *memcpy(void *dest, const void *src, size_t len)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (len--)
        *d++ = *s++;
    return dest;
}

void *memmove(void *dest, const void *src, size_t len)
{
    unsigned char *d = dest;
    const unsigned char *s = src;

    if (d == s)
        return dest;

    if (d < s)
    {
        while (len--)
            *d++ = *s++;
    }
    else
    {
        d += len;
        s += len;
        while (len--)
            *(--d) = *(--s);
    }
    return dest;
}

size_t strlen(const char *str)
{
    size_t len = 0;
    while (*str++)
        len++;
    return len;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strcpy(char *dest, const char *src)
{
    char *ret = dest;
    while ((*dest++ = *src++))
        ;
    return ret;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return dest;
}

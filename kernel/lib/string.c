/*
 * Project Genesis - string and memory routines.
 *
 * Plain byte loops: no vector instructions, no libc. They are also what the
 * compiler's own calls resolve to at link time.
 */

#include <lib/string.h>

void *memset(void *destination, int value, size_t count)
{
    unsigned char *bytes = (unsigned char *)destination;

    for (size_t index = 0; index < count; index++) {
        bytes[index] = (unsigned char)value;
    }

    return destination;
}

void *memcpy(void *destination, const void *source, size_t count)
{
    unsigned char *to = (unsigned char *)destination;
    const unsigned char *from = (const unsigned char *)source;

    for (size_t index = 0; index < count; index++) {
        to[index] = from[index];
    }

    return destination;
}

size_t strlen(const char *text)
{
    size_t length = 0;

    while (text[length] != '\0') {
        length++;
    }

    return length;
}

int strcmp(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        left++;
        right++;
    }

    return (int)(unsigned char)*left - (int)(unsigned char)*right;
}

int strncmp(const char *left, const char *right, size_t count)
{
    for (size_t index = 0; index < count; index++) {
        if (left[index] != right[index]) {
            return (int)(unsigned char)left[index] - (int)(unsigned char)right[index];
        }

        if (left[index] == '\0') {
            return 0;
        }
    }

    return 0;
}

char *strcpy(char *destination, const char *source)
{
    size_t index = 0;

    do {
        destination[index] = source[index];
    } while (source[index++] != '\0');

    return destination;
}

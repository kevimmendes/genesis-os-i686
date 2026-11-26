/*
 * Project Genesis - minimal freestanding string routines.
 *
 * GCC may emit calls to memcpy/memset even in freestanding code, so these are
 * part of the kernel rather than an optional convenience.
 */

#ifndef GENESIS_LIB_STRING_H
#define GENESIS_LIB_STRING_H

#include <stddef.h>

void *memset(void *destination, int value, size_t count);
void *memcpy(void *destination, const void *source, size_t count);

size_t strlen(const char *text);
int strcmp(const char *left, const char *right);
int strncmp(const char *left, const char *right, size_t count);
char *strcpy(char *destination, const char *source);

#endif /* GENESIS_LIB_STRING_H */

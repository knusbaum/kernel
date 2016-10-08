#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

void *memset(void *p, int c, size_t count);
void *memcpy(void *dest, const void *src, size_t n);

int toupper(int c);
int tolower(int c);

size_t strlen(const char* str);
int strcmp(const char *s1, const char *s2);

void PANIC(char *err);

#endif

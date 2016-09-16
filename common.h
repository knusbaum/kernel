#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

void *memset(void *p, int c, size_t count);

void PANIC(char *err);

#endif

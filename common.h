#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>

void *memset(void *p, int c, size_t count);
void *memcpy(void *dest, const void *src, size_t n);

int toupper(int c);
int tolower(int c);

char * itos(uint32_t myint, char buffer[], int bufflen);

size_t strlen(const char* str);
int strcmp(const char *s1, const char *s2);

int coerce_int(char *s, uint32_t *val);
uint8_t hex_char(uint8_t byte);

void PANIC(char *err);

#endif

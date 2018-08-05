#ifndef STDIO_H
#define STDIO_H

#include "stdint.h"
#include "stddef.h"

typedef struct FILE FILE;

FILE *fopen(const char *pathname, const char *mode);
FILE *fdopen(int fd, const char *mode);
FILE *freopen(const char *pathname, const char *mode, FILE *stream);

int fclose(FILE *stream);

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

#endif

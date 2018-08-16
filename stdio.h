#ifndef STDIO_H
#define STDIO_H

#include "stdint.h"
#include "stddef.h"
#include "kernio.h"

typedef struct FILE FILE;

struct fops {
    int (*fclose)(struct fops *stream);
    size_t (*fread)(void *ptr, size_t size, size_t nmemb, struct fops *stream);
    size_t (*fwrite)(const void *ptr, size_t size, size_t nmemb, struct fops *stream);
    void (*clearerr)(struct fops *stream);
    int (*feof)(struct fops *stream);
    int (*ferror)(struct fops *stream);
};

#define EOF (1<<8)

FILE *fopen(const char *pathname, const char *mode);
//FILE *freopen(const char *pathname, const char *mode, FILE *stream);

int fclose(FILE *stream);

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);


void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
//int fileno(FILE *stream);

//int fgetc(FILE *stream);
//char *fgets(char *s, int size, FILE *stream);
int getc(FILE *stream);

#define stdin stdino

extern FILE *stdino;
void setup_stdin();

#endif

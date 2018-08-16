#include "keyboard.h"
#include "stddef.h"
#include "stdio.h"
#include "fat32_io.h"
#include "common.h"

//typedef struct FILE {
//    //struct dir_entry file_ent;
//    uint32_t curr_cluster;
//    uint32_t file_size; // total file size
//    uint32_t fptr; // index into the file
//    uint32_t buffptr; // index into currbuf
//    uint8_t currbuf[]; // flexible member for current cluster
//} FILE;

typedef struct FILE {
    struct fops ops;
} FILE;

FILE *fopen(const char *pathname, const char *mode) {
    // Right now we only do fat32.
    return (FILE *)fat32_fopen(pathname, mode);
}

int fclose(FILE *stream) {
    return stream->ops.fclose(stream);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return stream->ops.fread(ptr, size, nmemb, stream);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return stream->ops.fwrite(ptr, size, nmemb, stream);
}

void clearerr(FILE *stream) {
    stream->ops.clearerr(stream);
}

int feof(FILE *stream) {
    return stream->ops.feof(stream);
}

int ferror(FILE *stream) {
    return stream->ops.ferror(stream);
}

int getc(FILE *stream) {
    char c;
    if(feof(stream)) {
        return EOF;
    }
    stream->ops.fread(&c, 1, 1, stream);
//    printf("%c", c);
    return c;
}

FILE *stdino;

int stdin_fclose(struct fops *stream) {}

size_t stdin_fread(void *ptr, size_t size, size_t nmemb, struct fops *stream) {
    size_t to_read = size * nmemb;
    char *p = ptr;
    if(to_read == 0) {
        return 0;
    }
    *p++ = get_ascii_char();
    to_read--;
    while(to_read > 0 && has_pressed_keys()) {
        *p++ = get_ascii_char();
    }
    return (size * nmemb) - to_read;
}

//size_t stdin_fwrite(const void *ptr, size_t size, size_t nmemb, struct fops *stream);
void stdin_clearerr(struct fops *stream) {}
int stdin_feof(struct fops *stream) {return 0;}
int stdin_ferror(struct fops *stream) {return 0;}

void setup_stdin() {
    stdino = kmalloc(sizeof (struct fops));
    stdino->ops.fclose = stdin_fclose;
    stdino->ops.fread = stdin_fread;
    stdino->ops.clearerr = stdin_clearerr;
    stdino->ops.feof = stdin_feof;
    stdino->ops.ferror = stdin_ferror;
}

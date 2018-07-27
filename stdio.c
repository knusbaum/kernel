#include "stddef.h"
#include "stdio.h"
#include "fat32.h"
#include "common.h"

typedef struct FILE {
    struct dir_entry *file_ent;
} FILE;

static inline int entry_for_path(const char *path, struct dir_entry *entry) {
    struct directory dir;
    populate_root_dir(master_fs, &dir);
    int found_file = 0;
    if(path[0] != '/') {
        return found_file;
    }
    
    char *cutpath = strdup(path);
    char *tokstate;
    char *next_dir = strtok_r(cutpath, '/', &tokstate);  
    while (next_dir) {
        for(int entryi = 0; entryi < dir.num_entries; i++) {
            
        }
        strtok_r(NULL, '/'
    }
    
}

FILE *fopen(const char *pathname, const char *mode) {
    
}

FILE *fdopen(int fd, const char *mode);
FILE *freopen(const char *pathname, const char *mode, FILE *stream);

int fclose(FILE *stream);

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

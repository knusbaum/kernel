#include "stddef.h"
#include "stdio.h"
#include "fat32.h"
#include "common.h"

typedef struct FILE {
    //struct dir_entry file_ent;
    uint32_t curr_cluster;
    uint32_t file_size; // total file size
    uint32_t fptr; // index into the file
    uint32_t buffptr; // index into currbuf
    uint8_t currbuf[]; // flexible member for current cluster
} FILE;

//static inline void dump_dir(struct directory *dir) {
//    for(int entryi = 0; entryi < dir->num_entries; entryi++) {
//        printf("\t%s\n", dir->entries[entryi]);
//    }
//}

static inline int entry_for_path(const char *path, struct dir_entry *entry) {
    struct directory dir;
    populate_root_dir(master_fs, &dir);
    int found_file = 0;
    if(path[0] != '/') {
        return found_file;
    }

    char *cutpath = strdup(path);
    char *tokstate = NULL;
    char *next_dir = strtok_r(cutpath, "/", &tokstate);
    struct dir_entry *currentry = NULL;
    entry->name = NULL;
    while (next_dir) {
        int found = 0;
        for(int entryi = 0; entryi < dir.num_entries; entryi++) {
            currentry = &dir.entries[entryi];
            if(strcmp(currentry->name, next_dir) == 0) {
                if(entry->name) kfree(entry->name);
                *entry = *currentry;
                // TODO: Make sure this doesn't leak. Very dangerous:
                entry->name = strdup(currentry->name);

                uint32_t cluster = currentry->first_cluster;
                free_directory(master_fs, &dir);
                populate_dir(master_fs, &dir, cluster);
                found = 1;
                break;

            }
        }
        if(!found) {
            kfree(cutpath);
            free_directory(master_fs, &dir);
            return 0;
        }
        next_dir = strtok_r(NULL, "/", &tokstate);
    }
    free_directory(master_fs, &dir);
    kfree(cutpath);
    return 1;
}

FILE *fopen(const char *pathname, const char *mode) {
    struct dir_entry entry;
    if(!entry_for_path(pathname, &entry)) {
        kfree(entry.name);
        return NULL;
    }
//    printf("Got entry: %s [%d]\n", entry.name, entry.first_cluster);
    kfree(entry.name);

    FILE *f = kmalloc(sizeof (FILE) + master_fs->cluster_size);
    f->curr_cluster = entry.first_cluster;
    f->file_size = entry.file_size;
    f->fptr = 0;
    f->buffptr = 0;
    getCluster(master_fs, f->currbuf, f->curr_cluster);
    return f;
}

FILE *fdopen(int fd, const char *mode);
FILE *freopen(const char *pathname, const char *mode, FILE *stream);

int fclose(FILE *stream) {
    kfree(stream);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t bytes_to_read = size * nmemb;
    size_t bytes_read = 0;

    if(stream->fptr + bytes_to_read > stream->file_size) {
        bytes_to_read = stream->file_size - stream->fptr;
    }
    //printf("Reading %d bytes.\n", bytes_to_read);
    while(bytes_to_read > 0) {
        if(stream->buffptr + bytes_to_read > master_fs->cluster_size) {
            // Need to read at least 1 more cluster
            size_t to_read_in_this_cluster = master_fs->cluster_size - stream->buffptr;
            memcpy(ptr + bytes_read, stream->currbuf + stream->buffptr, to_read_in_this_cluster);
            bytes_read += to_read_in_this_cluster;
            //printf("buffptr = 0\n");
            stream->buffptr = 0;
            //printf("Getting next cluster.\n");
            stream->curr_cluster = get_next_cluster_id(master_fs, stream->curr_cluster);
            //printf("Next cluster: %x\n", stream->curr_cluster);
            if(stream->curr_cluster >= EOC) {
                //printf("Returning.\n");
                stream->fptr += bytes_read;
                return bytes_read;
            }
            //printf("getting next cluster.\n");
            getCluster(master_fs, stream->currbuf, stream->curr_cluster);
            bytes_to_read -= to_read_in_this_cluster;
        }
        else {
            //printf("buffptr: %d\n", stream->buffptr);
            memcpy(ptr + bytes_read, stream->currbuf + stream->buffptr, bytes_to_read);
            bytes_read += bytes_to_read;
            stream->buffptr += bytes_to_read;
            bytes_to_read = 0;
        }
    }
    //printf("Returning.\n");
    stream->fptr += bytes_read;
    return bytes_read;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

#include "stdio.h"
#include "fat32.h"
#include "fat32_io.h"

struct fat32_fops {
    //struct dir_entry file_ent;
    struct fops ops;
    uint32_t curr_cluster;
    uint32_t file_size; // total file size
    uint32_t fptr; // index into the file
    uint32_t buffptr; // index into currbuf
    uint8_t currbuf[]; // flexible member for current cluster
};
    
static inline int entry_for_path(const char *path, struct dir_entry *entry) {
    printf("populating root dir\n");
    struct directory dir;
    populate_root_dir(master_fs, &dir);
    printf("done populating root dir\n");
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
        printf("next_dir: %s\n", next_dir);
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

int fat32_fclose(struct fops *stream) {
    kfree(stream);
}

size_t fat32_fread(void *ptr, size_t size, size_t nmemb, struct fops *stream) {
    struct fat32_fops *fstream = stream;
    size_t bytes_to_read = size * nmemb;
    size_t bytes_read = 0;

    if(fstream->fptr + bytes_to_read > fstream->file_size) {
        bytes_to_read = fstream->file_size - fstream->fptr;
    }
    //printf("Reading %d bytes.\n", bytes_to_read);
    while(bytes_to_read > 0) {
        if(fstream->buffptr + bytes_to_read > master_fs->cluster_size) {
            // Need to read at least 1 more cluster
            size_t to_read_in_this_cluster = master_fs->cluster_size - fstream->buffptr;
            memcpy(ptr + bytes_read, fstream->currbuf + fstream->buffptr, to_read_in_this_cluster);
            bytes_read += to_read_in_this_cluster;
            //printf("buffptr = 0\n");
            fstream->buffptr = 0;
            //printf("Getting next cluster.\n");
            fstream->curr_cluster = get_next_cluster_id(master_fs, fstream->curr_cluster);
            //printf("Next cluster: %x\n", fstream->curr_cluster);
            if(fstream->curr_cluster >= EOC) {
                //printf("Returning.\n");
                fstream->fptr += bytes_read;
                return bytes_read;
            }
            //printf("getting next cluster.\n");
            getCluster(master_fs, fstream->currbuf, fstream->curr_cluster);
            bytes_to_read -= to_read_in_this_cluster;
        }
        else {
            //printf("buffptr: %d\n", fstream->buffptr);
            memcpy(ptr + bytes_read, fstream->currbuf + fstream->buffptr, bytes_to_read);
            bytes_read += bytes_to_read;
            fstream->buffptr += bytes_to_read;
            bytes_to_read = 0;
        }
    }
    //printf("Returning.\n");
    fstream->fptr += bytes_read;
    return bytes_read;
}

//size_t fat32_fwrite(const void *ptr, size_t size, size_t nmemb, struct fops *stream);

void fat32_clearerr(struct fops *stream) {}
int fat32_feof(struct fops *stream) {
    struct fat32_fops *fstream = stream;
    return fstream->fptr == fstream->file_size;
}
int fat32_ferror(struct fops *stream) {
    return 0;
}

struct fops *fat32_fopen(const char *pathname, const char *mode) {
    printf("Getting entry for path.\n");
    struct dir_entry entry;
    if(!entry_for_path(pathname, &entry)) {
        //kfree(entry.name);
        return NULL;
    }
    printf("Got entry for path.\n");
//    printf("Got entry: %s [%d]\n", entry.name, entry.first_cluster);
    kfree(entry.name);

    struct fat32_fops *f = kmalloc(sizeof (struct fat32_fops) + master_fs->cluster_size);
    f->ops.fclose = fat32_fclose;
    f->ops.fread = fat32_fread;
//    f->ops.fwrite = fat32_fwrite; // WRITE not implemented.
    f->ops.clearerr = fat32_clearerr;
    f->ops.feof = fat32_feof;
    f->ops.ferror = fat32_ferror;
    
    f->curr_cluster = entry.first_cluster;
    f->file_size = entry.file_size;
    f->fptr = 0;
    f->buffptr = 0;
    getCluster(master_fs, f->currbuf, f->curr_cluster);
    return f;
}

//FILE *freopen(const char *pathname, const char *mode, FILE *stream);

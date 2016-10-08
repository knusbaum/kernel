#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "fat32.h"

int handle_commands(f32 *fs, struct directory *dir, char *buffer);

int main(int argc, char **argv) {

    char *filename = NULL;
    if(argc > 1) {
        filename = argv[1];
    }
    else {
        filename = "f32.disk";
    }

    f32 *fs = makeFilesystem(filename);
    if(fs == NULL) {
        printf("Failed to read file [%s]. Does not exist or it's not a FAT32 fs.\n", filename);
        printf("You may specify a file to read as a FAT32 partition as the first argument, or the program will look for a file called f32.disk.\n");
        exit(1);
    }

    const struct bios_parameter_block *bpb = getBPB(fs);

    printf("Bytes per sector: %d\n", bpb->bytes_per_sector);
    printf("Sectors per cluster: %d\n", bpb->sectors_per_cluster);
    printf("Reserved sectors: %d\n", bpb->reserved_sectors);
    printf("FAT_count: %d\n", bpb->FAT_count);
    printf("dir_entries: %d\n", bpb->dir_entries);
    printf("total_sectors: %d\n", bpb->total_sectors);
    printf("media_descriptor_type: %d\n", bpb->media_descriptor_type);
    printf("count_sectors_per_FAT12_16: %d\n", bpb->count_sectors_per_FAT12_16);
    printf("count_sectors_per_track: %d\n", bpb->count_sectors_per_track);
    printf("count_heads_or_sizes_on_media: %d\n", bpb->count_heads_or_sizes_on_media);
    printf("count_hidden_sectors: %d\n", bpb->count_hidden_sectors);
    printf("large_sectors_on_media: %d\n", bpb->large_sectors_on_media);
    // Extended Boot Record:
    printf("count_sectors_per_FAT32: %d\n", bpb->count_sectors_per_FAT32);
    printf("flags: %d\n", bpb->flags);
    printf("FAT_version: %d\n", bpb->FAT_version);
    printf("cluster_number_root_dir: %d\n", bpb->cluster_number_root_dir);
    printf("sector_number_FSInfo: %d\n", bpb->sector_number_FSInfo);
    printf("sector_number_backup_boot_sector: %d\n", bpb->sector_number_backup_boot_sector);
    printf("drive_number: %d\n", bpb->drive_number);
    printf("windows_flags: %d\n", bpb->windows_flags);
    printf("signature: %d\n", bpb->signature);
    printf("volume_id: %d\n", bpb->volume_id);
    printf("volume_label: [%s]\n", bpb->volume_label);
    printf("system_id: [%s]\n", bpb->system_id);



    printf("Reading root directory.\n");
    struct directory dir;
    populate_root_dir(fs, &dir);
    printf("Done reading root directory.\n");

    uint32_t bufflen = 24;
    printf("Entering command line!\n");

    printf("\n\nROOT DIRECTORY: cluster[%d]\n", 2);
    print_directory(fs, &dir);
    printf("Hello! Type 'help' to see available commands.\n");

    char buffer[bufflen + 1];
    while(1) {
        printf(">> ");
        int i;
        for(i = 0; i < bufflen; i++) {
            int c = fgetc(stdin);
            if(c == EOF) {
                printf("\nDone!\n");
                free_directory(fs, &dir);
                destroyFilesystem(fs);
                exit(0);
            }
            buffer[i] = c;
            if(c == '\n') break;
        }

        if(i == bufflen) {
            printf("Input too long.\n");
            while(fgetc(stdin) != '\n');
            continue;
        }
        buffer[i] = 0;

        // If it's just a return, print the current directory.
        if(strlen(buffer) == 0) {
            print_directory(fs, &dir);
            continue;
        }

        int x;
        int scanned = sscanf(buffer, "%u", &x);
        if(scanned == 0) {
            int command_ret = handle_commands(fs, &dir, buffer);
            if(!command_ret) {
                printf("Invalid input. Enter a number or command.\n");
            }
            else if(command_ret == -1) {
                break;
            }
            continue;
        }

        if(dir.num_entries <= x) {
            printf("Invalid selection.\n");
            continue;
        }

        if(dir.entries[x].dir_attrs & DIRECTORY) {
            // It's a directory. chdir to that one.
            uint32_t cluster = dir.entries[x].first_cluster;
            if(cluster == 0) cluster = 2;
            free_directory(fs, &dir);
            populate_dir(fs, &dir, cluster);
            print_directory(fs, &dir);
            continue;
        }
        else {
            char *file = readFile(fs, &dir.entries[x]);
            for(i = 0; i < dir.entries[x].file_size; i++) {
                putchar(file[i]);
            }
            free(file);
        }
    }
    free_directory(fs, &dir);
    destroyFilesystem(fs);
}

struct bytes {
    char *buff;
    size_t len;
    char *err;
};

struct bytes readLocalFile(char *fname) {
    char *buff = malloc(1024);
    if(buff == NULL) {
        return (struct bytes) {NULL, 0, "Failed to allocate buffer."};
    }
    size_t curlen = 1024;
    size_t totalRead = 0;
    FILE *f = fopen(fname, "r");
    if(f == NULL) {
        free(buff);
        return (struct bytes) {NULL, 0, strerror(errno)};
    }
    do {
        totalRead += fread(buff + totalRead, 1, curlen - totalRead, f);
        if(ferror(f)) {
            free(buff);
            fclose(f);
            return (struct bytes) {NULL, 0, strerror(errno)};
        }
        if(totalRead == curlen) {
            curlen *= 2;
            char *newbuff = realloc(buff, curlen);
            if(newbuff == NULL) {
                free(buff);
                fclose(f);
                return (struct bytes) {NULL, 0, "Failed to allocate buffer."};
            }
            buff = newbuff;
        }
    } while(!feof(f));
    fclose(f);
    return (struct bytes) {buff, totalRead, NULL};
}

void do_copy(f32 *fs, struct directory *dir, char *filename) {
    if(filename == NULL) return;
    struct bytes bs = readLocalFile(filename);
    if(bs.buff == NULL) {
        printf("Couldn't read file [%s]\n", filename);
        return;
    }
    writeFile(fs, dir, bs.buff, filename, bs.len);
    free(bs.buff);
}

void do_delete(f32 *fs, struct directory *dir, char *filename) {
    printf("do_delete(%s)\n", filename);
    int i;
    for(i = 0; i < dir->num_entries; i++) {
        if(strcmp(filename, dir->entries[i].name) == 0) {
            if(dir->entries[i].dir_attrs & DIRECTORY) {
                struct directory subdir;
                populate_dir(fs, &subdir, dir->entries[i].first_cluster);
                int j;
                for(j = 0; j < subdir.num_entries; j++) {
                    // Delete these last!
                    if(strcmp(subdir.entries[j].name, ".") == 0) {
                        // Don't recur on current directory!
                        continue;
                    }
                    if(strcmp(subdir.entries[j].name, "..") == 0) {
                        // Don't recur on parent directory.
                        continue;
                    }
//                    printf("Deleting [%s/%s]\n", dir->entries[i].name, subdir.entries[j].name);
                    do_delete(fs, &subdir, subdir.entries[j].name);
                }
                // Now delete '.' and '..'
                for(j = 0; j < subdir.num_entries; j++) {
                    if(strcmp(subdir.entries[j].name, ".") == 0) {
                        // Don't recur on current directory!
//                        printf("Deleting dot file [%s/%s]\n", dir->entries[i].name, subdir.entries[j].name);
                        delFile(fs, dir, subdir.entries[j].name);
                        continue;
                    }
                    if(strcmp(subdir.entries[j].name, "..") == 0) {
                        // Don't recur on parent directory.
//                        printf("Deleting dot file [%s/%s]\n", dir->entries[i].name, subdir.entries[j].name);
                        delFile(fs, dir, subdir.entries[j].name);
                        continue;
                    }
                }
                free_directory(fs, &subdir);
                // Finally, delete the directory itself.
//                printf("Deleting dir [%s]\n", filename);
                delFile(fs, dir, filename);
            }
            else {
//                printf("Deleting file [%s]\n", dir->entries[i].name);
                delFile(fs, dir, dir->entries[i].name);
            }
        }
    }
}

void do_touch(f32 *fs, struct directory *dir, char *filename) {
    writeFile(fs, dir, "", filename, 0);
}

int handle_commands(f32 *fs, struct directory *dir, char *buffer) {
    char *command = NULL;
    char *filename = NULL;
    int scanned = sscanf(buffer, "%ms %ms", &command, &filename);
    if(scanned == 0) { printf("Failed to parse command.\n"); return 0; }

    int ret = 0;
    if(strcmp(command, "copy") == 0) {
        do_copy(fs, dir, filename);
        ret = 1;
    }
    if(strcmp(command, "mkdir") == 0) {
        if(filename != NULL && strlen(filename) > 0) {
            printf("Making directory [%s].\n", filename);
            mkdir(fs, dir, filename);
        }
        else {
            printf("Need a directory name.\n");
        }
        ret = 1;
    }
    if(strcmp(command, "touch") == 0) {
        if(filename != NULL && strlen(filename) > 0) {
            do_touch(fs, dir, filename);
        }
        ret = 1;
    }
    if(strcmp(command, "ls") == 0) {
        system("ls -l");
        ret = 1;
    }
    if(strcmp(command, "del") == 0) {
        if(filename != NULL && strlen(filename) > 0) {
            do_delete(fs, dir, filename);
        }
        else {
            printf("Need a file/directory name.\n");
        }
        ret = 1;
    }
    if(strcmp(command, "freeclusters") == 0) {
        printf("Free clusters in FAT: %u\n", count_free_clusters(fs));
        ret = 1;
    }
    if(strcmp(command, "exit") == 0) {
        printf("See ya!\n");
        ret = -1;
    }
    if(strcmp(command, "help") == 0) {
        printf("Commands are: \n");
        printf("\t[a number] -> a number corresponding with a file or directory will print that file or enter that directory.\n");
        printf("\t(return) -> pressing return without entering a command will list the current directory. Entries marked with a 'D' next to their names are directories.\n");
        printf("\tcopy [filename] -> Copies a filename within the CWD of this program into the current directory of the FAT filesystem. (see ls)\n");
        printf("\tmkdir [dirname] -> Create a directory in the current directory.\n");
        printf("\ttouch [filename] -> Create an empty file in the current directory.\n");
        printf("\tls -> Call ls to list the files in the CWD of this program. Useful in conjunction with the 'copy' command to copy files into the FAT filesystem.\n");
        printf("\tdel [filename | dirname] -> Delete a file or (recursively) a directory.\n");
        printf("\tfreeclusters -> Count the free clusters available in the filesystem.\n");
        printf("\texit -> Exit this program. This gracefully closes the filesystem. Sending an EOF on stdin works as well. Ctrl+C and other signals should be avoided to prevent filesystem corruption.\n");
        ret = 1;
    }
    if(command) free(command);
    if(filename) free(filename);
    free_directory(fs, dir);
    populate_dir(fs, dir, dir->cluster);
    return ret;
}

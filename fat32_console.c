#include "stddef.h"
#include "common.h"
#include "fat32.h"
#include "keyboard.h"
#include "kheap.h"
#include "kernio.h"
#include "terminal.h"

int handle_commands(f32 *fs, struct directory *dir, char *buffer);

void fat32_console(f32 *fs) {
    printf("\n\nStarting up Fat32 console...\n");
    printf("Reading root directory... ");
    struct directory dir;
    populate_root_dir(fs, &dir);
    printf("Done.\n");

    uint32_t bufflen = 24;

    if(dir.num_entries > 0) {
        printf("Root directory:\n");
        print_directory(fs, &dir);
    }
    else {
        printf("Root directory empty.\n");
    }
    printf("Welcome to the Version 1 Fat32 console.\n");
    printf("Type 'help' to see available commands.\n");

    char buffer[bufflen + 1];
    while(1) {
        printf(">> ");
        uint32_t i;
        for(i = 0; i < bufflen; i++) {
            char c = get_ascii_char();
            if(c == BS) {
                if(i == 0) {
                    i--;
                    continue;
                }
                printf("%c", c);
                i-=2;
                continue;
            }
            if(c == EOT || c == ESC) {
                i--;
                continue;
            }
            printf("%c", c);

            buffer[i] = c;
            if(c == '\n') break;
        }

        if(i == bufflen) {
            printf("Input too long.\n");
            while(get_ascii_char() != '\n');
            continue;
        }
        buffer[i] = 0;

        // If it's just a return, print the current directory.
        if(strlen(buffer) == 0) {
            print_directory(fs, &dir);
            continue;
        }

        uint32_t x;
        int scanned = coerce_int(buffer, &x);
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
            uint8_t *file = readFile(fs, &dir.entries[x]);
            for(i = 0; i < dir.entries[x].file_size; i++) {
                printf("%c", file[i]);
            }
            kfree(file);
        }
    }
    printf("Shutting down filesystem.\n");
    free_directory(fs, &dir);
    destroyFilesystem(fs);
}

struct bytes {
    char *buff;
    size_t len;
    char *err;
};

void do_delete(f32 *fs, struct directory *dir, char *filename) {
    printf("do_delete(%s)\n", filename);
    uint32_t i;
    for(i = 0; i < dir->num_entries; i++) {
        if(strcmp(filename, dir->entries[i].name) == 0) {
            if(dir->entries[i].dir_attrs & DIRECTORY) {
                struct directory subdir;
                populate_dir(fs, &subdir, dir->entries[i].first_cluster);
                uint32_t j;
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
                    do_delete(fs, &subdir, subdir.entries[j].name);
                }
                // Now delete '.' and '..'
                for(j = 0; j < subdir.num_entries; j++) {
                    if(strcmp(subdir.entries[j].name, ".") == 0) {
                        // Don't recur on current directory!
                        delFile(fs, dir, subdir.entries[j].name);
                        continue;
                    }
                    if(strcmp(subdir.entries[j].name, "..") == 0) {
                        // Don't recur on parent directory.
                        delFile(fs, dir, subdir.entries[j].name);
                        continue;
                    }
                }
                free_directory(fs, &subdir);
                // Finally, delete the directory itself.
                delFile(fs, dir, filename);
            }
            else {
                delFile(fs, dir, dir->entries[i].name);
            }
        }
    }
}

void do_cat(f32 *fs, struct directory *dir, char *filename) {
    uint32_t i;
    for(i = 0; i < dir->num_entries; i++) {
        if(strcmp(filename, dir->entries[i].name) == 0) {
            printf("File already exists. del the file first.\n");
            return;
        }
    }
    printf("Ctrl+D to end file.\n");

    uint32_t index = 0;
    uint32_t buffsize = 1024;
    uint32_t currlinelen = 0;
    uint8_t *file = kmalloc(buffsize);
    while(1) {
        char c = get_ascii_char();
        if(c == EOT) {
            break;
        }
        printf("%c", c);
        if(c == BS) {
            if(currlinelen > 0) {
                index--;
                currlinelen--;
            }
            continue;
        }
        file[index++] = c;
        currlinelen++;
        if(c == '\n') {
            currlinelen = 0;
        }
        if(index == buffsize) {
            buffsize *= 2;
            file = krealloc(file, buffsize);
        }
    }
    writeFile(fs, dir, file, filename, index);
    kfree(file);
}

void do_touch(f32 *fs, struct directory *dir, char *filename) {
    writeFile(fs, dir, (uint8_t *)"", filename, 0);
}

int scan_command(char *buffer, char **comm, char **fname) {
    char *buffscan = buffer;
    if(!*buffscan) {
        // There's nothing in the buffer.
        return 0;
    }

    // skip any whitespace
    while(*buffscan && *buffscan == ' ') {
        buffscan++;
    }

    // Make sure there's something afterwards
    if(!*buffscan) {
        return 0;
    }

    // Point comm at the first non-whitespace character.
    *comm = buffscan;

    // Find a space.
    while(*buffscan && *buffscan != ' ') {
        buffscan++;
    }

    if(!*buffscan) {
        // There's no more in the string
        return 1;
    }
    // Terminate the string.
    *buffscan = 0;
    buffscan++;

    // skip any whitespace
    while(*buffscan && *buffscan == ' ') {
        buffscan++;
    }

    // If there's no more string left, return 1.
    if(!*buffscan) {
        return 1;
    }

    *fname = buffscan;

    // Chop any space off the end.
    while(*buffscan && *buffscan != ' ') {
        buffscan++;
    }
    *buffscan = 0;

    return 2;
}

int handle_commands(f32 *fs, struct directory *dir, char *buffer) {
    char *command = NULL;
    char *filename = NULL;
    int scanned = scan_command(buffer, &command, &filename);
    if(scanned == 0) { printf("Failed to parse command.\n"); return 0; }

    int ret = 0;
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
    if(strcmp(command, "del") == 0) {
        if(filename != NULL && strlen(filename) > 0) {
            do_delete(fs, dir, filename);
        }
        else {
            printf("Need a file/directory name.\n");
        }
        ret = 1;
    }
    if(strcmp(command, "cat") == 0) {
        if(filename != NULL && strlen(filename) > 0) {
            do_cat(fs, dir, filename);
        }
        else {
            printf("Need a filename.\n");
        }
        ret = 1;
    }
    if(strcmp(command, "freeclusters") == 0) {
        printf("Free clusters in FAT: %d\n", count_free_clusters(fs));
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
        printf("\tmkdir [dirname] -> Create a directory in the current directory.\n");
        printf("\ttouch [filename] -> Create an empty file in the current directory.\n");
        printf("\tcat [filename] -> Creates a file named 'filename' and lets you type into it. Ctrl+D ends the file.\n");
        printf("\tdel [filename | dirname] -> Delete a file or (recursively) a directory.\n");
        printf("\tfreeclusters -> Count the free clusters available in the filesystem.\n");
        printf("\texit -> Exit the FAT32 shell. This gracefully closes the filesystem. Always do this before shutting down the kernel.\n");
        ret = 1;
    }
    free_directory(fs, dir);
    populate_dir(fs, dir, dir->cluster);
    return ret;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "fat32.h"

int main(void) {
    f32 *fs = makeFilesystem("f32.disk");
    if(fs == NULL) {
        printf("Failed to read file. It's not a FAT32 fs.\n");
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


    printf("\n\nROOT DIRECTORY: cluster[%d]\n", 2);
    print_directory(fs, &dir);

    char *file = "Hello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\nHello, world!\n";

    //writeFile(fs, &dir, "Hello, world!\n", "hello_to_you.txt", 14);
    //writeFile(fs, &dir, file, "hello_to_you.txt", strlen(file));
    //exit(0);

    uint32_t bufflen = 24;

    char buffer[bufflen + 1];
    while(1) {
        printf("SELECT NUMBER: ");
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
            printf("Invalid input. Enter a number.\n");
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
}

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <strings.h>
#include <errno.h>
#include <ctype.h>
#include "fat32.h"

int x;

struct f32 {
    FILE *f;
    uint32_t *FAT;
    struct bios_parameter_block bpb;
    uint32_t partition_begin_sector;
    uint32_t fat_begin_sector;
    uint32_t cluster_begin_sector;
    uint32_t cluster_size;
};

static void read_bpb(f32 *fs, struct bios_parameter_block *bpb);
static uint32_t sector_for_cluster(f32 *fs, uint32_t cluster);

static void trim_spaces(char *c, int max) {
    int i = 0;
    while(*c != ' ' && i++ < max) {
        c++;
    }
    if(*c == ' ') *c = 0;
}

f32 *makeFilesystem(char *fatSystem) {
    f32 *fs = malloc(sizeof (struct f32));
    fs->f = fopen(fatSystem, "r");
    rewind(fs->f);
    read_bpb(fs, &fs->bpb);

    trim_spaces(fs->bpb.system_id, 8);
    if(strcmp(fs->bpb.system_id, "FAT32") != 0) {
        fclose(fs->f);
        free(fs);
        return NULL;
    }

    fs->partition_begin_sector = 0;
    fs->fat_begin_sector = fs->partition_begin_sector + fs->bpb.reserved_sectors;
    fs->cluster_begin_sector = fs->fat_begin_sector + (fs->bpb.FAT_count * fs->bpb.count_sectors_per_FAT32);
    fs->cluster_size = 512 * fs->bpb.sectors_per_cluster;

    // Load the FAT
    uint32_t bytes_per_fat = 512 * fs->bpb.count_sectors_per_FAT32;
    fs->FAT = malloc(bytes_per_fat);
    int sector_i;
    for(sector_i = 0; sector_i < fs->bpb.count_sectors_per_FAT32; sector_i++) {
        char sector[512];
        getSector(fs, sector, fs->fat_begin_sector + sector_i, 1);
        int integer_j;
        for(integer_j = 0; integer_j < 512/4; integer_j++) {
            fs->FAT[sector_i * (512 / 4) + integer_j]
                = readi32(sector, integer_j * 4);
        }
    }
    return fs;
}

void destroyFilesystem(f32 *fs) {
    fflush(fs->f);
    fclose(fs->f);
    free(fs->FAT);
    free(fs);
}

void getSector(f32 *fs, char *buff, uint32_t sector, uint32_t count) {
    uint32_t readbytes = count * 512;
    //char *buff = malloc(readbytes); // sector is 512 bytes.
    fseek(fs->f, sector * 512, SEEK_SET);
    int readcount = 0;
    while(readcount < readbytes) {
        if(feof(fs->f) || ferror(fs->f)) {
            return;
        }
        readcount += fread(buff + readcount, 1, readbytes - readcount, fs->f);
    }
}

uint16_t readi16(char *buff, size_t offset)
{
    unsigned char *ubuff = buff + offset;
    return ubuff[1] << 8 | ubuff[0];
}
uint32_t readi32(char *buff, size_t offset) {
    unsigned char *ubuff = buff + offset;
    return
        ((ubuff[3] << 24) & 0xFF000000) |
        ((ubuff[2] << 16) & 0x00FF0000) |
        ((ubuff[1] << 8) & 0x0000FF00) |
        (ubuff[0] & 0x000000FF);
}

/**
 * 11 2 The number of Bytes per sector (remember, all numbers are in the little-endian format).
 * 13 1 Number of sectors per cluster.
 * 14 2 Number of reserved sectors. The boot record sectors are included in this value.
 * 16 1 Number of File Allocation Tables (FAT's) on the storage media. Often this value is 2.
 * 17 2 Number of directory entries (must be set so that the root directory occupies entire sectors).
 * 19 2 The total sectors in the logical volume. If this value is 0, it means there are more than 65535 sectors in the volume, and the actual count is stored in "Large Sectors (bytes 32-35).
 * 21 1 This Byte indicates the media descriptor type.
 * 22 2 Number of sectors per FAT. FAT12/FAT16 only.
 * 24 2 Number of sectors per track.
 * 26 2 Number of heads or sides on the storage media.
 * 28 4 Number of hidden sectors. (i.e. the LBA of the beginning of the partition.)
 * 32 4 Large amount of sector on media. This field is set if there are more than 65535 sectors in the volume.
 */

/**
 * 36 4 Sectors per FAT. The size of the FAT in sectors.
 * 40 2 Flags.
 * 42 2 FAT version number. The high byte is the major version and the low byte is the minor version. FAT drivers should respect this field.
 * 44 4 The cluster number of the root directory. Often this field is set to 2.
 * 48 2 The sector number of the FSInfo structure.
 * 50 2 The sector number of the backup boot sector.
 * 52 12 Reserved. When the volume is formated these bytes should be zero.
 * 64 1 Drive number. The values here are identical to the values returned by the BIOS interrupt 0x13. 0x00 for a floppy disk and 0x80 for hard disks.
 * 65 1 Flags in Windows NT. Reserved otherwise.
 * 66 1 Signature (must be 0x28 or 0x29).
 * 67 4 VolumeID 'Serial' number. Used for tracking volumes between computers. You can ignore this if you want.
 * 71 11 Volume label string. This field is padded with spaces.
 * 82 8 System identifier string. Always "FAT32   ". The spec says never to trust the contents of this string for any use.
 * 90 420 Boot code.
 */

static void read_bpb(f32 *fs, struct bios_parameter_block *bpb) {
    char sector0[512];
    getSector(fs, sector0, 0, 1);

    bpb->bytes_per_sector = readi16(sector0, 11);;
    bpb->sectors_per_cluster = sector0[13];
    bpb->reserved_sectors = readi16(sector0, 14);
    bpb->FAT_count = sector0[16];
    bpb->dir_entries = readi16(sector0, 17);
    bpb->total_sectors = readi16(sector0, 19);
    bpb->media_descriptor_type = sector0[21];
    bpb->count_sectors_per_FAT12_16 = readi16(sector0, 22);
    bpb->count_sectors_per_track = readi16(sector0, 24);
    bpb->count_heads_or_sizes_on_media = readi16(sector0, 26);
    bpb->count_hidden_sectors = readi32(sector0, 28);
    bpb->large_sectors_on_media = readi32(sector0, 32);
    // EBR
    bpb->count_sectors_per_FAT32 = readi32(sector0, 36);
    bpb->flags = readi16(sector0, 40);
    bpb->FAT_version = readi16(sector0, 42);
    bpb->cluster_number_root_dir = readi32(sector0, 44);
    bpb->sector_number_FSInfo = readi16(sector0, 48);
    bpb->sector_number_backup_boot_sector = readi16(sector0, 50);
    // Skip 12 bytes
    bpb->drive_number = sector0[64];
    bpb->windows_flags = sector0[65];
    bpb->signature = sector0[66];
    bpb->volume_id = readi32(sector0, 67);
    memcpy(&bpb->volume_label, sector0 + 71, 11); bpb->volume_label[11] = 0;
    memcpy(&bpb->system_id, sector0 + 82, 8); bpb->system_id[8] = 0;
}

const struct bios_parameter_block *getBPB(f32 *fs) {
    return &fs->bpb;
}

static uint32_t sector_for_cluster(f32 *fs, uint32_t cluster) {
    return fs->cluster_begin_sector + ((cluster - 2) * fs->bpb.sectors_per_cluster);
}

// CLUSTER NUMBERS START AT 2 (for some reason...)
char *getCluster(f32 *fs, char *buff, uint32_t cluster_number) {
    uint32_t sector = sector_for_cluster(fs, cluster_number);
    uint32_t sector_count = fs->bpb.sectors_per_cluster;
    //char *buffer = malloc(sector_count * 512);
    getSector(fs, buff, sector, sector_count);
    return buffer;
}

uint32_t get_next_cluster_id(f32 *fs, uint32_t cluster) {
    return fs->FAT[cluster] & 0x0FFFFFFF;
}

void populate_root_dir(f32 *fs, struct directory *dir) {
    populate_dir(fs, dir, 2);
}

char *parse_long_name(char (*LFN_entries)[32], uint8_t entry_count) {
    // each entry can hold 13 characters.
    char *name = malloc(entry_count * 13);
    int i, j;
    for(i = 0; i < entry_count; i++) {
        char *entry = LFN_entries[i];
        uint8_t entry_no = (unsigned char)entry[0] & 0x0F;
        char *name_offset = name + ((entry_no - 1) * 13);

        for(j = 1; j < 10; j+=2) {
            if(entry[j] >= 32 && entry[j] <= 127) {
                *name_offset = entry[j];
            }
            else {
                *name_offset = 0;
            }
            name_offset++;
        }
        for(j = 14; j < 25; j+=2) {
            if(entry[j] >= 32 && entry[j] <= 127) {
                *name_offset = entry[j];
            }
            else {
                *name_offset = 0;
            }
            name_offset++;
        }
        for(j = 28; j < 31; j+=2) {
            if(entry[j] >= 32 && entry[j] <= 127) {
                *name_offset = entry[j];
            }
            else {
                *name_offset = 0;
            }
            name_offset++;
        }
    }
    return name;
}

void populate_dir(f32 *fs, struct directory *dir, uint32_t cluster) {
    dir->cluster = cluster;
    uint32_t dirs_per_cluster = fs->cluster_size / 32;
    uint32_t max_dirs = 0;
    dir->entries = 0;
    uint32_t entry_count = 0;

    uint8_t currLFN = 0;
    char LFN_entries[10][32];

    while(1) {
        max_dirs += dirs_per_cluster;
        dir->entries = realloc(dir->entries, max_dirs * sizeof (struct dir_entry));
        char root_cluster[fs->cluster_size];
        getCluster(fs, root_cluster, cluster);

        uint32_t i;
        for(i = 0; i < dirs_per_cluster; i++) {
            char *entry = root_cluster + (i * 32); // Shift by i 32-byte records.
            unsigned char first_byte = *entry;
            if(first_byte == 0x00 || first_byte == 0xE5) {
                // This directory entry has never been written
                // or it has been deleted.
                continue;
            }

            uint8_t attrs = entry[11];
            if(attrs == LFN) {
                // We'll add long-filename support later.
                memcpy(LFN_entries[currLFN], entry, 32);
                currLFN++;
                continue;
            }

            memcpy(dir->entries[entry_count].dir_name, entry, 11);
            dir->entries[entry_count].dir_name[11] = 0;
            dir->entries[entry_count].dir_attrs = attrs;
            uint16_t first_cluster_high = readi16(entry, 20);
            uint16_t first_cluster_low = readi16(entry, 26);
            dir->entries[entry_count].first_cluster = first_cluster_high << 16 | first_cluster_low;
            dir->entries[entry_count].file_size = readi32(entry, 28);

            if(currLFN > 0) {
                dir->entries[entry_count].name = parse_long_name(LFN_entries, currLFN);
            }
            else {
                // Trim up the short filename.
                char extension[4];
                memcpy(extension, dir->entries[entry_count].dir_name + 8, 3);
                extension[3] = 0;
                trim_spaces(extension, 3);

                dir->entries[entry_count].dir_name[8] = 0;
                trim_spaces(dir->entries[entry_count].dir_name, 8);

                if(strlen(extension) > 0) {
                    uint32_t len = strlen(dir->entries[entry_count].dir_name);
                    dir->entries[entry_count].dir_name[len++] = '.';
                    memcpy(dir->entries[entry_count].dir_name + len, extension, 4);
                }
                dir->entries[entry_count].name = dir->entries[entry_count].dir_name;
            }
            entry_count++;
            currLFN = 0;
        }

        dir->num_entries = entry_count;
        cluster = get_next_cluster_id(fs, cluster);
        if(cluster >= 0x0FFFFFF8) break;
    }
}

void free_directory(f32 *fs, struct directory *dir){
    int i;
    for(i = 0; i < dir->num_entries; i++) {
        if(dir->entries[i].name != dir->entries[i].dir_name)
            free(dir->entries[i].name);
    }
    free(dir->entries);
}

char *readFile(f32 *fs, struct dir_entry *dirent) {
    char *file = malloc(dirent->file_size);
    char *filecurrptr = file;
    uint32_t cluster = dirent->first_cluster;
    uint32_t copiedbytes = 0;
    while(1) {
        char cbytes[fs->cluster_size];
        getCluster(fs, cbytes, cluster);

        uint32_t remaining = dirent->file_size - copiedbytes;
        uint32_t to_copy = remaining > fs->cluster_size ? fs->cluster_size : remaining;

        memcpy(filecurrptr, cbytes, to_copy);

        filecurrptr += fs->cluster_size;
        copiedbytes += to_copy;

        cluster = get_next_cluster_id(fs, cluster);
        if(cluster >= 0x0FFFFFF8) break;
    }
    return file;
}

uint32_t allocateCluster(f32 *fs) {
    uint32_t i, ints_per_fat = (512 * fs->bpb.count_sectors_per_FAT32) / 4;
    for(i = 0; i < ints_per_fat; i++) {
        if(fs->FAT[i] == 0) {
            fs->FAT[i] = 0x0FFFFFFF;
            return i;
        }
    }
    return 0;
}

int writeFile(f32 *fs, struct directory *dir, char *file, char *fname, uint32_t flen) {
    uint32_t dirs_per_cluster = fs->cluster_size / 32;
    uint32_t required_clusters = flen / fs->cluster_size;
    // One for the traditional 8.3 name, one for each 13 charaters in the extended name.
    // Int division truncates, so if there's a remainder from length / 13, add another entry.
    uint32_t required_entries_long_fname = 1 + (strlen(fname) / 13) + (strlen(fname) % 13 == 0 ? 0 : 1);

    int index = -1;
    char root_cluster[fs->cluster_size];
    while(1) {
        getCluster(fs, root_cluster, cluster);
        
        int i, in_a_row = 0;
        for(i = 0; i < dirs_per_cluster; i++) {
            if(first_byte == 0x00 || first_byte == 0xE5) {
                in_a_row++;
            }
            else {
                in_a_row = 0;
            }
            
            if(in_a_row == required_entries_long_fname) {
                index = i - (in_a_row - 1);
                break;
            }
        }
        if(index >= 0) {
            // We found a spot to put our crap!
            break;
        }
        uint32_t next_cluster = fs->FAT[cluster];
        if(next_cluster >= 0x0FFFFFF8) {
            printf("We're out of space and there are no more clusters. We need to expand!\n");
            next_cluster = allocateCluster(fs);
            if(!next_cluster) {
                printf("Failed to allocate cluster. Disk full.\n");
                return 0;
            }
            fs->FAT[cluster] = next_cluster;
            cluster = next_cluster;
        }
    }
}
    
void print_directory(f32 *fs, struct directory *dir) {
    uint32_t i;
    int32_t max_name = 0;
    for(i = 0; i < dir->num_entries; i++) {
        int32_t namelen = strlen(dir->entries[i].name);
        max_name = namelen > max_name ? namelen : max_name;
    }

    for(i = 0; i < dir->num_entries; i++) {
        printf("[%d] %*s %c %8d bytes ",
               i,
               -max_name,
               dir->entries[i].name,
               dir->entries[i].dir_attrs & DIRECTORY?'D':' ',
               dir->entries[i].file_size, dir->entries[i].first_cluster);
        uint32_t cluster = dir->entries[i].first_cluster;
        uint32_t cluster_count = 1;
        while(1) {
            cluster = fs->FAT[cluster];
            if(cluster >= 0x0FFFFFF8) break;
            cluster_count++;
        }
        printf("clusters: [%u]\n", cluster_count);
    }
}

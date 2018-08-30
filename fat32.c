#include "fat32.h"
#include "ata_pio_drv.h"
#include "kheap.h"
#include "common.h"
#include "kernio.h"

f32 *master_fs;
//int x;

//struct f32 {
//    //FILE *f;
//    uint32_t *FAT;
//    struct bios_parameter_block bpb;
//    uint32_t partition_begin_sector;
//    uint32_t fat_begin_sector;
//    uint32_t cluster_begin_sector;
//    uint32_t cluster_size;
//    uint32_t cluster_alloc_hint;
//};

static void read_bpb(f32 *fs, struct bios_parameter_block *bpb);
static uint32_t sector_for_cluster(f32 *fs, uint32_t cluster);

static void trim_spaces(char *c, int max) {
    int i = 0;
    while(*c != ' ' && i++ < max) {
        c++;
    }
    if(*c == ' ') *c = 0;
}

static void getSector(f32 *fs, uint8_t *buff, uint32_t sector, uint32_t count) {
    ata_pio_read48(sector, count, buff);
}

static void putSector(f32 *fs, uint8_t *buff, uint32_t sector, uint32_t count) {
    uint32_t i;
    for(i = 0; i < count; i++) {
        ata_pio_write48(sector + i, 1, buff + (i * 512));
    }
}

static void flushFAT(f32 *fs) {
    // TODO: This is not endian-safe. Must marshal the integers into a byte buffer.
    putSector(fs, (uint8_t *)fs->FAT, fs->fat_begin_sector, fs->bpb.count_sectors_per_FAT32);
}

static uint16_t readi16(uint8_t *buff, size_t offset)
{
    uint8_t *ubuff = buff + offset;
    return ubuff[1] << 8 | ubuff[0];
}
static uint32_t readi32(uint8_t *buff, size_t offset) {
    uint8_t *ubuff = buff + offset;
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
    uint8_t sector0[512];
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

static uint32_t sector_for_cluster(f32 *fs, uint32_t cluster) {
    return fs->cluster_begin_sector + ((cluster - 2) * fs->bpb.sectors_per_cluster);
}

// CLUSTER NUMBERS START AT 2 (for some reason...)
void getCluster(f32 *fs, uint8_t *buff, uint32_t cluster_number) { // static
    if(cluster_number >= EOC) {
        PANIC("Can't get cluster. Hit End Of Chain.");
    }
    uint32_t sector = sector_for_cluster(fs, cluster_number);
    uint32_t sector_count = fs->bpb.sectors_per_cluster;
    getSector(fs, buff, sector, sector_count);
}

static void putCluster(f32 *fs, uint8_t *buff, uint32_t cluster_number) {
    uint32_t sector = sector_for_cluster(fs, cluster_number);
    uint32_t sector_count = fs->bpb.sectors_per_cluster;
    putSector(fs, buff, sector, sector_count);
}

uint32_t get_next_cluster_id(f32 *fs, uint32_t cluster) { // static
//    printf("fs->FAT: %x\n", fs->FAT);
//    printf("cluster: %x\n", cluster);
//    printf("fs->FAT[cluster]: %x\n", fs->FAT + cluster);
    return fs->FAT[cluster] & 0x0FFFFFFF;
}

static char *parse_long_name(uint8_t *entries, uint8_t entry_count) {
    // each entry can hold 13 characters.
    char *name = kmalloc(entry_count * 13);
    int i, j;
    for(i = 0; i < entry_count; i++) {
        uint8_t *entry = entries + (i * 32);
        uint8_t entry_no = (uint8_t)entry[0] & 0x0F;
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

static void clear_cluster(f32 *fs, uint32_t cluster) {
    uint8_t buffer[fs->cluster_size];
    memset(buffer, 0, fs->cluster_size);
    putCluster(fs, buffer, cluster);
}

static uint32_t allocateCluster(f32 *fs) {
    uint32_t i, ints_per_fat = (512 * fs->bpb.count_sectors_per_FAT32) / 4;
    for(i = fs->cluster_alloc_hint; i < ints_per_fat; i++) {
        if(fs->FAT[i] == 0) {
            fs->FAT[i] = 0x0FFFFFFF;
            clear_cluster(fs, i);
            fs->cluster_alloc_hint = i+1;
            return i;
        }
    }
    for(i = 0; i < fs->cluster_alloc_hint; i++) {
        if(fs->FAT[i] == 0) {
            fs->FAT[i] = 0x0FFFFFFF;
            clear_cluster(fs, i);
            fs->cluster_alloc_hint = i+1;
            return i;
        }
    }
    return 0;
}

// Creates a checksum for an 8.3 filename
// must be in directory-entry format, i.e.
// fat32.c -> "FAT32   C  "
static uint8_t checksum_fname(char *fname) {
    uint32_t i;
    uint8_t checksum = 0;
    for(i = 0; i < 11; i++) {
        uint8_t highbit = (checksum & 0x1) << 7;
        checksum = ((checksum >> 1) & 0x7F) | highbit;
        checksum = checksum + fname[i];
    }
    return checksum;
}

static void write_8_3_filename(char *fname, uint8_t *buffer) {
    memset(buffer, ' ', 11);
    uint32_t namelen = strlen(fname);
    // find the extension
    int i;
    int dot_index = -1;
    for(i = namelen-1; i >= 0; i--) {
        if(fname[i] == '.') {
            // Found it!
            dot_index = i;
            break;
        }
    }

    // Write the extension
    if(dot_index >= 0) {
        for(i = 0; i < 3; i++) {
            uint32_t c_index = dot_index + 1 + i;
            uint8_t c = c_index >= namelen ? ' ' : k_toupper(fname[c_index]);
            buffer[8 + i] = c;
        }
    }
    else {
        for(i = 0; i < 3; i++) {
            buffer[8 + i] = ' ';
        }
    }

    // Write the filename.
    uint32_t firstpart_len = namelen;
    if(dot_index >= 0) {
        firstpart_len = dot_index;
    }
    if(firstpart_len > 8) {
        // Write the weird tilde thing.
        for(i = 0; i < 6; i++) {
            buffer[i] = k_toupper(fname[i]);
        }
        buffer[6] = '~';
        buffer[7] = '1'; // probably need to enumerate like files and increment.
    }
    else {
        // Just write the file name.
        uint32_t j;
        for(j = 0; j < firstpart_len; j++) {
            buffer[j] = k_toupper(fname[j]);
        }
    }
}

static uint8_t *locate_entries(f32 *fs, uint8_t *cluster_buffer, struct directory *dir, uint32_t count, uint32_t *found_cluster) {
    uint32_t dirs_per_cluster = fs->cluster_size / 32;

    uint32_t i;
    int64_t index = -1;
    uint32_t cluster = dir->cluster;
    while(1) {
        getCluster(fs, cluster_buffer, cluster);

        uint32_t in_a_row = 0;
        for(i = 0; i < dirs_per_cluster; i++) {
            uint8_t *entry = cluster_buffer + (i * 32);
            uint8_t first_byte = entry[0];
            if(first_byte == 0x00 || first_byte == 0xE5) {
                in_a_row++;
            }
            else {
                in_a_row = 0;
            }

            if(in_a_row == count) {
                index = i - (in_a_row - 1);
                break;
            }
        }
        if(index >= 0) {
            // We found a spot to put our crap!
            break;
        }

        uint32_t next_cluster = fs->FAT[cluster];
        if(next_cluster >= EOC) {
            next_cluster = allocateCluster(fs);
            if(!next_cluster) {
                return 0;
            }
            fs->FAT[cluster] = next_cluster;
        }
        cluster = next_cluster;
    }
    *found_cluster = cluster;
    return cluster_buffer + (index * 32);
}

static void write_long_filename_entries(uint8_t *start, uint32_t num_entries, char *fname) {
    // Create a short filename to use for the checksum.
    char shortfname[12];
    shortfname[11] = 0;
    write_8_3_filename(fname, (uint8_t *)shortfname);
    uint8_t checksum = checksum_fname(shortfname);

    /* Write the long-filename entries */
    // tracks the number of characters we've written into
    // the long-filename entries.
    uint32_t writtenchars = 0;
    char *nameptr = fname;
    uint32_t namelen = strlen(fname);
    uint8_t *entry = NULL;
    uint32_t i;
    for(i = 0; i < num_entries; i++) {
        // reverse the entry order
        entry = start + ((num_entries - 1 - i) * 32);
        // Set the entry number
        entry[0] = i+1;
        entry[13] = checksum;

        // Characters are 16 bytes in long-filename entries (j+=2)
        // And they only go in certain areas in the 32-byte
        // block. (why we have three loops)
        uint32_t j;
        for(j = 1; j < 10; j+=2) {
            if(writtenchars < namelen) {
                entry[j] = *nameptr;
            }
            else {
                entry[j] = 0;
            }
            nameptr++;
            writtenchars++;
        }
        for(j = 14; j < 25; j+=2) {
            if(writtenchars < namelen) {
                entry[j] = *nameptr;
            }
            else {
                entry[j] = 0;
            }
            nameptr++;
            writtenchars++;
        }
        for(j = 28; j < 31; j+=2) {
            if(writtenchars < namelen) {
                entry[j] = *nameptr;
            }
            else {
                entry[j] = 0;
            }
            nameptr++;
            writtenchars++;
        }
        // Mark the attributes byte as LFN (Long File Name)
        entry[11] = LFN;
    }
    // Mark the last(first) entry with the end-of-long-filename bit
    entry[0] |= 0x40;
}

f32 *makeFilesystem(char *fatSystem) {
    f32 *fs = kmalloc(sizeof (struct f32));
    if(!identify()) {
        return NULL;
    }
    printf("Filesystem identified!\n");
    read_bpb(fs, &fs->bpb);

    trim_spaces(fs->bpb.system_id, 8);
    if(strcmp(fs->bpb.system_id, "FAT32") != 0) {
        kfree(fs);
        return NULL;
    }

    printf("Sectors per cluster: %d\n", fs->bpb.sectors_per_cluster);

    fs->partition_begin_sector = 0;
    fs->fat_begin_sector = fs->partition_begin_sector + fs->bpb.reserved_sectors;
    fs->cluster_begin_sector = fs->fat_begin_sector + (fs->bpb.FAT_count * fs->bpb.count_sectors_per_FAT32);
    fs->cluster_size = 512 * fs->bpb.sectors_per_cluster;
    fs->cluster_alloc_hint = 0;

    // Load the FAT
    uint32_t bytes_per_fat = 512 * fs->bpb.count_sectors_per_FAT32;
    fs->FAT = kmalloc(bytes_per_fat);
    uint32_t sector_i;
    for(sector_i = 0; sector_i < fs->bpb.count_sectors_per_FAT32; sector_i++) {
        uint8_t sector[512];
        getSector(fs, sector, fs->fat_begin_sector + sector_i, 1);
        uint32_t integer_j;
        for(integer_j = 0; integer_j < 512/4; integer_j++) {
            fs->FAT[sector_i * (512 / 4) + integer_j]
                = readi32(sector, integer_j * 4);
        }
    }
    return fs;
}

void destroyFilesystem(f32 *fs) {
    printf("Destroying filesystem.\n");
    flushFAT(fs);
    kfree(fs->FAT);
    kfree(fs);
}

const struct bios_parameter_block *getBPB(f32 *fs) {
    return &fs->bpb;
}

void populate_root_dir(f32 *fs, struct directory *dir) {
    populate_dir(fs, dir, 2);
}

// Populates dirent with the directory entry starting at start
// Returns a pointer to the next 32-byte chunk after the entry
// or NULL if either start does not point to a valid entry, or
// there are not enough entries to build a struct dir_entry
static uint8_t *read_dir_entry(f32 *fs, uint8_t *start, uint8_t *end, struct dir_entry *dirent) {
    uint8_t first_byte = start[0];
    uint8_t *entry = start;
    if(first_byte == 0x00 || first_byte == 0xE5) {
        // NOT A VALID ENTRY!
        return NULL;
    }

    uint32_t LFNCount = 0;
    while(entry[11] == LFN) {
        LFNCount++;
        entry += 32;
        if(entry == end) {
            return NULL;
        }
    }
    if(LFNCount > 0) {
        dirent->name = parse_long_name(start, LFNCount);
    }
    else {
        // There's no long file name.
        // Trim up the short filename.
        dirent->name = kmalloc(13);
        memcpy(dirent->name, entry, 11);
        dirent->name[11] = 0;
        char extension[4];
        memcpy(extension, dirent->name + 8, 3);
        extension[3] = 0;
        trim_spaces(extension, 3);

        dirent->name[8] = 0;
        trim_spaces(dirent->name, 8);

        if(strlen(extension) > 0) {
            uint32_t len = strlen(dirent->name);
            dirent->name[len++] = '.';
            memcpy(dirent->name + len, extension, 4);
        }
    }

    dirent->dir_attrs = entry[11];;
    uint16_t first_cluster_high = readi16(entry, 20);
    uint16_t first_cluster_low = readi16(entry, 26);
    dirent->first_cluster = first_cluster_high << 16 | first_cluster_low;
    dirent->file_size = readi32(entry, 28);
    return entry + 32;
}

// This is a complicated one. It parses a directory entry into the dir_entry pointed to by target_dirent.
// root_cluster  must point to a buffer big enough for two clusters.
// entry         points to the entry the caller wants to parse, and must point to a spot within root_cluster.
// nextentry     will be modified to hold the next spot within root_entry to begin looking for entries.
// cluster       is the cluster number of the cluster loaded into root_cluster.
// secondcluster will be modified IF this function needs to load another cluster to continue parsing
//               the entry, in which case, it will be set to the value of the cluster loaded.
//
void next_dir_entry(f32 *fs, uint8_t *root_cluster, uint8_t *entry, uint8_t **nextentry, struct dir_entry *target_dirent, uint32_t cluster, uint32_t *secondcluster) {

    uint8_t *end_of_cluster = root_cluster + fs->cluster_size;
    *nextentry = read_dir_entry(fs, entry, end_of_cluster, target_dirent);
    if(!*nextentry) {
        // Something went wrong!
        // Either the directory entry spans the bounds of a cluster,
        // or the directory entry is invalid.
        // Load the next cluster and retry.

        // Figure out how much of the last cluster to "replay"
        uint32_t bytes_from_prev_chunk = end_of_cluster - entry;

        *secondcluster = get_next_cluster_id(fs, cluster);
        if(*secondcluster >= EOC) {
            // There's not another directory cluster to load
            // and the previous entry was invalid!
            // It's possible the filesystem is corrupt or... you know...
            // my software could have bugs.
            PANIC("FOUND BAD DIRECTORY ENTRY!");
        }
        // Load the cluster after the previous saved entries.
        getCluster(fs, root_cluster + fs->cluster_size, *secondcluster);
        // Set entry to its new location at the beginning of root_cluster.
        entry = root_cluster + fs->cluster_size - bytes_from_prev_chunk;

        // Retry reading the entry.
        *nextentry = read_dir_entry(fs, entry, end_of_cluster + fs->cluster_size, target_dirent);
        if(!*nextentry) {
            // Still can't parse the directory entry.
            // Something is very wrong.
            PANIC("FAILED TO READ DIRECTORY ENTRY! THE SOFTWARE IS BUGGY!\n");
        }
    }
}

// TODO: Refactor this. It is so similar to delFile that it would be nice
// to combine the similar elements.
// WARN: If you fix a bug in this function, it's likely you will find the same
// bug in delFile.
void populate_dir(f32 *fs, struct directory *dir, uint32_t cluster) {
    dir->cluster = cluster;
    uint32_t dirs_per_cluster = fs->cluster_size / 32;
    uint32_t max_dirs = 0;
    dir->entries = 0;
    uint32_t entry_count = 0;

    while(1) {
        max_dirs += dirs_per_cluster;
        dir->entries = krealloc(dir->entries, max_dirs * sizeof (struct dir_entry));
        // Double the size in case we need to read a directory entry that
        // spans the bounds of a cluster.
        uint8_t root_cluster[fs->cluster_size * 2];
        getCluster(fs, root_cluster, cluster);

        uint8_t *entry = root_cluster;
        while((uint32_t)(entry - root_cluster) < fs->cluster_size) {
            uint8_t first_byte = *entry;
            if(first_byte == 0x00 || first_byte == 0xE5) {
                // This directory entry has never been written
                // or it has been deleted.
                entry += 32;
                continue;
            }

            uint32_t secondcluster = 0;
            uint8_t *nextentry = NULL;
            struct dir_entry *target_dirent = dir->entries + entry_count;
            next_dir_entry(fs, root_cluster, entry, &nextentry, target_dirent, cluster, &secondcluster);
            entry = nextentry;
            if(secondcluster) {
                cluster = secondcluster;
            }

            entry_count++;
        }
        cluster = get_next_cluster_id(fs, cluster);
        if(cluster >= EOC) break;
    }
    dir->num_entries = entry_count;
}

static void zero_FAT_chain(f32 *fs, uint32_t start_cluster) {
    uint32_t cluster = start_cluster;
    while(cluster < EOC && cluster != 0) {
        uint32_t next_cluster = fs->FAT[cluster];
        fs->FAT[cluster] = 0;
        cluster = next_cluster;
    }
    flushFAT(fs);
}

// TODO: Refactor this. It is so similar to populate_dir that it would be nice
// to combine the similar elements.
// WARN: If you fix a bug in this function, it's likely you will find the same
// bug in populate_dir.
void delFile(f32 *fs, struct directory *dir, char *filename) { //struct dir_entry *dirent) {
    uint32_t cluster = dir->cluster;

    // Double the size in case we need to read a directory entry that
    // spans the bounds of a cluster.
    uint8_t root_cluster[fs->cluster_size * 2];
    struct dir_entry target_dirent;

    // Try to locate and invalidate the directory entries corresponding to the
    // filename in dirent.
    while(1) {
        getCluster(fs, root_cluster, cluster);

        uint8_t *entry = root_cluster;
        while((uint32_t)(entry - root_cluster) < fs->cluster_size) {
            uint8_t first_byte = *entry;
            if(first_byte == 0x00 || first_byte == 0xE5) {
                // This directory entry has never been written
                // or it has been deleted.
                entry += 32;
                continue;
            }

            uint32_t secondcluster = 0;
            uint8_t *nextentry = NULL;
            next_dir_entry(fs, root_cluster, entry, &nextentry, &target_dirent, cluster, &secondcluster);

            // We have a target dirent! see if it's the one we want!
            if(strcmp(target_dirent.name, filename) == 0) {
                // We found it! Invalidate all the entries.
                memset(entry, 0, nextentry - entry);
                putCluster(fs, root_cluster, cluster);
                if(secondcluster) {
                    putCluster(fs, root_cluster + fs->cluster_size, secondcluster);
                }
                zero_FAT_chain(fs, target_dirent.first_cluster);
                kfree(target_dirent.name);
                return;
            }
            else {
                // We didn't find it. Continue.
                entry = nextentry;
                if(secondcluster) {
                    cluster = secondcluster;
                }
            }
            kfree(target_dirent.name);

        }
        cluster = get_next_cluster_id(fs, cluster);
        if(cluster >= EOC) return;
    }
}

void free_directory(f32 *fs, struct directory *dir){
    uint32_t i;
    for(i = 0; i < dir->num_entries; i++) {
        kfree(dir->entries[i].name);
    }
    kfree(dir->entries);
}

uint8_t *readFile(f32 *fs, struct dir_entry *dirent) {
    uint8_t *file = kmalloc(dirent->file_size);
    uint8_t *filecurrptr = file;
    uint32_t cluster = dirent->first_cluster;
    uint32_t copiedbytes = 0;
    while(1) {
        uint8_t cbytes[fs->cluster_size];
        printf("Cluster: %d\n", cluster);
        getCluster(fs, cbytes, cluster);

        uint32_t remaining = dirent->file_size - copiedbytes;
        uint32_t to_copy = remaining > fs->cluster_size ? fs->cluster_size : remaining;

        memcpy(filecurrptr, cbytes, to_copy);

        filecurrptr += fs->cluster_size;
        copiedbytes += to_copy;

        cluster = get_next_cluster_id(fs, cluster);
        if(cluster >= EOC) break;
    }
    return file;
}

static void writeFile_impl(f32 *fs, struct directory *dir, uint8_t *file, char *fname, uint32_t flen, uint8_t attrs, uint32_t setcluster) {
    uint32_t required_clusters = flen / fs->cluster_size;
    if(flen % fs->cluster_size != 0) required_clusters++;
    if(required_clusters == 0) required_clusters++; // Allocate at least one cluster.
    // One for the traditional 8.3 name, one for each 13 charaters in the extended name.
    // Int division truncates, so if there's a remainder from length / 13, add another entry.
    uint32_t required_entries_long_fname = (strlen(fname) / 13);
    if(strlen(fname) % 13 > 0) {
        required_entries_long_fname++;
    }

    uint32_t required_entries_total = required_entries_long_fname + 1;

    uint32_t cluster; // The cluster number that the entries are found in
    uint8_t root_cluster[fs->cluster_size];
    uint8_t *start_entries = locate_entries(fs, root_cluster, dir, required_entries_total, &cluster);
    write_long_filename_entries(start_entries, required_entries_long_fname, fname);

    // Write the actual file entry;
    uint8_t *actual_entry = start_entries + (required_entries_long_fname * 32);
    write_8_3_filename(fname, actual_entry);

    // Actually write the file!
    uint32_t writtenbytes = 0;
    uint32_t prevcluster = 0;
    uint32_t firstcluster = 0;
    uint32_t i;
    if(setcluster) {
        // Caller knows where the first cluster is.
        // Don't allocate or write anything.
        firstcluster = setcluster;
    }
    else {
        for(i = 0; i < required_clusters; i++) {
            uint32_t currcluster = allocateCluster(fs);
            if(!firstcluster) {
                firstcluster = currcluster;
            }
            uint8_t cluster_buffer[fs->cluster_size];
            memset(cluster_buffer, 0, fs->cluster_size);
            uint32_t bytes_to_write = flen - writtenbytes;
            if(bytes_to_write > fs->cluster_size) {
                bytes_to_write = fs->cluster_size;
            }
            memcpy(cluster_buffer, file + writtenbytes, bytes_to_write);
            writtenbytes += bytes_to_write;
            putCluster(fs, cluster_buffer, currcluster);
            if(prevcluster) {
                fs->FAT[prevcluster] = currcluster;
            }
            prevcluster = currcluster;
        }
    }

    // Write the other fields of the actual entry
    // We do it down here because we need the first cluster
    // number.

    // attrs
    actual_entry[11] = attrs;

    // high cluster bits
    actual_entry[20] = (firstcluster >> 16) & 0xFF;
    actual_entry[21] = (firstcluster >> 24) & 0xFF;

    // low cluster bits
    actual_entry[26] = (firstcluster) & 0xFF;
    actual_entry[27] = (firstcluster >> 8) & 0xFF;

    // file size
    actual_entry[28] = flen & 0xFF;
    actual_entry[29] = (flen >> 8) & 0xFF;
    actual_entry[30] = (flen >> 16) & 0xFF;
    actual_entry[31] = (flen >> 24) & 0xFF;

    // Write the cluster back to disk
    putCluster(fs, root_cluster, cluster);
    flushFAT(fs);
}

void writeFile(f32 *fs, struct directory *dir, uint8_t *file, char *fname, uint32_t flen) {
    writeFile_impl(fs, dir, file, fname, flen, 0, 0);
}

static void mkdir_subdirs(f32 *fs, struct directory *dir, uint32_t parentcluster) {
    writeFile_impl(fs, dir, NULL, ".", 0, DIRECTORY, dir->cluster);
    writeFile_impl(fs, dir, NULL, "..", 0, DIRECTORY, parentcluster);
}

void mkdir(f32 *fs, struct directory *dir, char *dirname) {
    writeFile_impl(fs, dir, NULL, dirname, 0, DIRECTORY, 0);

    // We need to add the subdirectories '.' and '..'
    struct directory subdir;
    populate_dir(fs, &subdir, dir->cluster);
    uint32_t i;
    for(i = 0; i < subdir.num_entries; i++) {
        if(strcmp(subdir.entries[i].name, dirname) == 0) {
            struct directory newsubdir;
            populate_dir(fs, &newsubdir, subdir.entries[i].first_cluster);
            mkdir_subdirs(fs, &newsubdir, subdir.cluster);
            free_directory(fs, &newsubdir);
        }
    }
    free_directory(fs, &subdir);
}

void print_directory(f32 *fs, struct directory *dir) {
    uint32_t i;
    uint32_t max_name = 0;
    for(i = 0; i < dir->num_entries; i++) {
        uint32_t namelen = strlen(dir->entries[i].name);
        max_name = namelen > max_name ? namelen : max_name;
    }

    char *namebuff = kmalloc(max_name + 1);
    for(i = 0; i < dir->num_entries; i++) {
//        printf("[%d] %*s %c %8d bytes ",
//               i,
//               -max_name,
//               dir->entries[i].name,
//               dir->entries[i].dir_attrs & DIRECTORY?'D':' ',
//               dir->entries[i].file_size, dir->entries[i].first_cluster);
        printf("[%d] ", i);


        uint32_t j;
        for(j = 0; j < max_name; j++) {
            namebuff[j] = ' ';
        }
        namebuff[max_name] = 0;
        for(j = 0; j < strlen(dir->entries[i].name); j++) {
            namebuff[j] = dir->entries[i].name[j];
        }

        printf("%s %c %d ",
               namebuff,
               dir->entries[i].dir_attrs & DIRECTORY?'D':' ',
               dir->entries[i].file_size);

        uint32_t cluster = dir->entries[i].first_cluster;
        uint32_t cluster_count = 1;
        while(1) {
            cluster = fs->FAT[cluster];
            if(cluster >= EOC) break;
            if(cluster == 0) {
                PANIC("BAD CLUSTER CHAIN! FS IS CORRUPT!");
            }
            cluster_count++;
        }
        printf("clusters: [%d]\n", cluster_count);
    }
    kfree(namebuff);
}

uint32_t count_free_clusters(f32 *fs) {
    uint32_t clusters_in_fat = (fs->bpb.count_sectors_per_FAT32 * 512) / 4;
    uint32_t i;
    uint32_t count = 0;
    for(i = 0; i < clusters_in_fat; i++) {
        if(fs->FAT[i] == 0) {
            count++;
        }
    }
    return count;
}

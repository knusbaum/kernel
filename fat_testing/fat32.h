
struct bios_parameter_block {
    uint16_t bytes_per_sector;          // IMPORTANT
    uint8_t sectors_per_cluster;        // IMPORTANT
    uint16_t reserved_sectors;          // IMPORTANT 
    uint8_t FAT_count;                  // IMPORTANT
    uint16_t dir_entries;
    uint16_t total_sectors;
    uint8_t media_descriptor_type;
    uint16_t count_sectors_per_FAT12_16; // FAT12/FAT16 only.
    uint16_t count_sectors_per_track;
    uint16_t count_heads_or_sizes_on_media;
    uint32_t count_hidden_sectors;
    uint32_t large_sectors_on_media;  // This is set instead of total_sectors if it's > 65535

    // Extended Boot Record
    uint32_t count_sectors_per_FAT32;   // IMPORTANT
    uint16_t flags;
    uint16_t FAT_version;
    uint32_t cluster_number_root_dir;   // IMPORTANT
    uint16_t sector_number_FSInfo;
    uint16_t sector_number_backup_boot_sector;
    uint8_t drive_number;
    uint8_t windows_flags;
    uint8_t signature;                  // IMPORTANT
    uint32_t volume_id;
    char volume_label[12];
    char system_id[9];
};

#define READONLY  1
#define HIDDEN    (1 << 1)
#define SYSTEM    (1 << 2)
#define VolumeID  (1 << 3)
#define DIRECTORY (1 << 4)
#define ARCHIVE   (1 << 5)
#define LFN (READONLY | HIDDEN | SYSTEM | VolumeID)

struct dir_entry {
    char *name;
    uint8_t dir_attrs;
    uint32_t first_cluster;
    uint32_t file_size;
};

struct directory {
    uint32_t cluster;
    struct dir_entry *entries;
    uint32_t num_entries;
};

typedef struct f32 f32;

f32 *makeFilesystem(char *fatSystem);
void destroyFilesystem(f32 *fs);

const struct bios_parameter_block *getBPB(f32 *fs);

void getSector(f32 *fs, char *buff, uint32_t sector, uint32_t count);
void getCluster(f32 *fs, char *buff, uint32_t cluster_number);

void putSector(f32 *fs, char *buff, uint32_t sector, uint32_t count);
void putCluster(f32 *fs, char *buff, uint32_t cluster_number);

uint16_t readi16(char *buff, size_t offset);
uint32_t readi32(char *buff, size_t offset);

void populate_root_dir(f32 *fs, struct directory *dir);
void populate_dir(f32 *fs, struct directory *dir, uint32_t cluster);
void free_directory(f32 *fs, struct directory *dir);

char *readFile(f32 *fs, struct dir_entry *dirent);
int writeFile(f32 *fs, struct directory *dir, char *file, char *fname, uint32_t flen);

void print_directory(f32 *fs, struct directory *dir);

void write_8_3_filename(char *fname, char *buffer);


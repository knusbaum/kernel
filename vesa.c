#include "common.h"
#include "realmode.h"
#include "font.h"
#include "terminal.h"


void vesa_putchar(char c);
void vesa_set_status(char *status);
void vesa_set_cursor(uint8_t x, uint8_t y);
uint32_t make_vesa_color(uint8_t r, uint8_t g, uint8_t b);

struct VbeInfoBlock {
   char VbeSignature[4];             // == "VESA"
   uint16_t VbeVersion;                 // == 0x0300 for VBE 3.0
   uint16_t OemStringPtr[2];            // isa vbeFarPtr
   uint8_t Capabilities[4];
   uint16_t VideoModePtr[2];         // isa vbeFarPtr
   uint16_t TotalMemory;             // as # of 64KB blocks
} __attribute__((packed));

struct ModeInfoBlock {
  uint16_t attributes;
  uint8_t winA,winB;
  uint16_t granularity;
  uint16_t winsize;
  uint16_t segmentA, segmentB;
  uint16_t realFctPtr[2];
  uint16_t pitch; // bytes per scanline

  uint16_t Xres, Yres;
  uint8_t Wchar, Ychar, planes, bpp, banks;
  uint8_t memory_model, bank_size, image_pages;
  uint8_t reserved0;

  uint8_t red_mask, red_position;
  uint8_t green_mask, green_position;
  uint8_t blue_mask, blue_position;
  uint8_t rsv_mask, rsv_position;
  uint8_t directcolor_attributes;

  uint32_t physbase;  // your LFB (Linear Framebuffer) address ;)
  uint32_t reserved1;
  uint16_t reserved2;
} __attribute__((packed));

#define LNG_PTR(seg, off) ((seg << 4) | off)
#define REAL_PTR(arr) LNG_PTR(arr[1], arr[0])
#define SEG(addr) (((uint32_t)addr >> 4) & 0xF000)
#define OFF(addr) ((uint32_t)addr & 0xFFFF)

struct VbeInfoBlock vib;
// VIB extends beyond size of struct. 512 is somewhat arbitrary.
struct ModeInfoBlock mib; // = 0x80000 + sizeof (struct VbeInfoBlock) + 512;
uint32_t *framebuffer = 0;

#define CHAR_ROW 16
#define CHAR_COLUMN 8
#define CHARLEN (CHAR_ROW * CHAR_COLUMN) // 8 columns * 16 rows
#define CHARCOUNT 94 // 94 printable characters

#define CHAROFF(c) ((c - 32) * CHARLEN)

uint32_t chars[CHARCOUNT * CHARLEN];

void populate_chars(uint32_t vesa_color) {
    for(unsigned char c = ' '; c < '~'; c++) {
        unsigned short offset = (c - 31) * 16 ;

        for(int row = 0; row < CHAR_ROW; row++) {
            uint8_t mask = 1 << 7;
            uint32_t *abs_row = chars + CHAROFF(c) + (row * 8);
            for(int i = 0; i < 8; i++) {
                if(font.Bitmap[offset + row] & mask) {
                    abs_row[i] = vesa_color; //0xFFFFFFFF;
                }
                else {
                    abs_row[i] = 0;
                }
                mask = (mask >> 1);
            }
        }
    }
}

void populate_vib() {
    struct VbeInfoBlock *loc_vib = 0x80000; // Safe low-memory

    regs16_t regs;
    regs.ax=0x4f00;
    regs.es=0x8000;
    regs.di=0x0;
    int32(0x10, &regs);
    vib = *loc_vib;
}

void set_vmode() {
    populate_vib();
    regs16_t regs;

    struct ModeInfoBlock *loc_mib = 0x80000 + sizeof (struct VbeInfoBlock) + 512;

    uint16_t *modes = (uint16_t *)REAL_PTR(vib.VideoModePtr);

    int i;
    for(i = 0; modes[i] != 0xFFFF; i++) {
        regs.ax = 0x4f01;
        regs.cx = modes[i];
        regs.es = SEG(loc_mib);
        regs.di = OFF(loc_mib);
        int32(0x10, &regs);

        // Currently, we only want 1280x720x32
        //1280 720 32
        if(loc_mib->Xres == 1280 && loc_mib->Yres == 720 && loc_mib->bpp == 32) {
            break;
        }
    }
    framebuffer = (uint32_t *)loc_mib->physbase;
//    printf("red_mask: %d\n", mib->red_mask);
//    printf("red_position: %d\n", mib->red_position);
//    printf("green_mask: %d\n", mib->green_mask);
//    printf("green_position: %d\n", mib->green_position);
//    printf("blue_mask: %d\n", mib->blue_mask);
//    printf("blue_position: %d\n", mib->blue_position);
//    printf("Bytes per scanline: %d (expecting %d)\n", mib->pitch, 1280 * 4);

    uint16_t mode = modes[i];
    regs.ax = 0x4F02;
    regs.bx = mode | 0x4000; // Set Linear Frame Buffer (LFB) bit.
    int32(0x10, &regs);

    terminal_putchar = vesa_putchar;
    terminal_set_status = vesa_set_status;
    terminal_set_cursor = vesa_set_cursor;
    mib = *loc_mib;
    populate_chars(make_vesa_color(255, 255, 255));
}

uint32_t make_vesa_color(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t red = ((uint32_t)r) << mib.red_position;
    uint32_t green = ((uint32_t)g) << mib.green_position;
    uint32_t blue = ((uint32_t)b) << mib.blue_position;
    return red | green | blue;
}

void set_vesa_color(uint8_t r, uint8_t g, uint8_t b) {
    populate_chars(make_vesa_color(r, g, b));
}

void draw_pixel_at(int x, int y, uint32_t color) {
    uint32_t *row = ((unsigned char *)framebuffer) + (y * mib.pitch);
    row[x] = color;
}

static inline void draw_character_at(int x, int y, int c, uint32_t fg, uint32_t bg) {
    uint32_t step = mib.pitch/4;
    uint32_t *chardat = chars + CHAROFF(c);
    uint32_t *abs_row = ((unsigned char *)framebuffer) + (y * mib.pitch);
    abs_row += x;
    for(int row = 0; row < CHAR_ROW *8; row+=8) {
        fastcp(abs_row, chardat + row, 32);
        abs_row += step;
    }
}

void shift_up() {
    fastcp(((char *)framebuffer) + ((1280 * 16) * 4), ((char *)framebuffer) + ((1280 * 16) * 8), (1280 * 704) * 4 - ((1280 * 16) * 4));
    memset(((char *)framebuffer) + (1280 * 704 * 4), 0, 1280 * 16 * 4);
}

int currx, curry;

void vesa_newline() {
    currx = 0;
    if(curry >= 704) {
        curry = 704;
        shift_up();
    }
    else {
        curry += 16;
    }
}

void vesa_set_cursor(uint8_t x, uint8_t y) {
    currx = x * 8;
    curry = y * 16;
}

void vesa_putchar(char c) {
    if(currx + 8 >= mib.Xres) {
        vesa_newline();
    }
    switch (c) {
    case '\n':
        vesa_newline();
        break;
    case '\t':
        vesa_putchar(' ');
        vesa_putchar(' ');
        vesa_putchar(' ');
        vesa_putchar(' ');
        break;
    case BS:
        if(currx > 0) {
            currx -= 8;
            vesa_putchar(' ');
            currx -= 8;
        }
        break;
    default:
        draw_character_at(currx, curry, c, 0xFFFFFFFF, 0);
        currx += 8;
        break;
    }

}

uint32_t get_framebuffer_addr() {
    return framebuffer;
}

uint32_t get_framebuffer_length() {
    return 1280 * 720 * 4;
}

void vesa_set_status(char *status) {
    int currx = 0;
    while(*status) {
        draw_character_at(currx, 0, *status++, 0xFFFFFFFF, 0);
        currx += 8;
    }
    while(currx + 8 < mib.Xres) {
        draw_character_at(currx, 0, ' ', 0xFFFFFFFF, 0);
        currx += 8;
    }
}

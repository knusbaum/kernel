#include "common.h"
#include "realmode.h"

// int32 test
void int32_test()
{
    int y;
    regs16_t regs;
     
    // switch to 320x200x256 graphics mode
    regs.ax = 0x0013;
    int32(0x10, &regs);
     
    // full screen with blue color (1)
    memset((char *)0xA0000, 1, (320*200));
     
    // draw horizontal line from 100,80 to 100,240 in multiple colors
    for(y = 0; y < 200; y++)
        memset((char *)0xA0000 + (y*320+80), y, 160);
     
    // wait for key
    regs.ax = 0x0000;
    int32(0x16, &regs);
     
    // switch to 80x25x16 text mode
    regs.ax = 0x0003;
    int32(0x10, &regs);
}

void set_320x200x256() {
    // switch to 320x200x256 graphics mode
    regs16_t regs;
    regs.ax = 0x0013;
    int32(0x10, &regs);
}

//static inline void draw_pixel_at(int x, int y, char color) {
//    memset((char *)0xA0000 + (y*320) + x, color, 1);
//}

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
  //VBE_FAR(realFctPtr);
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

struct VbeInfoBlock *vib = 0x80000; // Safe low-memory
// VIB extends beyond size of struct. 512 is somewhat arbitrary.
struct ModeInfoBlock *mib = 0x80000 + sizeof (struct VbeInfoBlock) + 512;
uint32_t *framebuffer;

void populate_vib() {
    regs16_t regs;
    regs.ax=0x4f00;
    regs.es=0x8000;
    regs.di=0x0;
    int32(0x10, &regs);
}

void set_vmode() {
    populate_vib();
    regs16_t regs;


    uint16_t *modes = (uint16_t *)REAL_PTR(vib->VideoModePtr);

    int i;
    for(i = 0; modes[i] != 0xFFFF; i++) {
        regs.ax = 0x4f01;
        regs.cx = modes[i];
        regs.es = SEG(mib);
        regs.di = OFF(mib);
        int32(0x10, &regs);

        // Currently, we only want 1280x720x32
        //1280 720 32
        if(mib->Xres == 1280 && mib->Yres == 720 && mib->bpp == 32) {
            printf("Vesa mode %d: %x (%dx%dx%d)\n", i, modes[i], mib->Xres, mib->Yres, mib->bpp);
            break;
        }
    }
    framebuffer = (uint32_t *)mib->physbase;
//    printf("red_mask: %d\n", mib->red_mask);
//    printf("red_position: %d\n", mib->red_position);
//    printf("green_mask: %d\n", mib->green_mask);
//    printf("green_position: %d\n", mib->green_position);
//    printf("blue_mask: %d\n", mib->blue_mask);
//    printf("blue_position: %d\n", mib->blue_position);
//    printf("Bytes per scanline: %d (expecting %d)\n", mib->pitch, 1280 * 4);

//    uint32_t red = 0xFF << mib->red_position;
//    uint32_t green = 0xFF << mib->green_position;
//    uint32_t blue = 0xFF << mib->blue_position;
//    
    uint16_t mode = modes[i];
    regs.ax = 0x4F02;
    regs.bx = mode | 0x4000; // Set Linear Frame Buffer (LFB) bit.
    int32(0x10, &regs);
//    
//    for(int r = 0; r < 1280 * 100; r++) {
//        int g = r + (1280 * 100);
//        int b = g + (1280 * 100);
//        framebuffer[r] = red;
//        framebuffer[g] = green;
//        framebuffer[b] = blue;
//    }
}

uint32_t make_vesa_color(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t red = r << mib->red_position;
    uint32_t green = g << mib->green_position;
    uint32_t blue = b << mib->blue_position;
    return red | green | blue;
}

void draw_pixel_at(int x, int y, uint32_t color) {
    uint32_t *row = ((unsigned char *)framebuffer) + (y * mib->pitch);
    row[x] = color;
}

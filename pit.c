#include "pit.h"
#include "isr.h"
#include "port.h"
#include "kernio.h"
#include "vesa.h"

#define PIT_NATURAL_FREQ 1193180

#define PIT_DATA0 0x40
#define PIT_DATA1 0x41
#define PIT_DATA2 0x42
#define PIT_COMMAND 0x43

#define DRAW_MODULO 10
uint8_t draw_modcount = 0;

uint8_t normal_color;

long long tick = 0;

extern uint32_t allocated_frames;
extern uint32_t total_frames;
extern uint32_t heap_free;
extern uint32_t allocations;

#define BUFFSIZE 77

static void timer_callback(registers_t regs)
{
    draw_modcount = (draw_modcount + 1) % DRAW_MODULO;

    if(draw_modcount == 0)
    {
        tick++;

        uint32_t used = (allocated_frames * 0x1000) / 1024 / 1024;
        uint32_t available = (total_frames * 0x1000) / 1024 / 1024;

        char buffer[BUFFSIZE];
        sprintf(buffer, "Frames: %d Mem: %d/%d MiB Heap-Free: %d Allocs: %d",
               allocated_frames,
               used, available,
               heap_free,
               allocations);
        uint32_t fg = get_vesa_color();
        uint32_t bg = get_vesa_background();
        set_vesa_color(make_vesa_color(0, 0, 0));
        set_vesa_background(make_vesa_color(0xFF, 0xFF, 0xFF));
        set_status(buffer);
        set_vesa_color(fg);
        set_vesa_background(bg);
    }
}

void init_timer(uint32_t frequency)
{
    printf("Initializing PIT timer\n");
    register_interrupt_handler(IRQ0, &timer_callback);

    uint32_t divisor;
    if(frequency)
        divisor = PIT_NATURAL_FREQ / frequency;
    else
        divisor = 0;
    /*
      http://wiki.osdev.org/Programmable_Interval_Timer#I.2FO_Ports

      Channel  Access Mode        Operating Mode    BCD/Binary
      0x36 == 0011 0110 == 00       11 (lobyte/hibyte) 011 (square Wave) 0 (16-bit bin

      )
    */
    outb(PIT_COMMAND, 0x36);

    //Chop freq up into bytes and send to data0 port
    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    outb(PIT_DATA0, low);
    outb(PIT_DATA0, high);
}

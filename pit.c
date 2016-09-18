#include "pit.h"
#include "isr.h"
#include "port.h"
#include "terminal.h"

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

#define BUFFSIZE 59

static void timer_callback(registers_t regs)
{
    draw_modcount = (draw_modcount + 1) % DRAW_MODULO;

    if(draw_modcount == 0)
    {
        tick++;

        // This whole thing below is sort of ugly and hacky
        // but I guess it works for now.

        // 1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
        // F  r  a  m  e  s  :     1  2  3  4  5  6  7  8  9  10

        // 20 21 22 23 24 25 26 27 28 29 30 31 32 33
        // M  e  m  :  x  x  x  x  /  x  x  x  x

        // 34 35 36 37
        // M  i  B

        // 38 39 40 41 42 43 44 45 46 47 48
        // H  e  a  p  -  F  r  e  e  :

        // 49 50 51 52 53 54 55 56 57 58 59
        // 1  2  3  4  5  6  7  8  9  10 \0

        char statusbuffer[BUFFSIZE];
        int i = 0;
        for(; i < BUFFSIZE; i++) {
            statusbuffer[i] = ' ';
        }

        char *ticks = "Frames: ";
        i = 0;
        while(*ticks != 0) {
            statusbuffer[i++] = *ticks;
            ticks++;
        }
        itos(allocated_frames, statusbuffer + 8, 11);
        statusbuffer[18] = ' ';

        char *mem = "Mem: ";
        i = 19;
        while(*mem != 0) {
            statusbuffer[i++] = *mem;
            mem++;
        }

        uint32_t used = (allocated_frames * 0x1000) / 1024 / 1024;
        uint32_t available = (total_frames * 0x1000) / 1024 / 1024;

        if(available - used < 512) {
            terminal_set_status_color(make_color(COLOR_BLACK, COLOR_RED));
        }
        else {
            terminal_set_status_color(normal_color);
        }

        itos(used, statusbuffer + 23, 5);
        statusbuffer[27] = '/';
        itos(available, statusbuffer + 28, 5);
        statusbuffer[32] = ' ';

        char *mib = "MiB";
        i = 33;
        while(*mib != 0) {
            statusbuffer[i++] = *mib;
            mib++;
        }

        statusbuffer[36] = ' ';

        char *heapfree = "Heap-Free:";
        i = 37;
        while(*heapfree != 0) {
            statusbuffer[i++] = *heapfree;
            heapfree++;
        }
        statusbuffer[47] = ' ';
        itos(heap_free, statusbuffer + 48, 11);

        terminal_set_status(statusbuffer);
    }
}

void init_timer(uint32_t frequency)
{
    terminal_writestring("Initializing PIT timer\n");
    register_interrupt_handler(IRQ0, &timer_callback);

    normal_color = make_color(COLOR_WHITE, COLOR_BLACK);

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

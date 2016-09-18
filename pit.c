#include "pit.h"
#include "isr.h"
#include "port.h"
#include "terminal.h"

#define PIT_NATURAL_FREQ 1193180

#define PIT_DATA0 0x40
#define PIT_DATA1 0x41
#define PIT_DATA2 0x42
#define PIT_COMMAND 0x43

#define MODULO 10

long long tick = 0;
uint8_t modcount = 0;

extern uint32_t allocated_frames;
extern uint32_t total_frames;

#define BUFFSIZE 26

static void timer_callback(registers_t regs)
{
    modcount = (modcount + 1) % MODULO;
    if(modcount == 0)
    {
        tick++;

        // This whole thing below is sort of ugly and hacky
        // but I guess it works for now.

        // 1  2  3  4  5  6  7  8  9  10 11 12 13 14
        // F  r  a  m  e  s  :     6  5  5  3  5

        // 15 16 17 18 19 20 21 22 23 24 25 26 27 28
        // M  e  m  :  x  x  x  x  /  x  x  x  x

        // 29 30 31 32
        // M  i  B  \0

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
        itos(allocated_frames, statusbuffer + 8, 6);
        statusbuffer[13] = ' ';

        char *mem = "Mem: ";
        i = 14;
        while(*mem != 0) {
            statusbuffer[i++] = *mem;
            mem++;
        }

        uint32_t used = (allocated_frames * 0x1000) / 1024 / 1024;
        uint32_t available = (total_frames * 0x1000) / 1024 / 1024;

        itos(used, statusbuffer + 18, 5);
        statusbuffer[22] = '/';
        itos(available, statusbuffer + 23, 5);
        statusbuffer[27] = ' ';

        char *mib = "MiB";
        i = 28;
        while(*mib != 0) {
            statusbuffer[i++] = *mib;
            mib++;
        }
        statusbuffer[31] = 0;

        terminal_set_status(statusbuffer);
    }
}

void init_timer(uint32_t frequency)
{
    terminal_writestring("Initializing PIT timer\n");
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

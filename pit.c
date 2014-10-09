#include "pit.h"
#include "isr.h"
#include "port.h"
#include "terminal.h"

#define PIT_NATURAL_FREQ 1193180

#define PIT_DATA0 0x40
#define PIT_DATA1 0x41
#define PIT_DATA2 0x42
#define PIT_COMMAND 0x43

#define MODULO 100

long long tick = 0;
uint8_t modcount = 0;
char *x = "ABCDEFGHIJKLM";

static void timer_callback(registers_t regs)
{
    (void)regs;
    modcount = (modcount + 1) % MODULO;
    if(modcount == 0)
    {
        tick++;
        //  terminal_writestring("Tick: ");
        terminal_write_dec(tick);
        //  terminal_writestring("\n");
        //terminal_putchar(x[tick % 13]);
        terminal_putchar('\n');
    }
}

void init_timer(uint32_t frequency)
{

  terminal_writestring("Initializing PIT timer\n");

  register_interrupt_handler(IRQ0, &timer_callback);
  
  //  uint32_t divisor = PIT_NATURAL_FREQ / frequency;
  uint32_t divisor;
  if(frequency)
    divisor = 1193180 / frequency;
  else
    divisor = 0;
  /*
  http://wiki.osdev.org/Programmable_Interval_Timer#I.2FO_Ports
    
                       Channel  Access Mode        Operating Mode    BCD/Binary
  0x36 == 0011 0110 == 00       11 (lobyte/hibyte) 011 (square Wave) 0 (16-bit bin
  
)
  */
  terminal_writestring("Divisor: ");
  terminal_write_dec(divisor);
  terminal_writestring("\n");

  //outb(PIT_COMMAND, 0x36);
  outb(0x43, 0x34);

  //Chop freq up into bytes and send to data0 port
  uint8_t low = (uint8_t)(divisor & 0xFF);
  uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

  //  outb(PIT_DATA0, low);
  //  outb(PIT_DATA0, high);
  outb(0x40, low);
  outb(0x40, high);

  terminal_writestring("Done setting up PIT\n");
}

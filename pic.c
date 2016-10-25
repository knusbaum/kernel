#include "pic.h"
#include "port.h"
#include "kernio.h"

#define MASTER_COMMAND 0x20
#define MASTER_DATA 0x21
#define SLAVE_COMMAND 0xA0
#define SLAVE_DATA 0xA1

#define ICW1_ICW4 0x01       /* ICW4 (not) needed */
#define ICW1_SINGLE 0x02     /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04  /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08      /* Level triggered (edge) mode */
#define ICW1_INIT 0x10       /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

unsigned int io_wait()
{
    int j = 0;
    for(unsigned int i = 0; i < 0xFFFFFFFF; i++)
    {
        j = i;
    }
    return j;
}

void remap_pic(void)
{
    printf("Remapping the IRQ table\n");

    outb(MASTER_COMMAND, ICW1_INIT | ICW1_ICW4); // Start initialization sequence
    outb(SLAVE_COMMAND, ICW1_INIT | ICW1_ICW4);  // Start initialization sequence
    outb(MASTER_DATA, 0x20);                     // Remap to 0x20
    outb(SLAVE_DATA, 0x28);                      // Remap slave to 0x28
    outb(MASTER_DATA, 4);                        // Tell Master about slave @ IRQ 2
    outb(SLAVE_DATA, 2);                         // Tell slave PIC its cascade ID

    outb(MASTER_DATA, ICW4_8086);
    outb(SLAVE_DATA, ICW4_8086);

    outb(MASTER_DATA, 0x0);
    outb(SLAVE_DATA, 0x0);
}

#include "ata_pio_drv.h"
#include "port.h"
#include "kheap.h"
#include "common.h"
#include "kernio.h"

#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRIVE_HEAD   0x1F6
#define ATA_PRIMARY_COMM_REGSTAT 0x1F7
#define ATA_PRIMARY_ALTSTAT_DCR  0x3F6


#define STAT_ERR  (1 << 0) // Indicates an error occurred. Send a new command to clear it
#define STAT_DRQ  (1 << 3) // Set when the drive has PIO data to transfer, or is ready to accept PIO data.
#define STAT_SRV  (1 << 4) // Overlapped Mode Service Request.
#define STAT_DF   (1 << 5) // Drive Fault Error (does not set ERR).
#define STAT_RDY  (1 << 6) // Bit is clear when drive is spun down, or after an error. Set otherwise.
#define STAT_BSY  (1 << 7) // Indicates the drive is preparing to send/receive data (wait for it to clear).
                           // In case of 'hang' (it never clears), do a software reset.


/**
 * To use the IDENTIFY command, select a target drive by sending 0xA0 for the master drive,
 * or 0xB0 for the slave, to the "drive select" IO port. On the Primary bus, this would be port 0x1F6.
 * Then set the Sectorcount, LBAlo, LBAmid, and LBAhi IO ports to 0 (port 0x1F2 to 0x1F5). Then send
 * the IDENTIFY command (0xEC) to the Command IO port (0x1F7).
 *
 * Then read the Status port (0x1F7) again.
 * If the value read is 0, the drive does not exist.
 * For any other value: poll the Status port (0x1F7) until bit 7 (BSY, value = 0x80) clears.
 * Because of some ATAPI drives that do not follow spec, at this point you need to check the
 * LBAmid and LBAhi ports (0x1F4 and 0x1F5) to see if they are non-zero. If so, the drive is not ATA,
 * and you should stop polling. Otherwise, continue polling one of the Status
 * ports until bit 3 (DRQ, value = 8) sets, or until bit 0 (ERR, value = 1) sets.
 *
 * At that point, if ERR is clear, the data is ready to read from the Data port (0x1F0).
 * Read 256 16-bit values, and store them.
 */

uint8_t identify() {
    inb(ATA_PRIMARY_COMM_REGSTAT);
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0);
    inb(ATA_PRIMARY_COMM_REGSTAT);
    outb(ATA_PRIMARY_SECCOUNT, 0);
    inb(ATA_PRIMARY_COMM_REGSTAT);
    outb(ATA_PRIMARY_LBA_LO, 0);
    inb(ATA_PRIMARY_COMM_REGSTAT);
    outb(ATA_PRIMARY_LBA_MID, 0);
    inb(ATA_PRIMARY_COMM_REGSTAT);
    outb(ATA_PRIMARY_LBA_HI, 0);
    inb(ATA_PRIMARY_COMM_REGSTAT);
    outb(ATA_PRIMARY_COMM_REGSTAT, 0xEC);
    outb(ATA_PRIMARY_COMM_REGSTAT, 0xE7);

    // Read the status port. If it's zero, the drive does not exist.
    uint8_t status = inb(ATA_PRIMARY_COMM_REGSTAT);

    printf("Waiting for status.\n");
    while(status & STAT_BSY) {
        uint32_t i = 0;
        while(1) {
            printf("Printing stuff %d\n", i);
            i++;
        }
        for(i = 0; i < 0x0FFFFFFF; i++) {}
        printf("Checking regstat.\n");
        status = inb(ATA_PRIMARY_COMM_REGSTAT);
    }
    
    if(status == 0) return 0;

    printf("Status indicates presence of a drive. Polling while STAT_BSY... ");
    while(status & STAT_BSY) {
      printf("\ninb(ATA_PRIMARY_COMM_REGSTAT);... ");
      status = inb(ATA_PRIMARY_COMM_REGSTAT);
    }
    printf("Done.\n");

    uint8_t mid = inb(ATA_PRIMARY_LBA_MID);
    uint8_t hi = inb(ATA_PRIMARY_LBA_HI);
    if(mid || hi) {
        // The drive is not ATA. (Who knows what it is.)
        return 0;
    }

    printf("Waiting for ERR or DRQ.\n");
    // Wait for ERR or DRQ
    while(!(status & (STAT_ERR | STAT_DRQ))) {
        status = inb(ATA_PRIMARY_COMM_REGSTAT);
    }

    if(status & STAT_ERR) {
        // There was an error on the drive. Forget about it.
        return 0;
    }

    printf("Reading IDENTIFY structure.\n");
    //uint8_t *buff = kmalloc(40960, 0, NULL);
    uint8_t buff[256 * 2];
    insw(ATA_PRIMARY_DATA, buff, 256);
    printf("Success. Disk is ready to go.\n");
    // We read it!
    return 1;
}

/**
 * An example of a 28 bit LBA PIO mode read on the Primary bus:
 *
 *     Send 0xE0 for the "master" or 0xF0 for the "slave", ORed with the highest 4 bits of the LBA to port 0x1F6: outb(0x1F6, 0xE0 | (slavebit << 4) | ((LBA >> 24) & 0x0F))
 *     Send a NULL byte to port 0x1F1, if you like (it is ignored and wastes lots of CPU time): outb(0x1F1, 0x00)
 *     Send the sectorcount to port 0x1F2: outb(0x1F2, (unsigned char) count)
 *     Send the low 8 bits of the LBA to port 0x1F3: outb(0x1F3, (unsigned char) LBA))
 *     Send the next 8 bits of the LBA to port 0x1F4: outb(0x1F4, (unsigned char)(LBA >> 8))
 *     Send the next 8 bits of the LBA to port 0x1F5: outb(0x1F5, (unsigned char)(LBA >> 16))
 *     Send the "READ SECTORS" command (0x20) to port 0x1F7: outb(0x1F7, 0x20)
 *     Wait for an IRQ or poll.
 *     Transfer 256 16-bit values, a uint16_t at a time, into your buffer from I/O port 0x1F0. (In assembler, REP INSW works well for this.)
 *     Then loop back to waiting for the next IRQ (or poll again -- see next note) for each successive sector.
 */


void ata_pio_read28(uint32_t LBA, uint8_t sectorcount, uint8_t *target) {
    // HARD CODE MASTER (for now)
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((LBA >> 24) & 0x0F));
    outb(ATA_PRIMARY_ERR, 0x00);
    outb(ATA_PRIMARY_SECCOUNT, sectorcount);
    outb(ATA_PRIMARY_LBA_LO, LBA & 0xFF);
    outb(ATA_PRIMARY_LBA_MID, (LBA >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA_HI, (LBA >> 16) & 0xFF);
    outb(ATA_PRIMARY_COMM_REGSTAT, 0x20);

    uint8_t i;
    for(i = 0; i < sectorcount; i++) {
        // POLL!
        while(1) {
            uint8_t status = inb(ATA_PRIMARY_COMM_REGSTAT);
            if(status & STAT_DRQ) {
                // Drive is ready to transfer data!
                break;
            }
        }
        // Transfer the data!
        insw(ATA_PRIMARY_DATA, (void *)target, 256);
        target += 256;
    }

}

/**
 * 48-bit LBA read
 *
 * Send 0x40 for the "master" or 0x50 for the "slave" to port 0x1F6: outb(0x1F6, 0x40 | (slavebit << 4))
 * outb (0x1F2, sectorcount high byte)
 * outb (0x1F3, LBA4)
 * outb (0x1F4, LBA5)
 * outb (0x1F5, LBA6)
 * outb (0x1F2, sectorcount low byte)
 * outb (0x1F3, LBA1)
 * outb (0x1F4, LBA2)
 * outb (0x1F5, LBA3)
 * Send the "READ SECTORS EXT" command (0x24) to port 0x1F7: outb(0x1F7, 0x24)
 */

void ata_pio_read48(uint64_t LBA, uint16_t sectorcount, uint8_t *target) {
    // HARD CODE MASTER (for now)
    outb(ATA_PRIMARY_DRIVE_HEAD, 0x40);                     // Select master
    outb(ATA_PRIMARY_SECCOUNT, (sectorcount >> 8) & 0xFF ); // sectorcount high
    outb(ATA_PRIMARY_LBA_LO, (LBA >> 24) & 0xFF);           // LBA4
    outb(ATA_PRIMARY_LBA_MID, (LBA >> 32) & 0xFF);          // LBA5
    outb(ATA_PRIMARY_LBA_HI, (LBA >> 40) & 0xFF);           // LBA6
    outb(ATA_PRIMARY_SECCOUNT, sectorcount & 0xFF);         // sectorcount low
    outb(ATA_PRIMARY_LBA_LO, LBA & 0xFF);                   // LBA1
    outb(ATA_PRIMARY_LBA_MID, (LBA >> 8) & 0xFF);           // LBA2
    outb(ATA_PRIMARY_LBA_HI, (LBA >> 16) & 0xFF);           // LBA3
    outb(ATA_PRIMARY_COMM_REGSTAT, 0x24);                   // READ SECTORS EXT


    uint8_t i;
    for(i = 0; i < sectorcount; i++) {
        // POLL!
        while(1) {
            uint8_t status = inb(ATA_PRIMARY_COMM_REGSTAT);
            if(status & STAT_DRQ) {
                // Drive is ready to transfer data!
                break;
            }
        }
        // Transfer the data!
        insw(ATA_PRIMARY_DATA, (void *)target, 256);
        target += 256;
    }

}

/**
 * To write sectors in 48 bit PIO mode, send command "WRITE SECTORS EXT" (0x34), instead.
 * (As before, do not use REP OUTSW when writing.) And remember to do a Cache Flush after
 * each write command completes.
 */
void ata_pio_write48(uint64_t LBA, uint16_t sectorcount, uint8_t *target) {

    // HARD CODE MASTER (for now)
    outb(ATA_PRIMARY_DRIVE_HEAD, 0x40);                     // Select master
    outb(ATA_PRIMARY_SECCOUNT, (sectorcount >> 8) & 0xFF ); // sectorcount high
    outb(ATA_PRIMARY_LBA_LO, (LBA >> 24) & 0xFF);           // LBA4
    outb(ATA_PRIMARY_LBA_MID, (LBA >> 32) & 0xFF);          // LBA5
    outb(ATA_PRIMARY_LBA_HI, (LBA >> 40) & 0xFF);           // LBA6
    outb(ATA_PRIMARY_SECCOUNT, sectorcount & 0xFF);         // sectorcount low
    outb(ATA_PRIMARY_LBA_LO, LBA & 0xFF);                   // LBA1
    outb(ATA_PRIMARY_LBA_MID, (LBA >> 8) & 0xFF);           // LBA2
    outb(ATA_PRIMARY_LBA_HI, (LBA >> 16) & 0xFF);           // LBA3
    outb(ATA_PRIMARY_COMM_REGSTAT, 0x34);                   // READ SECTORS EXT

    uint8_t i;
    for(i = 0; i < sectorcount; i++) {
        // POLL!
        while(1) {
            uint8_t status = inb(ATA_PRIMARY_COMM_REGSTAT);
            if(status & STAT_DRQ) {
                // Drive is ready to transfer data!
                break;
            }
            else if(status & STAT_ERR) {
                PANIC("DISK SET ERROR STATUS!");
            }
        }
        // Transfer the data!
        outsw(ATA_PRIMARY_DATA, (void *)target, 256);
        target += 256;
    }

    // Flush the cache.
    outb(ATA_PRIMARY_COMM_REGSTAT, 0xE7);
    // Poll for BSY.
    while(inb(ATA_PRIMARY_COMM_REGSTAT) & STAT_BSY) {}
}

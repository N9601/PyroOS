/* ============================================================================
 *  PyroOS  -  ATA (IDE) disk driver, 28-bit LBA PIO
 * ----------------------------------------------------------------------------
 *  Drives the primary IDE channel's master disk directly over I/O ports. This
 *  is the same disk we booted from, but now accessed without the BIOS. PIO
 *  ("programmed I/O") means the CPU itself moves every word through the data
 *  port, 256 words (512 bytes) per sector.
 * ==========================================================================*/
#include "ata.h"
#include "ports.h"

#define ATA_DATA    0x1F0
#define ATA_SECCNT  0x1F2
#define ATA_LBA_LO  0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HI  0x1F5
#define ATA_DRIVE   0x1F6
#define ATA_STATUS  0x1F7   /* read: status */
#define ATA_CMD     0x1F7   /* write: command */

#define ST_BSY  0x80        /* busy */
#define ST_DF   0x20        /* drive fault */
#define ST_DRQ  0x08        /* data request ready */
#define ST_ERR  0x01        /* error */

#define ATA_TIMEOUT 1000000 /* status polls before giving up */

/* While BSY is set the drive owns the status register and its other bits mean
   nothing, so ERR, DF and DRQ are only looked at once BSY has cleared. */

/* Wait for the drive to be ready to transfer a sector (BSY clear, DRQ set). */
static int ata_wait(void)
{
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t s = inb(ATA_STATUS);
        if (s & ST_BSY)
            continue;
        if (s & (ST_ERR | ST_DF))
            return -1;
        if (s & ST_DRQ)
            return 0;
    }
    return -1;
}

/* Wait for the drive to finish whatever it is doing (BSY clear). Returns -1 if
   it reports an error or never finishes. */
static int ata_idle(void)
{
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t s = inb(ATA_STATUS);
        if (!(s & ST_BSY))
            return (s & (ST_ERR | ST_DF)) ? -1 : 0;
    }
    return -1;
}

static int ata_select(uint32_t lba, uint8_t sectors)
{
    /* Command registers may only be written while the drive is not busy. */
    for (int i = 0; inb(ATA_STATUS) & ST_BSY; i++)
        if (i == ATA_TIMEOUT)
            return -1;
    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));   /* master + LBA mode + high bits */
    outb(ATA_SECCNT, sectors);
    outb(ATA_LBA_LO,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HI,  (uint8_t)((lba >> 16) & 0xFF));
    return 0;
}

int ata_read(uint32_t lba, uint8_t sectors, void *buffer)
{
    uint16_t *buf = (uint16_t *)buffer;
    if (ata_select(lba, sectors) != 0)
        return -1;
    outb(ATA_CMD, 0x20);                            /* READ SECTORS */

    for (int s = 0; s < sectors; s++) {
        if (ata_wait() != 0)
            return -1;
        for (int i = 0; i < 256; i++)
            buf[i] = inw(ATA_DATA);
        buf += 256;
    }
    return 0;
}

int ata_write(uint32_t lba, uint8_t sectors, const void *buffer)
{
    const uint16_t *buf = (const uint16_t *)buffer;
    if (ata_select(lba, sectors) != 0)
        return -1;
    outb(ATA_CMD, 0x30);                            /* WRITE SECTORS */

    for (int s = 0; s < sectors; s++) {
        if (ata_wait() != 0)
            return -1;
        for (int i = 0; i < 256; i++)
            outw(ATA_DATA, buf[i]);
        buf += 256;
    }

    /* The drive goes busy while it commits the last sector. Let it finish
       (and report how that went) before issuing the next command. */
    if (ata_idle() != 0)
        return -1;
    outb(ATA_CMD, 0xE7);                            /* FLUSH CACHE */
    return ata_idle();
}

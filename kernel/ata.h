/* ============================================================================
 *  PyroOS  -  ATA (IDE) disk driver, PIO mode
 * ==========================================================================*/
#ifndef ATA_H
#define ATA_H

#include <stdint.h>

/* Read/write `sectors` 512-byte sectors starting at LBA into/from buffer.
   Return 0 on success, -1 on error. Primary bus, master drive. */
int ata_read(uint32_t lba, uint8_t sectors, void *buffer);
int ata_write(uint32_t lba, uint8_t sectors, const void *buffer);

#endif

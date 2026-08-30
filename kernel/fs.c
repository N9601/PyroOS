/* ============================================================================
 *  PyroOS  -  PyroFS, a minimal filesystem
 * ----------------------------------------------------------------------------
 *  On-disk layout (LBAs on the boot disk):
 *      99   superblock  { magic, next_free_lba }
 *     100   directory   16 entries x 32 bytes = 512 bytes
 *     101+  file data    each file gets a fixed 8-sector (4 KB) slot
 *
 *  Files are stored contiguously. Allocation is a simple bump pointer
 *  (next_free_lba); there is no free-space reclamation on rewrite, which keeps
 *  the code small and correct. Plenty for a teaching filesystem.
 * ==========================================================================*/
#include "fs.h"
#include "ata.h"
#include "string.h"
#include "screen.h"

#define FS_MAGIC     0x50594F53u    /* 'PYOS' */
#define SUPER_LBA    99
#define DIR_LBA      100
#define DATA_LBA     101
#define MAX_FILES    16
#define NAME_LEN     20
#define SLOT_SECTORS 32             /* fixed 16 KB per file. Was 8 (4 KB), which
                                       silently truncated any ELF: the linker
                                       page-aligns .text to file offset 0x1000,
                                       so even a 300-byte program lands past a
                                       4 KB cut. 16 files x 16 KB = 256 KB, well
                                       inside the 1 MB image. */

typedef struct {
    char     name[NAME_LEN];
    uint32_t size;
    uint32_t start_lba;
    uint32_t used;
} fs_entry_t;                       /* exactly 32 bytes */

typedef struct {
    uint32_t magic;
    uint32_t next_free_lba;
    uint8_t  pad[512 - 8];
} fs_super_t;                       /* exactly 512 bytes */

static fs_super_t super;
static fs_entry_t dir[MAX_FILES];   /* 512 bytes total */
static uint8_t    secbuf[512];

static void load_meta(void)
{
    ata_read(SUPER_LBA, 1, &super);
    ata_read(DIR_LBA, 1, dir);
}

static void format(void)
{
    memset(&super, 0, sizeof(super));
    super.magic = FS_MAGIC;
    super.next_free_lba = DATA_LBA;
    memset(dir, 0, sizeof(dir));
    ata_write(SUPER_LBA, 1, &super);
    ata_write(DIR_LBA, 1, dir);
}

void fs_init(void)
{
    load_meta();
    if (super.magic != FS_MAGIC)
        format();
}

static fs_entry_t *find(const char *name)
{
    for (int i = 0; i < MAX_FILES; i++)
        if (dir[i].used && strcmp(dir[i].name, name) == 0)
            return &dir[i];
    return 0;
}

void fs_list(void)
{
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (dir[i].used) {
            kprint("  ");
            kprint(dir[i].name);
            kprint("  (");
            kprint_dec(dir[i].size);
            kprint(" bytes)\n");
            count++;
        }
    }
    if (!count)
        kprint("  (no files)\n");
}

int fs_write(const char *name, const void *data, uint32_t size)
{
    fs_entry_t *e = find(name);
    if (!e) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (!dir[i].used) { e = &dir[i]; break; }
        }
        if (!e)
            return -1;                          /* directory full */
        e->used = 1;
        strncpy_z(e->name, name, NAME_LEN);
        e->start_lba = super.next_free_lba;
        super.next_free_lba += SLOT_SECTORS;    /* reserve a fixed slot */
        ata_write(SUPER_LBA, 1, &super);
    }

    if (size > SLOT_SECTORS * 512)
        size = SLOT_SECTORS * 512;              /* clamp to the slot */

    uint32_t nsec = (size + 511) / 512;
    if (nsec == 0)
        nsec = 1;

    const uint8_t *src = (const uint8_t *)data;
    for (uint32_t s = 0; s < nsec; s++) {
        memset(secbuf, 0, 512);
        uint32_t off = s * 512;
        uint32_t chunk = (size > off) ? (size - off) : 0;
        if (chunk > 512) chunk = 512;
        if (chunk) memcpy(secbuf, src + off, chunk);
        if (ata_write(e->start_lba + s, 1, secbuf) != 0)
            return -1;
    }

    e->size = size;
    ata_write(DIR_LBA, 1, dir);
    return 0;
}

int fs_read(const char *name, void *buf, uint32_t max, uint32_t *out)
{
    fs_entry_t *e = find(name);
    if (!e)
        return -1;

    uint32_t size = e->size;
    if (size > max)
        size = max;

    uint32_t nsec = (size + 511) / 512;
    uint8_t *dst = (uint8_t *)buf;
    for (uint32_t s = 0; s < nsec; s++) {
        if (ata_read(e->start_lba + s, 1, secbuf) != 0)
            return -1;
        uint32_t off = s * 512;
        uint32_t chunk = (size > off) ? (size - off) : 0;
        if (chunk > 512) chunk = 512;
        if (chunk) memcpy(dst + off, secbuf, chunk);
    }

    if (out)
        *out = size;
    return 0;
}

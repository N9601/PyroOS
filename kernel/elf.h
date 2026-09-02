/* ============================================================================
 *  PyroOS  -  ELF32 executable format
 * ----------------------------------------------------------------------------
 *  Until now PyroOS ran flat binaries: objcopy stripped every header off a
 *  linked program, leaving raw machine code that had to be loaded at exactly
 *  the address it was linked for. That works, but it throws away everything the
 *  linker knew. There is no entry point recorded, so the loader has to assume
 *  the first byte is the first instruction. There is no segment table, so .bss
 *  cannot be zeroed and read-only data cannot be marked read-only.
 *
 *  ELF keeps that information. The header names the entry point, and the
 *  program header table says which parts of the file go where in memory, how
 *  much space each needs once loaded, and what permissions it wants. That is
 *  exactly the input a real exec needs.
 * ==========================================================================*/
#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define ELF_MAGIC0  0x7F        /* the four bytes every ELF file starts with */
#define ELF_MAGIC1  'E'
#define ELF_MAGIC2  'L'
#define ELF_MAGIC3  'F'

#define ELFCLASS32  1           /* 32-bit objects */
#define ELFDATA2LSB 1           /* little endian */
#define ET_EXEC     2           /* an executable, not a relocatable or shared object */
#define EM_386      3           /* Intel 80386 */

#define PT_LOAD     1           /* a segment that must be copied into memory */

/* Segment permission bits, as the linker recorded them. */
#define PF_X        0x1
#define PF_W        0x2
#define PF_R        0x4

typedef struct {
    uint8_t  e_ident[16];       /* magic, class, endianness, version */
    uint16_t e_type;            /* ET_EXEC for something we can run */
    uint16_t e_machine;         /* EM_386 */
    uint32_t e_version;
    uint32_t e_entry;           /* virtual address of the first instruction */
    uint32_t e_phoff;           /* file offset of the program header table */
    uint32_t e_shoff;           /* section headers: for linkers, not loaders */
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;       /* size of one program header entry */
    uint16_t e_phnum;           /* how many of them */
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf32_ehdr_t;

typedef struct {
    uint32_t p_type;            /* PT_LOAD is the only one a loader must honour */
    uint32_t p_offset;          /* where the bytes are in the file */
    uint32_t p_vaddr;           /* where they belong in memory */
    uint32_t p_paddr;
    uint32_t p_filesz;          /* how many bytes the file actually holds */
    uint32_t p_memsz;           /* how much memory it needs once loaded */
    uint32_t p_flags;           /* PF_R | PF_W | PF_X */
    uint32_t p_align;
} __attribute__((packed)) elf32_phdr_t;

/* Why p_memsz can exceed p_filesz: .bss is zero-initialised data. Storing a
   megabyte of zeroes in the file would be wasteful, so the linker records the
   size and leaves the bytes out. The loader is expected to zero the difference,
   and a loader that forgets hands the program uninitialised garbage. */

typedef enum {
    ELF_OK = 0,
    ELF_BAD_MAGIC,              /* not an ELF file at all */
    ELF_BAD_CLASS,              /* 64-bit, or the wrong endianness */
    ELF_BAD_TYPE,               /* not an executable */
    ELF_BAD_MACHINE,            /* built for another architecture */
    ELF_BAD_HEADERS,            /* header table runs off the end of the image */
    ELF_NO_SEGMENTS             /* nothing to load */
} elf_status_t;

elf_status_t elf_validate(const void *image, uint32_t size);
const char  *elf_status_name(elf_status_t s);
void         elf_dump(const void *image, uint32_t size);   /* print the headers */

/* Load a validated image. Segments and entry must fall inside [lo, hi).
   Returns 0, or -1 invalid, -2 segment out of range, -3 entry out of range. */
int          elf_load(const void *image, uint32_t size, uint32_t lo, uint32_t hi,
                      uint32_t *entry_out);

/* After loading, make read-only segments actually read-only to ring 3. */
void         elf_protect(const void *image);

#endif

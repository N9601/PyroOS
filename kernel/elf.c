/* ============================================================================
 *  PyroOS  -  ELF32 validation and inspection
 * ----------------------------------------------------------------------------
 *  Everything here treats the image as hostile. An executable read off disk is
 *  attacker-controlled data as far as the kernel is concerned, and a loader
 *  that trusts its offsets is a loader that can be talked into copying bytes
 *  anywhere in memory. So every offset is bounds-checked against the actual
 *  size of the image before it is used.
 * ==========================================================================*/
#include "elf.h"
#include "screen.h"
#include "string.h"

elf_status_t elf_validate(const void *image, uint32_t size)
{
    if (!image || size < sizeof(elf32_ehdr_t))
        return ELF_BAD_MAGIC;

    const elf32_ehdr_t *eh = (const elf32_ehdr_t *)image;

    if (eh->e_ident[0] != ELF_MAGIC0 || eh->e_ident[1] != ELF_MAGIC1 ||
        eh->e_ident[2] != ELF_MAGIC2 || eh->e_ident[3] != ELF_MAGIC3)
        return ELF_BAD_MAGIC;

    if (eh->e_ident[4] != ELFCLASS32 || eh->e_ident[5] != ELFDATA2LSB)
        return ELF_BAD_CLASS;       /* a 64-bit binary would be misread wholesale */

    if (eh->e_type != ET_EXEC)
        return ELF_BAD_TYPE;        /* shared objects need a dynamic linker */

    if (eh->e_machine != EM_386)
        return ELF_BAD_MACHINE;

    if (eh->e_phnum == 0 || eh->e_phentsize != sizeof(elf32_phdr_t))
        return ELF_NO_SEGMENTS;

    /* The program header table must lie entirely inside the image. Computed in
       64-bit so a crafted e_phoff near 4 GB cannot wrap around and pass. */
    uint64_t ph_end = (uint64_t)eh->e_phoff +
                      (uint64_t)eh->e_phnum * (uint64_t)eh->e_phentsize;
    if (ph_end > (uint64_t)size)
        return ELF_BAD_HEADERS;

    /* Every loadable segment's file range must lie inside the image too. */
    const elf32_phdr_t *ph = (const elf32_phdr_t *)((const uint8_t *)image + eh->e_phoff);
    int loadable = 0;
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD)
            continue;
        loadable++;
        if (ph[i].p_memsz < ph[i].p_filesz)
            return ELF_BAD_HEADERS;         /* would imply a negative .bss */
        uint64_t seg_end = (uint64_t)ph[i].p_offset + (uint64_t)ph[i].p_filesz;
        if (seg_end > (uint64_t)size)
            return ELF_BAD_HEADERS;
    }
    if (!loadable)
        return ELF_NO_SEGMENTS;

    return ELF_OK;
}

const char *elf_status_name(elf_status_t s)
{
    switch (s) {
    case ELF_OK:           return "ok";
    case ELF_BAD_MAGIC:    return "not an ELF file";
    case ELF_BAD_CLASS:    return "not 32-bit little-endian";
    case ELF_BAD_TYPE:     return "not an executable";
    case ELF_BAD_MACHINE:  return "wrong architecture";
    case ELF_BAD_HEADERS:  return "headers point outside the file";
    case ELF_NO_SEGMENTS:  return "nothing to load";
    default:               return "unknown";
    }
}

void elf_dump(const void *image, uint32_t size)
{
    elf_status_t st = elf_validate(image, size);
    kprint("  validation: ");
    kprint(elf_status_name(st));
    kprint_char('\n');
    if (st != ELF_OK)
        return;

    const elf32_ehdr_t *eh = (const elf32_ehdr_t *)image;
    kprint("  entry point: ");
    kprint_hex(eh->e_entry);
    kprint(", ");
    kprint_dec(eh->e_phnum);
    kprint(" program headers\n");

    const elf32_phdr_t *ph = (const elf32_phdr_t *)((const uint8_t *)image + eh->e_phoff);
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD)
            continue;
        kprint("  load ");
        kprint_hex(ph[i].p_vaddr);
        kprint(" file ");
        kprint_dec(ph[i].p_filesz);
        kprint(" mem ");
        kprint_dec(ph[i].p_memsz);
        kprint(ph[i].p_flags & PF_W ? " rw" : " r-");
        kprint(ph[i].p_flags & PF_X ? "x" : "-");
        if (ph[i].p_memsz > ph[i].p_filesz) {
            kprint("  (bss ");
            kprint_dec(ph[i].p_memsz - ph[i].p_filesz);
            kprint(" bytes)");
        }
        kprint_char('\n');
    }
}

/* ----------------------------------------------------------------------------
 *  elf_load: copy a validated image into memory and report the entry point.
 *
 *  For each PT_LOAD segment: copy p_filesz bytes from the file to p_vaddr, then
 *  zero the p_memsz - p_filesz bytes beyond it. That second step is .bss. The
 *  linker records how much zeroed space the program needs but does not store
 *  the zeroes, because a megabyte of nothing is still a megabyte on disk. A
 *  loader that skips it hands the program whatever the last tenant left behind,
 *  which is both a correctness bug and an information leak.
 *
 *  Segments are confined to the user zone. Nothing stops a hand-crafted ELF
 *  from claiming p_vaddr in the middle of the kernel, and honouring that would
 *  turn "run this program" into "overwrite the interrupt table".
 * --------------------------------------------------------------------------*/
int elf_load(const void *image, uint32_t size, uint32_t lo, uint32_t hi,
             uint32_t *entry_out)
{
    if (elf_validate(image, size) != ELF_OK)
        return -1;

    const elf32_ehdr_t *eh = (const elf32_ehdr_t *)image;
    const elf32_phdr_t *ph = (const elf32_phdr_t *)((const uint8_t *)image + eh->e_phoff);

    /* Check every segment fits the permitted window before copying any of them,
       so a bad ELF cannot leave memory half-written. */
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD || ph[i].p_memsz == 0)
            continue;
        uint64_t start = (uint64_t)ph[i].p_vaddr;
        uint64_t end   = start + (uint64_t)ph[i].p_memsz;
        if (start < (uint64_t)lo || end > (uint64_t)hi)
            return -2;                  /* segment outside the user zone */
    }

    if (eh->e_entry < lo || eh->e_entry >= hi)
        return -3;                      /* entry point outside the user zone */

    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD || ph[i].p_memsz == 0)
            continue;
        uint8_t *dst = (uint8_t *)ph[i].p_vaddr;
        const uint8_t *src = (const uint8_t *)image + ph[i].p_offset;
        memcpy(dst, src, ph[i].p_filesz);
        memset(dst + ph[i].p_filesz, 0, ph[i].p_memsz - ph[i].p_filesz);
    }

    if (entry_out)
        *entry_out = eh->e_entry;
    return 0;
}

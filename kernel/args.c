/* ============================================================================
 *  PyroOS  -  building argc/argv on the user stack
 * ----------------------------------------------------------------------------
 *  The layout the program expects on entry, growing downward from the top of
 *  the user stack:
 *
 *      stack_top ->  [ argument text: "exec\0prog.elf\0hello\0" ]
 *                    [ argv[argc] = NULL                        ]
 *                    [ argv[argc-1] ... argv[0]  (pointers)     ]
 *                    [ argv (pointer to the array)              ]
 *      new esp   ->  [ argc                                     ]
 *
 *  So a program reading [esp] finds argc and [esp+4] finds argv, which is the
 *  ordinary cdecl arrangement for a call to main(argc, argv). The strings sit
 *  above the pointers because the pointers have to point at something that will
 *  not be overwritten as the program pushes its own frames.
 *
 *  Everything is written into the user zone, which ring 3 can read. That is the
 *  point: the program must be able to reach its own arguments without a syscall.
 * ==========================================================================*/
#include "args.h"
#include "string.h"

uint32_t args_build(const char *line, uint32_t stack_top)
{
    const char *argp[ARG_MAX];
    uint32_t    arglen[ARG_MAX];
    int argc = 0;
    uint32_t total = 0;

    /* Split on runs of spaces. Empty gaps produce no argument. */
    const char *p = line;
    while (*p && argc < ARG_MAX) {
        while (*p == ' ')
            p++;
        if (!*p)
            break;
        const char *start = p;
        while (*p && *p != ' ')
            p++;
        uint32_t len = (uint32_t)(p - start);
        argp[argc] = start;
        arglen[argc] = len;
        total += len + 1;               /* the NUL terminator counts */
        argc++;
    }

    if (argc == 0 || total > ARG_BYTES)
        return 0;

    /* Copy the strings down from the top, remembering where each landed. */
    uint32_t sp = stack_top - total;
    sp &= ~3u;                          /* keep the stack 4-byte aligned */
    uint32_t text = sp;
    uint32_t slot[ARG_MAX];
    uint32_t off = 0;
    for (int i = 0; i < argc; i++) {
        char *dst = (char *)(text + off);
        memcpy(dst, argp[i], arglen[i]);
        dst[arglen[i]] = '\0';
        slot[i] = text + off;
        off += arglen[i] + 1;
    }

    /* Then the pointer array, NULL-terminated as the convention requires, so a
       program can walk argv without consulting argc. */
    sp -= (uint32_t)(argc + 1) * 4;
    uint32_t *argv = (uint32_t *)sp;
    for (int i = 0; i < argc; i++)
        argv[i] = slot[i];
    argv[argc] = 0;
    uint32_t argv_addr = sp;

    sp -= 4;
    *(uint32_t *)sp = argv_addr;        /* the argv argument */
    sp -= 4;
    *(uint32_t *)sp = (uint32_t)argc;   /* the argc argument */

    /* Finally a fake return address, because the entry point is an ordinary C
       function and the compiler generates it expecting to have been called.
       It reads its first argument at [esp+4], not [esp]. Nothing ever returns
       through this slot: a program leaves by SYS_EXIT or by faulting. */
    sp -= 4;
    *(uint32_t *)sp = 0;

    return sp;
}

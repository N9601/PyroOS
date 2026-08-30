/* ============================================================================
 *  PyroOS  -  argument passing to ring-3 programs
 * ----------------------------------------------------------------------------
 *  A program that cannot be told what to work on has to hardcode it. argc and
 *  argv are how every UNIX process receives its instructions, and they are
 *  built by the kernel, not the program: the strings are copied onto the user
 *  stack before the program starts, and the entry point finds them there.
 * ==========================================================================*/
#ifndef ARGS_H
#define ARGS_H

#include <stdint.h>

#define ARG_MAX      8      /* arguments per program, argv[0] included */
#define ARG_BYTES  256      /* total bytes of argument text */

/* Split `line` on spaces and build an argc/argv block on the user stack ending
   at stack_top. Returns the new stack pointer the program should start with,
   or 0 if the arguments do not fit. */
uint32_t args_build(const char *line, uint32_t stack_top);

#endif

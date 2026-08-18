/* ============================================================================
 *  PyroOS  -  PyroFS, a minimal filesystem
 * ==========================================================================*/
#ifndef FS_H
#define FS_H

#include <stdint.h>

void fs_init(void);                 /* load metadata; format the disk if new */
void fs_list(void);                 /* print all files */
int  fs_write(const char *name, const void *data, uint32_t size);      /* create/overwrite */
int  fs_read(const char *name, void *buf, uint32_t max, uint32_t *out); /* read into buf */

#endif

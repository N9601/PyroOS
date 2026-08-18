/* ============================================================================
 *  PyroOS  -  pixel-art flame logo
 * ==========================================================================*/
#ifndef LOGO_H
#define LOGO_H

#include <stdint.h>

void draw_logo(void);              /* clear the screen and draw the logo */
void logo_splash(uint32_t ticks);  /* draw it, hold for `ticks`, then clear */

#endif

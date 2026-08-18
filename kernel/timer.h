/* ============================================================================
 *  PyroOS  -  programmable interval timer (PIT) driver
 * ==========================================================================*/
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_install(uint32_t frequency_hz);  /* start the timer at N ticks/sec */
uint32_t timer_ticks(void);                 /* total ticks since boot */

#endif

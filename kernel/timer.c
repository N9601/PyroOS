/* ============================================================================
 *  PyroOS  -  programmable interval timer (PIT), IRQ0
 * ----------------------------------------------------------------------------
 *  The PIT fires IRQ0 at a rate we choose. We just count the ticks; later
 *  milestones (a scheduler) will use this as the system heartbeat.
 * ==========================================================================*/
#include "timer.h"
#include "isr.h"
#include "ports.h"
#include "task.h"

static volatile uint32_t tick = 0;

static void timer_callback(registers_t *r)
{
    (void)r;
    tick++;
    preempt_tick();     /* drive preemptive scheduling when it is armed */
}

uint32_t timer_ticks(void)
{
    return tick;
}

void timer_install(uint32_t frequency_hz)
{
    register_interrupt_handler(32, timer_callback);   /* IRQ0 -> vector 32 */

    /* The PIT runs at 1193180 Hz. Dividing that by our target frequency gives
       the reload value that produces the rate we want. */
    uint32_t divisor = 1193180 / frequency_hz;

    outb(0x43, 0x36);                       /* command: channel 0, rate generator */
    outb(0x40, (uint8_t)(divisor & 0xFF));         /* low byte of divisor */
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));  /* high byte of divisor */
}

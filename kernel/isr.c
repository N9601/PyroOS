/* ============================================================================
 *  PyroOS  -  interrupt dispatch, PIC remap, handler registry
 * ==========================================================================*/
#include "isr.h"
#include "idt.h"
#include "ports.h"
#include "screen.h"
#include "context.h"

/* The 48 stubs from interrupt.asm. */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);  extern void isr3(void);
extern void isr4(void);  extern void isr5(void);  extern void isr6(void);  extern void isr7(void);
extern void isr8(void);  extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);  extern void irq3(void);
extern void irq4(void);  extern void irq5(void);  extern void irq6(void);  extern void irq7(void);
extern void irq8(void);  extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void); extern void irq15(void);

/* Registered C handlers, indexed by interrupt number. */
static isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler)
{
    interrupt_handlers[n] = handler;
}

/* The PIC defaults to sending IRQs on interrupt numbers 0-15, which collide
   with the CPU's own exception numbers. We reprogram it to use 32-47 instead.
   This is the classic 8259 initialization dance (ICW1..ICW4). */
static void pic_remap(void)
{
    outb(0x20, 0x11);   /* start init, master PIC */
    outb(0xA0, 0x11);   /* start init, slave PIC */
    outb(0x21, 0x20);   /* master vector offset -> 32 */
    outb(0xA1, 0x28);   /* slave vector offset  -> 40 */
    outb(0x21, 0x04);   /* tell master: slave is at IRQ2 */
    outb(0xA1, 0x02);   /* tell slave its cascade identity */
    outb(0x21, 0x01);   /* 8086 mode, master */
    outb(0xA1, 0x01);   /* 8086 mode, slave */
    outb(0x21, 0x00);   /* unmask all IRQs on master */
    outb(0xA1, 0x00);   /* unmask all IRQs on slave */
}

void isr_install(void)
{
    /* CPU exceptions: vectors 0-31. Flags 0x8E = present, ring 0, 32-bit gate.
       Selector 0x08 = our GDT code segment. */
    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

    pic_remap();

    /* Hardware IRQs: now delivered on vectors 32-47. */
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
}

static const char *exception_names[32] = {
    "divide by zero", "debug", "non-maskable interrupt", "breakpoint",
    "overflow", "bound range exceeded", "invalid opcode", "device not available",
    "double fault", "coprocessor segment overrun", "invalid TSS",
    "segment not present", "stack-segment fault", "general protection fault",
    "page fault", "reserved", "x87 floating-point", "alignment check",
    "machine check", "SIMD floating-point", "virtualization", "control protection",
    "reserved", "reserved", "reserved", "reserved", "reserved", "reserved",
    "hypervisor injection", "VMM communication", "security", "reserved"
};

/* Fault recovery: when armed, a CPU exception unwinds back to the saved
   context instead of halting. The caller must save the context itself (with
   save_context) in a stack frame that survives until the fault, then arm. */
ctx_t               fault_recovery_ctx;
volatile int        g_user_faulted = 0;   /* set when a fault triggered recovery */
static volatile int fault_armed = 0;

void fault_arm(void)    { fault_armed = 1; }
void fault_disarm(void) { fault_armed = 0; }

/* Called from isr_common_stub for CPU exceptions. Reports the fault; if
   recovery is armed, unwinds back to the caller, otherwise halts. */
void isr_handler(registers_t *r)
{
    kprint_color("\n  [CPU exception] ", COLOR_RED_ON_BLACK);
    if (r->int_no < 32)
        kprint_color(exception_names[r->int_no], COLOR_RED_ON_BLACK);
    kprint_color(" (int ", COLOR_RED_ON_BLACK);
    kprint_dec(r->int_no);
    kprint_color(")\n", COLOR_RED_ON_BLACK);

    if (r->int_no == 14) {                      /* page fault: CR2 holds the address */
        uint32_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        kprint_color("  faulting address ", COLOR_RED_ON_BLACK);
        kprint_hex(cr2);
        kprint_color(", error code ", COLOR_RED_ON_BLACK);
        kprint_hex(r->err_code);
        kprint_char('\n');
    }

    if (fault_armed) {
        fault_armed = 0;
        g_user_faulted = 1;
        restore_context(&fault_recovery_ctx);   /* recover: never returns here */
    }

    kprint_color("  no handler armed; halting.\n", COLOR_RED_ON_BLACK);
    for (;;)
        __asm__ volatile("hlt");
}

/* Called from irq_common_stub for hardware interrupts. */
void irq_handler(registers_t *r)
{
    /* Acknowledge the interrupt by sending End-Of-Interrupt to the PIC(s).
       If it came from the slave PIC (IRQ 8-15), the slave must be told too. */
    if (r->int_no >= 40)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);

    /* Dispatch to a registered driver, if any. */
    if (interrupt_handlers[r->int_no])
        interrupt_handlers[r->int_no](r);
}

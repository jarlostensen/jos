#ifndef JOS_INTERRUPTS_H
#define JOS_INTERRUPTS_H

#include <stdbool.h>

// initialise the interrupt module
void _k_init_isrs();
// effectively load IDT
void _k_load_isrs();

typedef void (*isr_handler_func_t)(uint16_t caller_cs, uint32_t caller_eip);
// register a handler for the given interrupt.
// this  can be done at any time after initialisation and the handler will be effective from the 
// next interrupt.
isr_handler_func_t k_set_isr_handler(int i, isr_handler_func_t handler);

typedef void (*irq_handler_func_t)(int irq_num);
// register a handler for the given IRQ.
// this  can be done at any time after initialisation and the handler will be effective from the next IRQ.
// NOTE: the irq number must be in the range 0..31, i.e. irrespective of PIC remapping
// this does NOT enable the corresponding IRQ, use k_enable_irq for that
void k_set_irq_handler(int irq, irq_handler_func_t handler);

// enable the given irq
void k_enable_irq(int irq);
// disable the given IRQ
void k_disable_ieq(int irq);
// returns true if the given IRQ is enabled
bool k_irq_enabled(int irq);

#endif
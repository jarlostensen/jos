#ifndef JOS_INTERRUPTS_H
#define JOS_INTERRUPTS_H

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
void k_set_irq_handler(int irq, irq_handler_func_t handler);

#endif
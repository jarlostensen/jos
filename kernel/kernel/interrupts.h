#ifndef JOS_INTERRUPTS_H
#define JOS_INTERRUPTS_H

void k_init_isrs();
void k_load_isrs();

typedef void (*isr_handler_func_t)(void);
isr_handler_func_t k_set_isr_handler(int i, isr_handler_func_t handler);

#endif

#ifndef JOS_KERNEL_H
#define JOS_KERNEL_H

#define JOS_BOCHS_DBGBREAK() asm("xchg %bx,%bx")

extern int is_protected_mode();

extern void outb(uint16_t port, uint8_t value);
extern void outw(uint16_t port, uint16_t value);
extern uint8_t inb(uint16_t port);
extern uint16_t inw(uint16_t port);

#endif // JOS_KERNEL_H
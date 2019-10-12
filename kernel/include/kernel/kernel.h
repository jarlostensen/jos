
#ifndef JOS_KERNEL_H
#define JOS_KERNEL_H

#define JOS_BOCHS_DBGBREAK() asm("xchg %bx,%bx")

int k_is_protected_mode();
void k_panic();

void k_outb(uint16_t port, uint8_t value);
void k_outw(uint16_t port, uint16_t value);
uint8_t k_inb(uint16_t port);
uint16_t k_inw(uint16_t port);
// wait by doing a nop-write to port 0x80 (POST)
void k_io_wait(void);

#endif // JOS_KERNEL_H
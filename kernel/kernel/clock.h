#ifndef JOS_CLOCK_H
#define JOS_CLOCK_H

void k_init_clock();
uint64_t k_get_ms_since_boot();
void k_wait_oneshot_one_period();

#endif // JOS_CLOCK_H
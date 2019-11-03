#ifndef JOS_CLOCK_H
#define JOS_CLOCK_H

void k_clock_init();
uint64_t k_get_ms_since_boot();
void k_wait_oneshot_one_period();

// 32.32 fp resolution, i.e. millisecond accuracy
uint64_t k_clock_get_ms_res();

#endif // JOS_CLOCK_H
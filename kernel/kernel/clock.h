#ifndef JOS_CLOCK_H
#define JOS_CLOCK_H

void k_clock_init();
uint64_t k_clock_ms_since_boot();

// 32.32 fp resolution, i.e. millisecond accuracy
uint64_t k_clock_get_ms_res();
// estimates the number of cycles for a given number of milliseconds
uint64_t k_clock_ms_to_cycles(uint64_t ms);

#endif // JOS_CLOCK_H
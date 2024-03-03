#ifndef PERF_AWARE_HAVERSINE_H
#include "haversine.h"
#endif

#include <x86intrin.h>
#include <sys/time.h>

// returns number of microseconds in a second
u64 get_os_time_freq(void) {
    return 1000000;
}

// returns current unix timestamp in microseconds
u64 read_os_timer(void) {
    struct timeval value;
    gettimeofday(&value, 0);
    u64 result = get_os_time_freq()*(u64)value.tv_sec + (u64)value.tv_usec;
    return result;
}

// returns virtual counter value stored in cntvct_el0 register
// (this is for ARM; in x64 would use __rdtsc instead)
u64 read_cpu_timer(void) {
//    u64 value;
//    asm volatile("mrs %0, cntvct_el0" : "=r"(value));
//    return value;
    return __rdtsc();
}

// determine the number of CPU cycle counts per second by counting
// the number of CPU cycles that happen in a known second, namely
// a second of OS time (though you can use less than a second to
// calculate this; here we use 100 milliseconds)
u64 estimate_cpu_timer_freq(void) {
    u64 milliseconds_to_wait = 100;
    u64 os_freq = get_os_time_freq();

    u64 cpu_start = read_cpu_timer();
    u64 os_start = read_os_timer();
    u64 os_end = 0;
    u64 os_elapsed = 0;
    u64 os_wait_time = os_freq * milliseconds_to_wait / 1000;

    while (os_elapsed < os_wait_time) {
        os_end = read_os_timer();
        os_elapsed = os_end - os_start;
    }

    u64 cpu_end = read_cpu_timer();
    u64 cpu_elapsed = cpu_end - cpu_start;

    u64 cpu_freq = 0;
    if (os_elapsed) {
        cpu_freq = os_freq * cpu_elapsed / os_elapsed;
    }

    return cpu_freq;
}

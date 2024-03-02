#ifndef PERF_AWARE_HAVERSINE_H
#include "haversine.h"
#endif

#include <sys/time.h>

u64 get_os_time_freq(void) {
    return 1000000;
}

u64 read_os_timer(void) {
    struct timeval value;
    gettimeofday(&value, 0);

    u64 result = get_os_time_freq()*(u64)value.tv_sec + (u64)value.tv_usec;
    return result;
}

u64 read_cpu_timer(void) {
    u64 value;
    asm volatile("mrs %0, cntvct_el0" : "=r"(value));
    return value;
}

int main(int argc, char *argv[]) {
    u64 MillisecondsToWait = 1000;
    if(argc == 2)
    {
        MillisecondsToWait = atol(argv[1]);
    }

    u64 OSFreq = get_os_time_freq();
    printf("    OS Freq: %llu (reported)\n", OSFreq);

    u64 CPUStart = read_cpu_timer();
    u64 OSStart = read_os_timer();
    u64 OSEnd = 0;
    u64 OSElapsed = 0;
    u64 OSWaitTime = OSFreq * MillisecondsToWait / 1000;
    while(OSElapsed < OSWaitTime)
    {
        OSEnd = read_os_timer();
        OSElapsed = OSEnd - OSStart;
    }

    u64 CPUEnd = read_cpu_timer();
    u64 CPUElapsed = CPUEnd - CPUStart;
    u64 CPUFreq = 0;
    if(OSElapsed)
    {
        CPUFreq = OSFreq * CPUElapsed / OSElapsed;
    }

    printf("   OS Timer: %llu -> %llu = %llu elapsed\n", OSStart, OSEnd, OSElapsed);
    printf(" OS Seconds: %.4f\n", (f64)OSElapsed/(f64)OSFreq);

    printf("  CPU Timer: %llu -> %llu = %llu elapsed\n", CPUStart, CPUEnd, CPUElapsed);
    printf("   CPU Freq: %llu (guessed)\n", CPUFreq);

    return 0;
}
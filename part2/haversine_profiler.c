#ifndef PERF_AWARE_HAVERSINE_H
#include "haversine.h"
#include "haversine_clock.c"
#endif

#define MAX_ID 32

typedef struct {
    char id[MAX_ID];
    u64 begin;
    u64 end;
} Profile;

typedef struct {
    Profile profiles[4096];
    u32 count;
    u64 global_begin;
    u64 global_end;
} GlobalProfiler;

GlobalProfiler g_profiler;

#define NAME_CONCAT(a, b) a##b
#define NAME(a, b) NAME_CONCAT(a, b)
#define BEGIN_TIME_BLOCK(string)   u32 NAME(idx_, __LINE__) = g_profiler.count++; \
                                   strcpy(g_profiler.profiles[NAME(idx_, __LINE__)].id, (string)); \
                                   g_profiler.profiles[NAME(idx_, __LINE__)].begin = read_cpu_timer();
#define END_TIME_BLOCK(string)     g_profiler.profiles[find_profile((string))].end = read_cpu_timer();
#define BEGIN_TIME_FUNCTION     BEGIN_TIME_BLOCK(__func__)
#define END_TIME_FUNCTION       END_TIME_BLOCK(__func__)

void begin_profiler(void) {
    g_profiler.global_begin = read_cpu_timer();
    g_profiler.count = 0;
}

void end_profiler(void) {
    g_profiler.global_end = read_cpu_timer();
}

u32 find_profile(const char *string) {
    for (int i = 0; i < g_profiler.count; i++) {
        if (strcmp(string, g_profiler.profiles[i].id) == 0) {
            return i;
        }
    }
    return g_profiler.count;
}

void print_profile_results(void) {
    u64 total_elapsed = g_profiler.global_end - g_profiler.global_begin;
    if (total_elapsed <= 0) {
        fprintf(stderr, "ERROR: cannot report results before profiling complete\n");
        exit(1);
    }
    u64 cpu_freq = estimate_cpu_timer_freq();
    fprintf(stdout, "\nTotal time: %.4fms (CPU freq %llu)\n", 1000.0 * (f64)total_elapsed / (f64)cpu_freq, cpu_freq);

    for (int i = 0; i < g_profiler.count; i++) {
        Profile profile = g_profiler.profiles[i];
        u64 time = profile.end - profile.begin;
        fprintf(stdout, "\t%s: %llu (%.2f%%)\n", profile.id, time, (f64)time / (f64)total_elapsed * 100);
    }
}


#ifndef PERF_AWARE_HAVERSINE_H
#include "haversine.h"
#include "haversine_clock.c"
#endif

#define MAX_ID 32

typedef struct {
    char id[MAX_ID];
    u64 begin;
    u64 total;
    u32 parent_idx;
    u64 total_children;
    u64 times_called;
} Profile;

typedef struct {
    Profile profiles[4096];
    u32 count;
    u64 global_begin;
    u64 global_end;
} GlobalProfiler;

GlobalProfiler g_profiler;
u32 g_profiler_parent = 0; // 0 means no parent/not in block

#define NAME_CONCAT(a, b)          a##b
#define NAME(a, b)                 NAME_CONCAT(a, b)
#define BEGIN_TIME_BLOCK(string)   begin_block((string));
#define END_TIME_BLOCK(string)     end_block((string));
#define BEGIN_TIME_FUNCTION        BEGIN_TIME_BLOCK(__func__)
#define END_TIME_FUNCTION          END_TIME_BLOCK(__func__)

u32 find_profile(const char *string) {
    for (int i = 1; i < g_profiler.count; i++) {
        if (strcmp(string, g_profiler.profiles[i].id) == 0) {
            return i;
        }
    }
    return g_profiler.count;
}

void begin_profiler(void) {
    g_profiler.global_begin = read_cpu_timer();
    g_profiler.count = 1; // reserve first position
}

void end_profiler(void) {
    g_profiler.global_end = read_cpu_timer();
}

void begin_block(const char *id) {
    u32 idx = find_profile(id);
    Profile *profile = g_profiler.profiles + idx;
    profile->begin = read_cpu_timer();
    profile->total_children = 0;
    if (idx == g_profiler.count) {
        // first time this block is being called
        strcpy(profile->id, id);
        profile->total = 0;
        profile->times_called = 0;
        g_profiler.count++;
    }
    profile->times_called += 1;
    profile->parent_idx = g_profiler_parent;
    g_profiler_parent = idx;
}

void end_block(const char *id) {
    u32 idx = find_profile(id);
    Profile *profile = g_profiler.profiles + idx;
    u64 end = read_cpu_timer();
    u64 time = end - profile->begin;
    profile->total += time;

    Profile *parent = g_profiler.profiles + profile->parent_idx;
    parent->total_children += time;

    g_profiler_parent = profile->parent_idx;
}

void print_profile_results(void) {
    u64 total_elapsed = g_profiler.global_end - g_profiler.global_begin;
    u64 cpu_freq = estimate_cpu_timer_freq();
    fprintf(stdout, "\nTotal time: %.4fms (CPU freq %lu)\n", 1000.0 * (f64)total_elapsed / (f64)cpu_freq, cpu_freq);

    Profile *profile;
    for (int i = 1; i < g_profiler.count; i++) {
        profile = g_profiler.profiles + i;
        u64 elapsed = profile->total - profile->total_children;
        fprintf(stdout, "\t%s[%lu]: %lu (%.2f%%", profile->id, profile->times_called, elapsed, (f64)elapsed / (f64)total_elapsed * 100);
        if (profile->total_children) {
            f64 percent_with_children = 100.0 * ((f64)profile->total / (f64)total_elapsed);
            fprintf(stdout, ", w children: %.2f", percent_with_children);
        }
        fprintf(stdout, ")\n");
    }
}


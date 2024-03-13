#ifndef PERF_AWARE_HAVERSINE_H
#include "haversine.h"
#include "haversine_clock.c"
#endif

#define MAX_ID 32

typedef struct {
    char const *label;
    u64 tsc_elapsed_exclusive; // does NOT include children
    u64 tsc_elapsed_inclusive; // DOES include children
    u64 hit_count;
} Profile;

typedef struct {
    Profile profiles[4096];
    u64 start;
    u64 end;
} GlobalProfiler;

GlobalProfiler profiler;

typedef struct {
    char const *label;
    u64 old_tsc_elapsed_inclusive;
    u64 start_tsc;
    u32 anchor_idx;
} ProfileBlock;

// This means there cannot be blocks that overlap like [ { ] }
ProfileBlock stack[1024];
u32 sp = 1;

#define NAME_CONCAT(a, b)          a##b
#define NAME(a, b)                 NAME_CONCAT(a,b)
#define BEGIN_TIME_BLOCK(string)   start_block(string, __COUNTER__ + 1);
#define END_TIME_BLOCK(string)     end_block(string);
#define BEGIN_TIME_FUNCTION        BEGIN_TIME_BLOCK(__func__)
#define END_TIME_FUNCTION          END_TIME_BLOCK(__func__)

void start_block(char const *label, u32 idx) {
    Profile *anchor = profiler.profiles + idx;
    anchor->label = label; // unfortunately no easy way to write this once

    ProfileBlock *block = stack + sp;
    block->label = label;
    block->old_tsc_elapsed_inclusive = anchor->tsc_elapsed_inclusive;
    block->start_tsc = read_cpu_timer();
    block->anchor_idx = idx;
    sp++;
}

void end_block(char const *label) {
    sp--;
    ProfileBlock *block = stack + sp;
    u64 elapsed = read_cpu_timer() - block->start_tsc;

    if (sp > 1) {
        ProfileBlock *parent_block = stack + (sp-1);
        Profile *parent_anchor = profiler.profiles + parent_block->anchor_idx;
        parent_anchor->tsc_elapsed_exclusive -= elapsed;
    }

    Profile *anchor = profiler.profiles + block->anchor_idx;
    anchor->tsc_elapsed_exclusive += elapsed;
    anchor->tsc_elapsed_inclusive = block->old_tsc_elapsed_inclusive + elapsed;
    anchor->hit_count += 1;
}

void begin_profiler(void) {
    profiler.start = read_cpu_timer();
}

void end_profiler(void) {
    profiler.end = read_cpu_timer();
}

void print_time_elapsed(u64 total_tsc_elapsed, Profile *anchor) {
    f64 percent = 100.0 * ((f64)anchor->tsc_elapsed_exclusive / (f64)total_tsc_elapsed);
    printf(" %s[%lu]: %lu (%.2f%%", anchor->label, anchor->hit_count, anchor->tsc_elapsed_exclusive, percent);
    if (anchor->tsc_elapsed_inclusive != anchor->tsc_elapsed_exclusive) {
        f64 percent_with_children = 100.0 * ((f64)anchor->tsc_elapsed_inclusive / (f64)total_tsc_elapsed);
        printf(", %.2f%% with children", percent_with_children);
    }
    printf(")\n");
}

void print_profile_results(void) {
    u64 total_elapsed = profiler.end - profiler.start;
    u64 cpu_freq = estimate_cpu_timer_freq();
    fprintf(stdout, "\nTotal time: %.4fms (CPU freq %lu)\n", 1000.0 * (f64)total_elapsed / (f64)cpu_freq, cpu_freq);
    for (u32 i = 1; i < len(profiler.profiles); i++) {
        Profile *anchor = profiler.profiles + i;
        if (anchor->tsc_elapsed_inclusive) {
            print_time_elapsed(total_elapsed, anchor);
        }
    }
}


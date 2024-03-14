#ifndef PERF_AWARE_HAVERSINE_H
#include "haversine.h"
#include "haversine_clock.c"
#endif

#ifndef PROFILER
#define PROFILER 0
#endif

#if PROFILER

#define MAX_ID 32

typedef struct {
    char const *label;
    u64 tsc_elapsed_exclusive; // does NOT include children
    u64 tsc_elapsed_inclusive; // DOES include children
    u64 processed_byte_count;
    u64 hit_count;
} Profile;

typedef struct {
    char const *label;
    u64 old_tsc_elapsed_inclusive;
    u64 start_tsc;
    u32 anchor_idx;
} ProfileBlock;

static Profile global_profiles[4096];
static ProfileBlock stack[1024];
static u32 sp = 1;

#define NAME_CONCAT(a, b)          a##b
#define NAME(a, b)                 NAME_CONCAT(a,b)

#define BEGIN_BANDWIDTH_BLOCK(string, bytes)    start_block(string, __COUNTER__ + 1, bytes);
#define END_BANDWIDTH_BLOCK(string)             end_block(string);

#define BEGIN_TIME_BLOCK(string)   BEGIN_BANDWIDTH_BLOCK(string, 0)
#define END_TIME_BLOCK(string)     END_BANDWIDTH_BLOCK(string);
#define BEGIN_TIME_FUNCTION        BEGIN_TIME_BLOCK(__func__)
#define END_TIME_FUNCTION          END_TIME_BLOCK(__func__)

void start_block(char const *label, u32 idx, u64 bytes) {
    Profile *anchor = global_profiles + idx;
    anchor->label = label; // unfortunately no easy way to write this once
    anchor->processed_byte_count += bytes;

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
        Profile *parent_anchor = global_profiles + parent_block->anchor_idx;
        parent_anchor->tsc_elapsed_exclusive -= elapsed;
    }

    Profile *anchor = global_profiles + block->anchor_idx;
    anchor->tsc_elapsed_exclusive += elapsed;
    anchor->tsc_elapsed_inclusive = block->old_tsc_elapsed_inclusive + elapsed;
    anchor->hit_count += 1;
}

void print_anchor_results(u64 total_elapsed, u64 cpu_freq) {
    for (u32 i = 1; i < len(global_profiles); i++) {
        Profile *anchor = global_profiles + i;
        if (anchor->tsc_elapsed_inclusive) {
            f64 percent = 100.0 * ((f64)anchor->tsc_elapsed_exclusive / (f64)total_elapsed);
            printf(" %s[%lu]: %lu (%.2f%%", anchor->label, anchor->hit_count, anchor->tsc_elapsed_exclusive, percent);
            if (anchor->tsc_elapsed_inclusive != anchor->tsc_elapsed_exclusive) {
                f64 percent_with_children = 100.0 * ((f64)anchor->tsc_elapsed_inclusive / (f64)total_elapsed);
                printf(", %.2f%% with children", percent_with_children);
            }
            if (anchor->processed_byte_count) {
                f64 megabyte = 1024.0f * 1024.0f;
                f64 gigabyte = megabyte * 1024.0f;

                f64 seconds = (f64)anchor->tsc_elapsed_inclusive / (f64)cpu_freq;
                f64 bytes_per_second = (f64)anchor->processed_byte_count / seconds;
                f64 megabytes = (f64)anchor->processed_byte_count / (f64)megabyte;
                f64 gigabytes_per_second = bytes_per_second / gigabyte;

                printf("  %.3fmb at %.2fgb/s", megabytes, gigabytes_per_second);
            }
            printf(")\n");
        }
    }
}

#else

#define BEGIN_TIME_BLOCK(...)
#define END_TIME_BLOCK(...)
#define BEGIN_TIME_FUNCTION
#define END_TIME_FUNCTION
#define BEGIN_BANDWIDTH_BLOCK(...)
#define END_BANDWIDTH_BLOCK(...)
#define print_anchor_results(...)

#endif

typedef struct {
    u64 start;
    u64 end;
} GlobalProfiler;

static GlobalProfiler profiler;

void begin_profiler(void) {
    profiler.start = read_cpu_timer();
}

void end_and_print_profiler(void) {
    profiler.end = read_cpu_timer();

    u64 total_elapsed = profiler.end - profiler.start;
    u64 cpu_freq = estimate_cpu_timer_freq();
    fprintf(stdout, "\nTotal time: %.4fms (CPU freq %lu)\n", 1000.0 * (f64)total_elapsed / (f64)cpu_freq, cpu_freq);

    print_anchor_results(total_elapsed, cpu_freq);
}

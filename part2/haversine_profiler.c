#ifndef PERF_AWARE_HAVERSINE_H
#include "haversine.h"
#include "haversine_clock.c"
#endif

#define MAX_ID 32

typedef struct {
    char const *label;
    u64 total;
    u64 total_children;
    u64 times_hit;
    u32 nested_cnt;
} Profile;

typedef struct {
    Profile profiles[4096];
    u64 start;
    u64 end;
} GlobalProfiler;

GlobalProfiler profiler;

typedef struct {
    char const *label;
    u64 start_timestamp;
    u32 parent_idx;
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
    anchor->times_hit++;
    anchor->nested_cnt += 1;
    if (anchor->nested_cnt > 1) {
        return;
    }
    ProfileBlock *block = stack + sp;
    block->parent_idx = stack[sp-1].anchor_idx;
    block->anchor_idx = idx;
    block->label = label;
    block->start_timestamp = read_cpu_timer();
    anchor->label = label;
    sp++;
}

Profile *find_anchor(char const *label) {
    Profile *anchor;
    for (int i = 1; i < len(profiler.profiles); i++) {
        anchor = profiler.profiles + i;
        if (anchor->label != NULL && strcmp(anchor->label, label) == 0) {
            return anchor;
        }
    }
    fprintf(stderr, "ERROR: called find_anchor with invalid label\n");
    exit(1);
}

void end_block(char const *label) {
    Profile *anchor = find_anchor(label);
    anchor->nested_cnt -= 1;
    if (anchor->nested_cnt > 0) {
        return;
    }
    sp--;
    ProfileBlock *block = stack + sp;
    u64 elapsed = read_cpu_timer() - block->start_timestamp;

    if (block->parent_idx) {
        Profile *parent = profiler.profiles + block->parent_idx;
        parent->total_children += elapsed;
    }

    anchor->total += elapsed;
}

void begin_profiler(void) {
    profiler.start = read_cpu_timer();
}

void end_profiler(void) {
    profiler.end = read_cpu_timer();
}

void print_profile_results(void) {
    u64 total_elapsed = profiler.end - profiler.start;
    u64 cpu_freq = estimate_cpu_timer_freq();
    fprintf(stdout, "\nTotal time: %.4fms (CPU freq %lu)\n", 1000.0 * (f64)total_elapsed / (f64)cpu_freq, cpu_freq);

    Profile *profile;
    for (int i = 1; i < len(profiler.profiles); i++) {
        profile = profiler.profiles + i;
        if (profile->label == NULL) continue;
        u64 elapsed = profile->total - profile->total_children;
        fprintf(stdout, "\t%s[%lu]: %lu (%.2f%%", profile->label, profile->times_hit, elapsed, (f64)elapsed / (f64)total_elapsed * 100);
        if (profile->total_children) {
            f64 percent_with_children = 100.0 * ((f64)profile->total / (f64)total_elapsed);
            fprintf(stdout, ", %.2f%% w children", percent_with_children);
        }
        fprintf(stdout, ")\n");
    }
}


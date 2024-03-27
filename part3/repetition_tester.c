#include "common.h"
#include "clock.c"

typedef enum {
    TestModeUninitialized,
    TestModeTesting,
    TestModeCompleted,
    TestModeError,
} TestMode;

typedef struct {
    u64 test_count;
    u64 total_time;
    u64 max_time;
    u64 min_time;
} RepetitionTestResults;

typedef struct {
    u64 target_processed_byte_count;
    u64 cpu_timer_freq;
    u64 try_for_time;
    u64 tests_started_at;
    
    TestMode mode;
    bool print_new_minimums;
    u32 open_block_count;
    u32 close_block_count;
    u64 time_accumulated_on_this_test;
    u64 bytes_accumulated_on_this_test;

    RepetitionTestResults results;
} RepetitionTester;

static f64 seconds_from_cpu_time(f64 cpu_time, u64 cpu_timer_freq) {
    f64 result = 0.0;
    if(cpu_timer_freq) {
        result = (cpu_time / (f64)cpu_timer_freq);
    }
    
    return result;
}
 
static void print_time(char const *label, f64 cpu_time, u64 cpu_timer_freq, u64 byte_count)
{
    printf("%s: %.0f", label, cpu_time);
    if(cpu_timer_freq) {
        f64 seconds = seconds_from_cpu_time(cpu_time, cpu_timer_freq);
        printf(" (%fms)", 1000.0f*seconds);
    
        if(byte_count) {
            f64 gigabyte = (1024.0f * 1024.0f * 1024.0f);
            f64 best_bandwidth = byte_count / (gigabyte * seconds);
            printf(" %fgb/s", best_bandwidth);
        }
    }
}

static void print_results(RepetitionTestResults results, u64 cpu_timer_freq, u64 byte_count)
{
    print_time("Min", (f64)results.min_time, cpu_timer_freq, byte_count);
    printf("\n");
    
    print_time("Max", (f64)results.max_time, cpu_timer_freq, byte_count);
    printf("\n");
    
    if(results.test_count) {
        print_time("Avg", (f64)results.total_time / (f64)results.test_count, cpu_timer_freq, byte_count);
        printf("\n");
    }
}

static void error(RepetitionTester *tester, char const *message)
{
    tester->mode = TestModeError;
    fprintf(stderr, "ERROR: %s\n", message);
}

static void new_test_wave(RepetitionTester *tester, u64 target_processed_byte_count, u64 cpu_timer_freq)
{
    if(tester->mode == TestModeUninitialized)
    {
        tester->mode = TestModeTesting;
        tester->target_processed_byte_count = target_processed_byte_count;
        tester->cpu_timer_freq = cpu_timer_freq;
        tester->print_new_minimums = true;
        tester->results.min_time = (u64)-1;
    }
    else if(tester->mode == TestModeCompleted)
    {
        tester->mode = TestModeTesting;
        
        if(tester->target_processed_byte_count != target_processed_byte_count)
        {
            error(tester, "TargetProcessedbyte_count changed");
        }
        
        if(tester->cpu_timer_freq != cpu_timer_freq)
        {
            error(tester, "CPU frequency changed");
        }
    }

    tester->try_for_time = 5 * cpu_timer_freq;
    tester->tests_started_at = read_cpu_timer();
}

static void begin_time(RepetitionTester *tester)
{
    ++tester->open_block_count;
    tester->time_accumulated_on_this_test -= read_cpu_timer();
}

static void end_time(RepetitionTester *tester)
{
    ++tester->close_block_count;
    tester->time_accumulated_on_this_test += read_cpu_timer();
}

static void count_bytes(RepetitionTester *tester, u64 byte_count)
{
    tester->bytes_accumulated_on_this_test += byte_count;
}

static bool is_testing(RepetitionTester *tester)
{
    if(tester->mode == TestModeTesting)
    {
        u64 current_time = read_cpu_timer();
        
        if(tester->open_block_count) {
            if(tester->open_block_count != tester->close_block_count) {
                error(tester, "Unbalanced BeginTime/EndTime");
            }
           
            if(tester->bytes_accumulated_on_this_test != tester->target_processed_byte_count) {
                error(tester, "Processed byte count mismatch");
            }
    
            if(tester->mode == TestModeTesting) {
                RepetitionTestResults *results = &tester->results;
                u64 elapsed_time = tester->time_accumulated_on_this_test;
                results->test_count += 1;
                results->total_time += elapsed_time;
                if(results->max_time < elapsed_time) {
                    results->max_time = elapsed_time;
                }
                
                if(results->min_time > elapsed_time) {
                    results->min_time = elapsed_time;
                    tester->tests_started_at = current_time;
                    if(tester->print_new_minimums) {
                        print_time("Min", results->min_time, tester->cpu_timer_freq, tester->bytes_accumulated_on_this_test);
                        printf("               \r");
                    }
                }
                
                tester->open_block_count = 0;
                tester->close_block_count = 0;
                tester->time_accumulated_on_this_test = 0;
                tester->bytes_accumulated_on_this_test = 0;
            }
        }
        
        if((current_time - tester->tests_started_at) > tester->try_for_time)
        {
            tester->mode = TestModeCompleted;
            
            printf("                                                          \n");
            print_results(tester->results, tester->cpu_timer_freq, tester->target_processed_byte_count);
        }
    }
    
    bool result = (tester->mode == TestModeTesting);
    return result;
}

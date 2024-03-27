#include "common.h"
#include "buffer.c"
#include "clock.c"
#include "read_overhead_test.c"

#include <sys/stat.h>

typedef struct {
    char const *name;
    ReadOverheadTestFunction *func;
} TestFunction;

TestFunction test_functions[] = {
    {"fread", read_via_fread},
    {"read", read_via_read},
};

int main(int argc, char **argv) { 
    u64 cpu_timer_freq = estimate_cpu_timer_freq();
    
    if(argc == 2) {
        char *file_name = argv[1];
        struct stat status;
        stat(file_name, &status);
        
        ReadParameters params = {};
        params.dest = allocate_buffer(status.st_size);
        params.file_name = file_name;
    
        if(params.dest.count > 0) {
            RepetitionTester testers[len(test_functions)][AllocTypeCount] = {};
            
            for(;;) {
                for(u32 func_idx = 0; func_idx < len(test_functions); ++func_idx) {
                    for (u32 alloc_idx = 0; alloc_idx < AllocTypeCount; ++alloc_idx) {
                        params.alloc_type = (AllocType) alloc_idx;
                        RepetitionTester *tester = &testers[func_idx][alloc_idx];
                        TestFunction test_func = test_functions[func_idx];
                        
                        printf("\n--- %s%s%s ---\n",
                               describe_alloc_type(params.alloc_type),
                               params.alloc_type ? " + " : "",
                               test_func.name
                        );
                        new_test_wave(tester, params.dest.count, cpu_timer_freq);
                        test_func.func(tester, &params);
                    }
                }
            }
        } else {
            fprintf(stderr, "ERROR: Test data size must be non-zero\n");
        }
    } else {
        fprintf(stderr, "Usage: %s [existing filename]\n", argv[0]);
    }
		
    return 0;
}

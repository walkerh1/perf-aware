#include "common.h"
#include "buffer.c"
#include "clock.c"
#include "read_overhead_test.c"

#include <sys/stat.h>

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))


typedef struct 
{
    char const *Name;
    read_overhead_test_func *Func;
} test_function;

test_function TestFunctions[] =
{
    {"fread", ReadViaFRead},
    {"_read", ReadViaRead},
    {"ReadFile", ReadViaReadFile},
};

int main(int ArgCount, char **Args)
{
    // NOTE(casey): Since we do not use these functions in this particular build, we reference their pointers
    // here to prevent the compiler from complaining about "unused functions".
    (void)&IsInBounds;
    (void)&AreEqual;
    
    u64 CPUTimerFreq = estimate_cpu_timer_freq();
    
    if(ArgCount == 2)
    {
        char *FileName = Args[1];
        struct stat Stat;
        stat(FileName, &Stat);
        
        read_parameters Params = {};
        Params.Dest = AllocateBuffer(Stat.st_size);
        Params.FileName = FileName;
    
        if(Params.Dest.Count > 0)
        {
            repetition_tester Testers[ArrayCount(TestFunctions)] = {};
            
            for(;;)
            {
                for(u32 FuncIndex = 0; FuncIndex < ArrayCount(TestFunctions); ++FuncIndex)
                {
                    repetition_tester *Tester = Testers + FuncIndex;
                    test_function TestFunc = TestFunctions[FuncIndex];
                    
                    printf("\n--- %s ---\n", TestFunc.Name);
                    NewTestWave(Tester, Params.Dest.Count, CPUTimerFreq);
                    TestFunc.Func(Tester, &Params);
                }
            }
            
            // NOTE(casey): We would normally call this here, but we can't because the compiler will complain about "unreachable code".
            // So instead we just reference the pointer to prevent the compiler complaining about unused function :(
            (void)&FreeBuffer;
        }
        else
        {
            fprintf(stderr, "ERROR: Test data size must be non-zero\n");
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s [existing filename]\n", Args[0]);
    }
		
    return 0;
}

#include "repetition_tester.c"
#include "buffer.c"
#include "common.h"

#include <fcntl.h>
#include <unistd.h>

typedef enum {
    AllocTypeNone,
    AllocTypeMalloc,
    AllocTypeCount,
} AllocType;

typedef struct {
    AllocType alloc_type;
    Buffer dest;
    char const *file_name;
} ReadParameters;

typedef void ReadOverheadTestFunction(RepetitionTester *tester, ReadParameters *params);

static char const *describe_alloc_type(AllocType alloc_type) {
    char const *res;
    switch (alloc_type) {
        case AllocTypeNone: { res = ""; } break;
        case AllocTypeMalloc: { res = "malloc"; } break;
        default: { res = "unknown"; } break;
    }
    return res;
}

static void handle_allocation(ReadParameters *params, Buffer *buffer) {
    switch (params->alloc_type) {
        case AllocTypeNone: {} break;
        case AllocTypeMalloc: { *buffer = allocate_buffer(params->dest.count); } break;
        default: { fprintf(stderr, "ERROR: unrecognised allocation type\n"); } break;
    }
}

static void handle_deallocation(ReadParameters *params, Buffer *buffer) {
    switch (params->alloc_type) {
        case AllocTypeNone: {} break;
        case AllocTypeMalloc: { free_buffer(buffer); } break;
        default: { fprintf(stderr, "ERROR: unrecognised allocation type\n"); } break;
    }
}

static void read_via_fread(RepetitionTester *tester, ReadParameters *params) {
    while(is_testing(tester)) {
        FILE *file = fopen(params->file_name, "rb");
        if(file) {
            Buffer dest_buffer = params->dest;
            handle_allocation(params, &dest_buffer);
            
            begin_time(tester);
            size_t result = fread(dest_buffer.data, dest_buffer.count, 1, file);
            end_time(tester);
            
            if(result == 1) {
                count_bytes(tester, dest_buffer.count);
            } else {
                error(tester, "fread failed");
            }
            
            handle_deallocation(params, &dest_buffer);
            fclose(file);
        } else {
            error(tester, "fopen failed");
        }
    }
}

static void read_via_read(RepetitionTester *tester, ReadParameters *params)
{
    while(is_testing(tester))
    {
        int file = open(params->file_name, O_RDONLY);
        if(file != -1)
        {
            Buffer dest_buffer = params->dest;
            handle_allocation(params, &dest_buffer);
        
            u8 *dest = dest_buffer.data;
            u64 size_remaining = dest_buffer.count;
            while(size_remaining)
            {
                u32 read_size = INT_MAX;
                if((u64)read_size > size_remaining)
                {
                    read_size = (u32)size_remaining;
                }

                begin_time(tester);
                int result = read(file, dest, read_size);
                end_time(tester);

                if(result == (int)read_size)
                {
                    count_bytes(tester, read_size);
                }
                else
                {
                    error(tester, "read failed");
                    break;
                }
                
                size_remaining -= read_size;
                dest += read_size;
            }
            
            handle_deallocation(params, &dest_buffer);
            close(file);
        }
        else
        {
            error(tester, "open failed");
        }
    }
}


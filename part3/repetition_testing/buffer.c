#ifndef BUFFER_H
#define BUFFER_H

#include "common.h"

typedef struct {
    size_t count;
    u8 *data;
} Buffer;

#define CONSTANT_STRING(string) {sizeof(string) - 1, (u8 *)(string)}

static bool is_in_bounds(Buffer source, u64 at) {
    bool result = (at < source.count);
    return result;
}

static bool are_equal(Buffer a, Buffer b) {
    if(a.count != b.count) {
        return false;
    }
    
    for(u64 idx = 0; idx < a.count; ++idx) {
        if(a.data[idx] != b.data[idx]) {
            return false;
        }
    }
    
    return true;
}

static Buffer allocate_buffer(size_t count) {
    Buffer result = {};
    result.data = (u8 *)malloc(count);

    if(result.data) {
        result.count = count;
    } else {
        fprintf(stderr, "ERROR: Unable to allocate %lu bytes.\n", count);
    }
    
    return result;
}

static void free_buffer(Buffer *buffer) {
    if(buffer->data) {
        free(buffer->data);
    }
}

#endif

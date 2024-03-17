#ifndef BUFFER_H
#define BUFFER_H

#include "common.h"

typedef struct
{
    size_t Count;
    u8 *Data;
} buffer;

#define CONSTANT_STRING(String) {sizeof(String) - 1, (u8 *)(String)}

static bool IsInBounds(buffer Source, u64 At)
{
    bool Result = (At < Source.Count);
    return Result;
}

static bool AreEqual(buffer A, buffer B)
{
    if(A.Count != B.Count)
    {
        return false;
    }
    
    for(u64 Index = 0; Index < A.Count; ++Index)
    {
        if(A.Data[Index] != B.Data[Index])
        {
            return false;
        }
    }
    
    return true;
}

static buffer AllocateBuffer(size_t Count)
{
    buffer Result = {};
    Result.Data = (u8 *)malloc(Count);
    if(Result.Data)
    {
        Result.Count = Count;
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to allocate %lu bytes.\n", Count);
    }
    
    return Result;
}

static void FreeBuffer(buffer *Buffer)
{
    if(Buffer->Data)
    {
        free(Buffer->Data);
    }
    *Buffer = {};
}

#endif

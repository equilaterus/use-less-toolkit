#ifndef NO_INCLUDES
#include <cstring>
#include <stdlib.h>
#include <assert.h>
#include "ult_common.h"
#endif

#define ARENA_SIZE_IN_BYTES Megabytes(100)
#define LOG_SIZE_IN_BYTES Megabytes(90)

//NOTE(pipecaniza): right now the arena is not capable to free resources(linear allocator) to avoid fragmentation
struct arena
{
    uint8* MemoryArena;
    uint32 BufferSize;
};

function arena
MakeArena()
{
    arena Result = {};
    Result.MemoryArena = (uint8*)calloc(1, ARENA_SIZE_IN_BYTES);
    return(Result);
}

function uint8*
MakeLog()
{
    return (uint8*)calloc(1, LOG_SIZE_IN_BYTES);
}

function uint8*
PushToMemory(arena* Arena, void* Ptr, uint32 SizeInBytes)
{
    assert(Arena->BufferSize + SizeInBytes < ARENA_SIZE_IN_BYTES);

    uint8* BytePtr = (uint8*)Ptr;
    uint8* BaseAddress = Arena->MemoryArena + Arena->BufferSize;
    for (uint32 i = 0; i < SizeInBytes; ++i)
    {
        uint8* CurrentByte = BytePtr + i;
        Arena->MemoryArena[Arena->BufferSize++] = *CurrentByte;
    }
    return(BaseAddress);
}


function void
PushToMemoryAtLocation(arena* Arena, uint8* ArenaPointer, void* Ptr, uint32 SizeInBytes)
{
    // NOTE(pipecaniza): Check that ArenaPointer is inside the arena memory
    assert((ArenaPointer > Arena->MemoryArena && ArenaPointer < Arena->MemoryArena + ARENA_SIZE_IN_BYTES) ||
            (ArenaPointer < Arena->MemoryArena && ArenaPointer > Arena->MemoryArena + ARENA_SIZE_IN_BYTES));
    assert(Arena->BufferSize + SizeInBytes < ARENA_SIZE_IN_BYTES);

    uint8* BytePtr = (uint8*)Ptr;
    uint8* BaseAddress = ArenaPointer;
    for (uint32 i = 0; i < SizeInBytes; ++i)
    {
        uint8* CurrentByte = BytePtr + i;
        *(BaseAddress + i) = *CurrentByte;
    }
}


function uint8*
ReserveMemory(arena* Arena, uint32 SizeInBytes)
{
    assert(Arena->BufferSize + SizeInBytes < ARENA_SIZE_IN_BYTES);
    uint8* BaseAddress = Arena->MemoryArena + Arena->BufferSize;
    Arena->BufferSize += SizeInBytes;
    return(BaseAddress);
}


inline function string
AllocateString(arena* Arena, char* Data)
{
    // Add +1 to store '\0' so we can send the data directly to ImGui without additional steps
    uint8* Address = PushToMemory(Arena, Data, strlen(Data ) + 1);
    return BundleString(Address);
}

function void
ResetArena(arena* Arena) {
    memset(Arena->MemoryArena, 0, ARENA_SIZE_IN_BYTES);
    Arena->BufferSize = 0;
}

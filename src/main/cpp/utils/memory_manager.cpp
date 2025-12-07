#include "memory_manager.h"
#include <cstdlib>
#include <cstring>

void* AllocateAligned(size_t size, size_t alignment) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
    return ptr;
}

void FreeAligned(void* ptr) {
    free(ptr);
}


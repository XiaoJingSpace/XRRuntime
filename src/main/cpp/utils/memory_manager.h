#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <cstddef>

// Memory management utilities
void* AllocateAligned(size_t size, size_t alignment);
void FreeAligned(void* ptr);

#endif // MEMORY_MANAGER_H


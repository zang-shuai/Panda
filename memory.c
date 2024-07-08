//
// Created by 臧帅 on 24-7-8.
//

#include <stdlib.h>
#include "memory.h"

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }
    void *result = realloc(pointer, newSize);
//    内存分配失败
    if (result == NULL) exit(1);
    return result;
}
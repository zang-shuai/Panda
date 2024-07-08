//
// Created by 臧帅 on 24-7-8.
//

#ifndef PANDA_MEMORY_H
#define PANDA_MEMORY_H

#include "common.h"
#include "chunk.h"
#include <stdlib.h>

// 2 倍扩容，返回容量
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// 返回扩容后的新地址
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
        (type*)reallocate(pointer, sizeof(type) * (oldCount),sizeof(type) * (newCount))

// 释放旧地址
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

// 重新分配内存函数，为 0 则代表释放内存
void *reallocate(void *pointer, size_t oldSize, size_t newSize);



#endif //PANDA_MEMORY_H

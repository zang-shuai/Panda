//
// Created by 臧帅 on 24-7-8.
//

#ifndef PANDA_MEMORY_H
#define PANDA_MEMORY_H

#include "common.h"
#include "chunk.h"
#include "object.h"
#include <stdlib.h>
#include "compiler.h"

// 在堆上分配一个新数组，其大小刚好可以容纳字符串中的字符和末尾的结束符
#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

// 2 倍扩容，返回容量
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// 返回扩容后的新地址
//#define GROW_ARRAY(type, pointer, oldCount, newCount) \
//        (type*)reallocate(pointer, sizeof(type) * (oldCount),sizeof(type) * (newCount))
#define GROW_ARRAY(type, pointer, oldCount, newCount) (type*)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

// 释放旧地址
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

// 重新分配内存函数，为 0 则代表释放内存
void *reallocate(void *pointer, size_t oldSize, size_t newSize);

void markObject(Obj *object);

void markValue(Value value);

// 垃圾回收函数
void collectGarbage();

// 释放对象链表的指针
void freeObjects();


#endif //PANDA_MEMORY_H

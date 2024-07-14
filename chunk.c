//
// Created by 臧帅 on 24-7-8.
//
#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))


void initChunk(Chunk *chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->lines = NULL;
    chunk->code = NULL;
    // 初始化一个常量池
    initValueArray(&chunk->constants);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code,
                                 oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines,
                                  oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    ++chunk->count;
}

void freeChunk(Chunk *chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    // 释放常量池
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

// 将值 value 插入到常量池中，并返回该值在常量池中的位置
int addConstant(Chunk *chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}
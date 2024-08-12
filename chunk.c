//
// Created by 臧帅 on 24-7-8.
//
#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

#include "vm.h"

// 初始化 chunk
void initChunk(Chunk *chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->lines = NULL;
    chunk->code = NULL;
    // 初始化一个常量池
    initValueArray(&chunk->constants);
}

// 将byte值写入chunk
void writeChunk(Chunk *chunk, uint8_t byte, int line) {
    // 判断容量、chunk 中有 2 个数组、byte与line
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    ++chunk->count;
}

// 销毁 chunk
void freeChunk(Chunk *chunk) {
    // 释放code数组
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    // 释放数组
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    // 释放value 池
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

// 将值 value 插入到常量池中，并返回该值在value池中的位置
int addConstant(Chunk *chunk, Value value) {
    // 将值入栈出栈，垃圾回收时使用
    push(value);
    writeValueArray(&chunk->constants, value);
    pop();
    return chunk->constants.count - 1;
}
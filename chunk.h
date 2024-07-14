//
// Created by 臧帅 on 24-7-8.
//

#ifndef PANDA_CHUNK_H
#define PANDA_CHUNK_H

#include "common.h"
#include "memory.h"
#include "value.h"

//定义操作码
typedef enum {
//    常量指令
    OP_CONSTANT,
//    return 指令
    OP_RETURN,
} OpCode;

// 动态数组
typedef struct {
    // 数组数据量
    int count;
    // 数组容量，超过容量时扩容
    int capacity;
    uint8_t *code;
    // 行号（报错时使用）
    int *lines;
    ValueArray constants;
} Chunk;

//初始化动态数组
void initChunk(Chunk *chunk);

// 释放
void freeChunk(Chunk *chunk);

//写入数据

void writeChunk(Chunk *chunk, uint8_t byte, int line);

// 常量池中增加常量
int addConstant(Chunk *chunk, Value value);

#endif //PANDA_CHUNK_H

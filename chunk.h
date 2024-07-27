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
// 常量指令
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
// 一元指令，取反指令
    OP_SET_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_NEGATE,
    OP_PRINT,
//    +-*/二元指令
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
// return 指令
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

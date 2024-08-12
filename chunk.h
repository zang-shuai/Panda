//
// Created by 臧帅 on 24-7-8.
//

#ifndef PANDA_CHUNK_H
#define PANDA_CHUNK_H

#include "common.h"
//#include "memory.h"
#include "value.h"

//定义操作码
typedef enum {
    // 常量指令，向常量池加入常量,(常量值必定跟在后面)
    OP_CONSTANT,
    // 预设值
    OP_NIL,
    OP_TRUE,
    OP_FALSE,

    // 输出指令
    OP_PRINT,
    // 运算指令 +-*/二元指令
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    // 比较指令
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_NEGATE,

    //
    // 跳转操作
    // 栈顶为 false 时跳转
    OP_JUMP_IF_FALSE,
    // 向前跳转
    OP_JUMP,
    // 向后跳转
    OP_LOOP,

    // 栈指令
    OP_POP,

    // 变量指令
    // 全局变量
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    // 局部变量
    OP_SET_LOCAL,
    OP_GET_LOCAL,
    // 上值
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_CLOSE_UPVALUE,

    // 函数指令
    OP_CALL,
    OP_RETURN,
    OP_METHOD,
    OP_CLOSURE,


    // 对象指令
    // 继承
    OP_CLASS,
    OP_INHERIT,
    OP_INVOKE,
    // 属性
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    // 继承
    OP_GET_SUPER,
    OP_SUPER_INVOKE,
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
    // 常量池
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

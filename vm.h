//
// Created by 臧帅 on 24-7-9.
//

#ifndef PANDA_VM_H
#define PANDA_VM_H

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256
// 虚拟机
typedef struct {
    // 指向指令集
    Chunk *chunk;
    // 指向当前指令的位置（指令指针）
    uint8_t *ip;
    // 虚拟机栈
    Value stack[STACK_MAX];
    Value *stackTop;
    // 对象链
    Obj* objects;
} VM;

// 解释结果
typedef enum {
    // 解释成功
    INTERPRET_OK,
    // 编译错误
    INTERPRET_COMPILE_ERROR,
    // 运行错误
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();

void freeVM();

InterpretResult interpret(const char *source);


void push(Value value);

Value pop();


#endif //PANDA_VM_H

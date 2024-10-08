//
// Created by 臧帅 on 24-7-9.
//

#ifndef PANDA_VM_H
#define PANDA_VM_H

#include "chunk.h"
//#include "value.h"
#include "object.h"
//#include "table.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
// 函数调用
typedef struct {
    // 指向函数对象的指针
    ObjClosure *closure;
    // chunk 指向的位置
    uint8_t *ip;
    // 数组，里面存储局部变量（全局变量在常量池中）
    Value *slots;
} CallFrame;

// 虚拟机
typedef struct {
    // 指向指令集
    Chunk *chunk;
    // 指向当前指令的位置（指令指针）
    uint8_t *ip;
    // 函数调用帧、每个函数有一个
    CallFrame frames[FRAMES_MAX];
    // 帧数量，（函数数量）
    int frameCount;

    // 虚拟机栈
    Value stack[STACK_MAX];
    Value *stackTop;
    // 全局变量 hash 表
    Table globals;
    // 字符串 hash 表
    Table strings;
    ObjString* initString;
    //
    ObjUpvalue *openUpvalues;
    // GC
    size_t bytesAllocated;
    size_t nextGC;
    // 对象链
    Obj *objects;

    // 动态数组，表示灰色栈的数量
    int grayCount;
    int grayCapacity;
    Obj **grayStack;
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

//
// Created by 臧帅 on 24-7-9.
//

#include "vm.h"
#include "common.h"
#include <stdio.h>

#include <stdarg.h>
#include "compiler.h"
#include "debug.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

static void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM() {
    resetStack();
}

void freeVM() {
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

// 判断该 value 是否为 nil 或者 false
static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

// 运行指令集，返回解释结果
static InterpretResult run() {

// 返回当前chunk 值，并将指针后移一位
#define READ_BYTE() (*vm.ip++)
// 读取常量指令，返回读取到的常量
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
// 二元指令操作宏，从栈中取出 2 个操作符号，进行二元运算
#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtimeError("Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
    } while (false)

    for (;;) {
//        如果是调试模式，则输出 chunk 中的各种信息
#ifdef DEBUG_TRACE_EXECUTION
        // 输出当前块的信息

        disassembleInstruction(vm.chunk,
                               (int) (vm.ip - vm.chunk->code));
        // 栈跟踪代码，输出此时虚拟机栈的各种信息
        printf("          ");

        for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");

        }
        printf("\n");
#endif
        uint8_t instruction;


        switch (instruction = READ_BYTE()) {
            // 如果当前指令为 OP_CONSTANT ，则读取常量（位于下一个chunk 块），并将读取的常量返回，再输出一个空行
            case OP_CONSTANT: {
                // 读取常量
                Value constant = READ_CONSTANT();
                // 将常量加入到栈中
                push(constant);
                // 打印该常量
                break;
            }
            case OP_NIL:
                push(NIL_VAL);
                break;
            case OP_TRUE:
                push(BOOL_VAL(true));
                break;
            case OP_FALSE:
                push(BOOL_VAL(false));
                break;

            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }

            case OP_GREATER:
                BINARY_OP(BOOL_VAL, >);
                break;
            case OP_LESS:
                BINARY_OP(BOOL_VAL, <);
                break;


            case OP_ADD:
                BINARY_OP(NUMBER_VAL, +);
                break;
            case OP_SUBTRACT:
                BINARY_OP(NUMBER_VAL, -);
                break;
            case OP_MULTIPLY:
                BINARY_OP(NUMBER_VAL, *);
                break;
            case OP_DIVIDE:
                BINARY_OP(NUMBER_VAL, /);
                break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;

            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
//                    runtimeError("操作数必须是一个数字。");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;

                // 如果当前指令为 return，则直接返回 OK，解释成功
            case OP_RETURN: {
//                printValue(pop());
//                printf("\n");
                return INTERPRET_OK;
            }
        }
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

//
InterpretResult interpret(const char *source) {
    Chunk chunk;
    initChunk(&chunk);
// 编译 source文件，编译结果放入chunk中
    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    // 将 chunk 交予虚拟机
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    // 获取程序的解释结果
    InterpretResult result = run();

    // 释放 chunk
    freeChunk(&chunk);
    // 返回结果
    return result;
}

void push(Value value) {
    printf("%d",vm.stackTop->type);
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}
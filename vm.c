//
// Created by 臧帅 on 24-7-9.
//

#include "vm.h"
#include "common.h"
#include <stdio.h>

#include "compiler.h"
#include "debug.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

void initVM() {
    resetStack();
}

void freeVM() {
}

// 运行指令集，返回解释结果
static InterpretResult run() {
// 返回当前chunk 值，并将指针后移一位
#define READ_BYTE() (*vm.ip++)
// 读取常量指令，返回读取到的常量
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
// 二元指令操作宏，从栈中取出 2 个操作符号，进行二元运算
#define BINARY_OP(op) \
    do { \
      double b = pop(); \
      double a = pop(); \
      push(a op b); \
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
                printValue(constant);
                printf("\n");
                break;
            }
                // 处理取反指令
            case OP_NEGATE:
                push(-pop());
                break;

            // 二元指令
            case OP_ADD:      BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE:   BINARY_OP(/); break;



            // 如果当前指令为 return，则直接返回 OK，解释成功
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

//
InterpretResult interpret(const char* source) {
    compile(source);
    return INTERPRET_OK;
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}
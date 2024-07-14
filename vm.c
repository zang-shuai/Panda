//
// Created by 臧帅 on 24-7-9.
//

#include "vm.h"
#include "common.h"
#include <stdio.h>

#include "debug.h"

VM vm;

void initVM() {
}

void freeVM() {
}

// 运行指令集，返回解释结果
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

    for (;;) {
//        如果是调试模式，则输出 chunk 中的各种信息
#ifdef DEBUG_TRACE_EXECUTION
        disassembleInstruction(vm.chunk,
                               (int) (vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            // 如果当前指令为 OP_CONSTANT ，则读取常量（位于下一个chunk 块），并将读取的常量返回，再输出一个空行
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                printValue(constant);
                printf("\n");
                break;
            }
                // 如果当前指令为 return，则直接返回 OK，解释成功
            case OP_RETURN: {
                return INTERPRET_OK;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
}

//
InterpretResult interpret(Chunk *chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}


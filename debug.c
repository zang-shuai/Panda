//
// Created by 臧帅 on 24-7-8.
//
// 该模块用于输出 chunk 内容，便于检查程序
#include "debug.h"
#include "value.h"
#include <stdio.h>

static int simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}


// 遍历输出整个chunk
void disassembleChunk(Chunk *chunk, const char *name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}


static int constantInstruction(const char *name, Chunk *chunk,
                               int offset) {
    uint8_t constant = chunk->code[offset + 1];
    // 输出指令名，和该指令所对应的常量在常量池中的位置
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

// 输出该 chunk 块的具体情况
int disassembleInstruction(Chunk *chunk, int offset) {
    printf("%04d ", offset);
    // 如果当前值的行号与上一个相同，则输出|，否则返回行号
    if (offset > 0 &&
        chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }
    uint8_t instruction = chunk->code[offset];
    // 通过 switch 方法返回当前指令与当前指令关联的下一个或多个指令，
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

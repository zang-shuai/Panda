//
// Created by 臧帅 on 24-7-8.
//
// 该模块用于输出 chunk 内容，便于检查程序
#include "debug.h"
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


// 输出该 chunk 块的具体情况
int disassembleInstruction(Chunk *chunk, int offset) {
    printf("%04d ", offset);

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

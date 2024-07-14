//
// Created by 臧帅 on 24-7-14.
//

#ifndef PANDA_COMPILER_H
#define PANDA_COMPILER_H
// 将文件转为chunk
#include "chunk.h"

bool compile(const char* source, Chunk* chunk);

#endif //PANDA_COMPILER_H

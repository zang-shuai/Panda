//
// Created by 臧帅 on 24-7-14.
//

#ifndef PANDA_COMPILER_H
#define PANDA_COMPILER_H
// 将文件转为chunk
#include "object.h"
//#include "chunk.h"
#include "vm.h"

ObjFunction *compile(const char *source);

void markCompilerRoots();

#endif //PANDA_COMPILER_H

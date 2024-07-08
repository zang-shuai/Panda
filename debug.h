//
// Created by 臧帅 on 24-7-8.
//

#ifndef PANDA_DEBUG_H
#define PANDA_DEBUG_H

#include "chunk.h"


void disassembleChunk(Chunk *chunk, const char *name);

int disassembleInstruction(Chunk *chunk, int offset);


#endif //PANDA_DEBUG_H

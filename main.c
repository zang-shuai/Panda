#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void repl() {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}
// 读取输入进来的文件
#include <stdio.h>
#include <stdlib.h>

// 静态函数 readFile，读取指定路径的文件内容并返回一个指向该内容的指针
static char *readFile(const char *path) {
    // 打开文件，以二进制读模式 ("rb")
    FILE *file = fopen(path, "rb");
    // 检查文件是否成功打开，如果未成功打开则输出错误信息并退出程序
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    // 移动文件指针到文件末尾
    fseek(file, 0L, SEEK_END);
    // 获取文件大小
    size_t fileSize = ftell(file);
    // 重置文件指针到文件开头
    rewind(file);
    // 为文件内容分配内存，+1 是为了存储结尾的 '\0' 字符
    char *buffer = (char *) malloc(fileSize + 1);
    // 检查内存是否成功分配，如果未成功分配则输出错误信息并退出程序
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    // 读取文件内容到分配的内存中
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    // 检查读取的字节数是否等于文件大小，如果不等则输出错误信息并退出程序
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    // 在读取的内容后添加 '\0' 以确保字符串的结尾
    buffer[bytesRead] = '\0';
    // 关闭文件
    fclose(file);
    // 返回指向读取内容的指针
    return buffer;
}


static void runFile(const char *path) {
    // 读取文件
    char *source = readFile(path);
    // 解释该文件
    InterpretResult result = interpret(source);
    // 释放文件内存
    free(source);
    // 编译错误
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    // 解释错误
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char *argv[]) {
//    chunk 与 vm 测试
//    initVM();
//    Chunk chunk;
//    initChunk(&chunk);
////    定义常量指令
//    int constant = addConstant(&chunk, 1.2);
//    writeChunk(&chunk, OP_CONSTANT, 123);
//    writeChunk(&chunk, constant, 123);
//
////    定义常量指令
//    constant = addConstant(&chunk, 3.4);
//    writeChunk(&chunk, OP_CONSTANT, 123);
//    writeChunk(&chunk, constant, 123);
////    加
//    writeChunk(&chunk, OP_ADD, 123);
////    定义常量
//    constant = addConstant(&chunk, 5.6);
//    writeChunk(&chunk, OP_CONSTANT, 123);
//    writeChunk(&chunk, constant, 123);
////    除
//    writeChunk(&chunk, OP_DIVIDE, 123);
////    取反
//    writeChunk(&chunk, OP_NEGATE, 123);
////    返回
//    writeChunk(&chunk, OP_RETURN, 123);
//
//    disassembleChunk(&chunk, "test chunk");
//    interpret(&chunk);
//    freeVM();
//    freeChunk(&chunk);


    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM();
    return 0;
}
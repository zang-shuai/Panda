//
// Created by 臧帅 on 24-7-9.
//

#include "vm.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "object.h"
#include "memory.h"
#include "compiler.h"
#include "debug.h"

VM vm;

// 栈顶指针指向数组底
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

// 初始化虚拟机（初始化栈）
void initVM() {
    // 初试栈为空
    resetStack();
    // 对象链初试为空
    vm.objects = NULL;
    initTable(&vm.globals);
    // 初始化 hash 表
    initTable(&vm.strings);
}

void freeVM() {
    // 释放 hash 表
    freeTable(&vm.strings);
    // 释放对象链
    freeObjects();
    freeTable(&vm.globals);
}

// 获取当前的 Value ？？？？？？？？？
static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

// 判断该 value 是否为 nil 或者 false，返回 bool
static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

// 连接函数，连接两个字符串
static void concatenate() {
    // 从栈顶弹出 2 个值
    ObjString *b = AS_STRING(pop());
    ObjString *a = AS_STRING(pop());
    // 计算总长度
    int length = a->length + b->length;
    // 重新分配内存
    char *chars = ALLOCATE(char, length + 1);
    // 拷贝值
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';
    // 获取新的 string 对象
    ObjString *result = takeString(chars, length);
    // 将新的 C字符串转为 panda 值
    push(OBJ_VAL(result));
}

// 运行指令集，返回解释结果
static InterpretResult run() {

// 返回当前chunk 值，并将指针后移一位
#define READ_BYTE() (*vm.ip++)
// 读取常量指令，返回读取到的常量
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
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
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString *name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_POP:
                pop();
                break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm.stack[slot]);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString *name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString *name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

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


            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError(
                            "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
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
            case OP_PRINT: {
                printValue(pop());
                printf("\n");
                break;
            }

            case OP_RETURN: {
//                printValue(pop());
//                printf("\n");
                return INTERPRET_OK;
            }
        }

    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
#undef READ_STRING
}

// 启动解释器
InterpretResult interpret(const char *source) {
    // 为解释器赋予 chunk
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
    printf("%d", vm.stackTop->type);
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}
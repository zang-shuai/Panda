//
// Created by 臧帅 on 24-7-9.
//

#include "vm.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "object.h"
#include "memory.h"
#include "compiler.h"
#include "debug.h"

VM vm;

static Value clockNative(int argCount, Value *args) {
    return NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
}

// 栈顶指针指向数组底
static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
}


static void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ",
                function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
    resetStack();
}

static void defineNative(const char *name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int) strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
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
    defineNative("clock", clockNative);
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

static bool call(ObjFunction *function, int argCount) {
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.（没有得到期望数量的参数）", function->arity, argCount);
        return false;
    }
    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.（栈溢出/函数调用过多）");
        return false;
    }
    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_FUNCTION:
                return call(AS_FUNCTION(callee), argCount);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            default:
                break; // Non-callable object type.
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
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
    CallFrame *frame = &vm.frames[vm.frameCount - 1];
// 返回当前chunk 值，并将指针后移一位
#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() (frame->ip += 2,(uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
// 读取常量指令，返回读取到的常量
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])


// 它从字节码块中抽取接下来的两个字节，并从中构建出一个16位无符号整数。
#define READ_STRING() AS_STRING(READ_CONSTANT())
// 二元指令操作宏，从栈中取出 2 个操作符号，进行二元运算
#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtimeError("Operands must be numbers.（比较的值必须是字符串）"); \
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

        disassembleInstruction(&frame->function->chunk,
                               (int) (frame->ip - frame->function->chunk.code));

        // 栈跟踪代码，输出此时虚拟机栈的各种信息
        printf("          ");

        for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");

        }

        printf("\n");
        // 函数信息
        disassembleInstruction(&frame->function->chunk,
                               (int) (frame->ip - frame->function->chunk.code));
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
                // 将 nil 值加入到栈中
            case OP_NIL:
                push(NIL_VAL);
                break;
                // 将 true 值加入到栈中
            case OP_TRUE:
                push(BOOL_VAL(true));
                break;
                // 将 false 值加入到栈中
            case OP_FALSE:
                push(BOOL_VAL(false));
                break;
                // 读取局部变量，加入到栈中
            case OP_GET_LOCAL: {
                // (*frame->ip++)
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
                // 设置变量值，将栈顶值加入到 value 池中的合适的位置
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
                // 从 hash 表中查找全局变量，并插入
            case OP_GET_GLOBAL: {
                // 读取字符串，字符串为全局变量名
                ObjString *name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.（没有定义该全局变量）", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_POP:
                pop();
                break;
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
                    runtimeError("Undefined variable '%s'.（没有定义该全局变量）", name->chars);
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
            // 向前跳转
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            // 为 false 则跳转
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) frame->ip += offset;
                break;
            }
            // 反向跳转
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            // 调用函数
            case OP_CALL: {
                // 获取参数数量
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            //
            case OP_RETURN: {
                Value result = pop();
                // 函数减一
                vm.frameCount--;

                // 如果函数减完了，说明程序结束
                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                // 程序没有结束，将返回值插入栈中，修改当前帧
                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
//                return INTERPRET_OK;
            }
        }

    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
#undef READ_STRING
#undef READ_SHORT
}

// 启动解释器
InterpretResult interpret(const char *source) {
    // 编译该文件，并返回编译完后的函数对象
    ObjFunction *function = compile(source);
//    exit(0);
    if (function == NULL)
        return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));

    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stack;

    call(function, 0);
    return run();
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
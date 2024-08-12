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

static Value clockNative(int argCount, Value *args);

// 栈顶指针指向数组底
static void resetStack() ;


static void runtimeError(const char *format, ...) ;

static void defineNative(const char *name, NativeFn function);

// 初始化虚拟机（初始化栈）
void initVM();

void freeVM();

// 获取当前的 Value ？？？？？？？？？
static Value peek(int distance);

static bool call(ObjClosure *closure, int argCount);


static bool callValue(Value callee, int argCount);
// 初始化器
static bool invokeFromClass(ObjClass *klass, ObjString *name, int argCount);

static bool invoke(ObjString *name, int argCount);

static bool bindMethod(ObjClass *klass, ObjString *name);

static ObjUpvalue *captureUpvalue(Value *local);

static void closeUpvalues(Value *last) ;

static void defineMethod(ObjString *name);

// 判断该 value 是否为 nil 或者 false，返回 bool
static bool isFalsey(Value value) ;

// 连接函数，连接两个字符串
static void concatenate();

// 运行指令集，返回解释结果
static InterpretResult run() ;

// 启动解释器
InterpretResult interpret(const char *source) ;

void push(Value value) ;

Value pop();
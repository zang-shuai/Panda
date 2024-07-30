//
// Created by 臧帅 on 24-7-15.
//

#ifndef PANDA_OBJECT_H
#define PANDA_OBJECT_H

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

// 返回对象类型
#define OBJ_TYPE(value)        (AS_OBJ(value)->type)
// 返回是否为字符串
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
// 返回该值是否为函数
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)

// 返回ObjString*
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
// 返回字符数组本身
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)
// 返回函数对象
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))

#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)
// 对象类型
typedef enum {
    // 字符串
    OBJ_STRING,
    // 本地调用
    OBJ_NATIVE,
    // 函数
    OBJ_FUNCTION,
} ObjType;

//
struct Obj {
    ObjType type;
    struct Obj *next;
};

// 函数结构体
typedef struct {
    Obj obj;
//    函数所需要的参数数量
    int arity;
    Chunk chunk;
    ObjString *name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;
struct ObjString {
    Obj obj;
    int length;
    char *chars;
    // 字符串的哈希值，用于在 hash 表中查找
    uint32_t hash;
};


ObjFunction *newFunction();

ObjNative *newNative(NativeFn function);

// C 语言字符串转为 panda 字符串
ObjString *takeString(char *chars, int length);

// 拷贝字符串
ObjString *copyString(const char *chars, int length);

void printObject(Value value);

// 判断 value的类型是不是type
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //PANDA_OBJECT_H

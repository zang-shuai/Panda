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
// 返回是否为闭包
#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)

#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)

#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
// 返回ObjString*
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
// 返回字符数组本身
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)
// 返回函数对象
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))

#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))

#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))

#define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)
// value 转为闭包对象
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
// 对象类型
typedef enum {
    // 对象方法
    OBJ_BOUND_METHOD,
    //
    OBJ_CLASS,
    // 闭包
    OBJ_CLOSURE,
    // 函数
    OBJ_FUNCTION,
    // 实例
    OBJ_INSTANCE,
    // 本地调用
    OBJ_NATIVE,
    // 字符串
    OBJ_STRING,
    // 闭包外值
    OBJ_UPVALUE
} ObjType;

//对象
struct Obj {
    ObjType type;
    // 垃圾回收
    bool isMarked;
    struct Obj *next;
};

// 函数结构体
typedef struct {
    Obj obj;
//    函数所需要的参数数量
    int arity;
    // 闭包外值数量
    int upvalueCount;
    // 函数内容
    Chunk chunk;
    // 函数名
    ObjString *name;
} ObjFunction;

// 定义了一个函数指针，返回值为 Value
typedef Value (*NativeFn)(int argCount, Value *args);

// 本地方法
typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

// 字符串
struct ObjString {
    Obj obj;
    int length;
    char *chars;
    // 字符串的哈希值，用于在 hash 表中查找
    uint32_t hash;
};

// 闭包上值
typedef struct ObjUpvalue {
    Obj obj;
    Value *location;
    Value closed;
    struct ObjUpvalue *next;
} ObjUpvalue;

// 闭包
typedef struct {
    Obj obj;
    ObjFunction *function;
    ObjUpvalue **upvalues;
    int upvalueCount;
} ObjClosure;

// 类
typedef struct {
    Obj obj;
    ObjString *name;
    Table methods;
} ObjClass;

// 实例
typedef struct {
    Obj obj;
    ObjClass *klass;
    Table fields;
} ObjInstance;

// 方法和初始化器
typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure *method;
} ObjBoundMethod;

ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method);

ObjClass *newClass(ObjString *name);

ObjClosure *newClosure(ObjFunction *function);

ObjFunction *newFunction();

ObjInstance *newInstance(ObjClass *klass);

ObjNative *newNative(NativeFn function);


// C 语言字符串转为 panda 字符串
ObjString *takeString(char *chars, int length);

// 拷贝字符串
ObjString *copyString(const char *chars, int length);

ObjUpvalue *newUpvalue(Value *slot);

void printObject(Value value);

// 判断 value的类型是不是type
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //PANDA_OBJECT_H

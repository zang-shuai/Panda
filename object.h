//
// Created by 臧帅 on 24-7-15.
//

#ifndef PANDA_OBJECT_H
#define PANDA_OBJECT_H

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
// 返回ObjString*
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
// 返回字符数组本身
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_STRING,
} ObjType;


struct Obj {
    ObjType type;
    struct Obj* next;
};

struct ObjString {
    Obj obj;
    int length;
    char *chars;
    // 字符串的哈希值，用于在 hash 表中查找
    uint32_t hash;
};
// C 语言字符串转为 panda 字符串
ObjString* takeString(char* chars, int length);

// 拷贝字符串
ObjString* copyString(const char* chars, int length);

void printObject(Value value);

// 判断 value的类型是不是type
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //PANDA_OBJECT_H

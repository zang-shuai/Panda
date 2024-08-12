//
// Created by 臧帅 on 24-7-8.
//

#include "value.h"
#include <stdio.h>
#include <string.h>
#include "object.h"

#include "memory.h"

void initValueArray(ValueArray *array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}


void writeValueArray(ValueArray *array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        // 获取扩容后的容量
        array->capacity = GROW_CAPACITY(oldCapacity);
        // 真实扩容，扩容的具体过程，由 C 语言内部完成
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }
    // 插入新值
    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray *array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

// 输出常量池中的数据的值，其中要将 value 值转为 C 语言的值（debug 时使用）
void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER:
            printf("%g", AS_NUMBER(value));
            break;
        case VAL_OBJ:
            printObject(value);
            break;
    }
}
// 判断两个 Value 值是否相等
bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
            // 对象判断两个 Value 是否指针相等
        case VAL_OBJ:
            return AS_OBJ(a) == AS_OBJ(b);
        default:
            return false; // Unreachable.
    }
}
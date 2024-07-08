//
// Created by 臧帅 on 24-7-8.
// 存储常量值

#ifndef PANDA_VALUE_H
#define PANDA_VALUE_H


typedef double Value;
// 常量池
typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

void initValueArray(ValueArray *array);

void writeValueArray(ValueArray *array, Value value);


void freeValueArray(ValueArray *array);

void printValue(Value value);

#endif //PANDA_VALUE_H

//
// Created by 臧帅 on 24-7-15.
//

#ifndef PANDA_TABLE_H
#define PANDA_TABLE_H

#include "common.h"
#include "value.h"

typedef struct {
    ObjString *key;
    Value value;
} Entry;


typedef struct {
    int count;
    int capacity;
    Entry *entries;
} Table;

void initTable(Table *table);

void freeTable(Table *table);
// 插入值
bool tableSet(Table *table, ObjString *key, Value value);
// 哈希表的所有条目复制到另一个哈希表中
void tableAddAll(Table *from, Table *to);
ObjString* tableFindString(Table* table, const char* chars,
                           int length, uint32_t hash);
// 检索值
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableDelete(Table* table, ObjString* key);
#endif //PANDA_TABLE_H

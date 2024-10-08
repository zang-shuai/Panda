//
// Created by 臧帅 on 24-7-15.
//

#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

// 初始化 hash 表
void initTable(Table *table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

// 销毁 hash 表
void freeTable(Table *table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

// 查询要找的 entry，传入为：初始 entry，容量大小，要查找到的 key
static Entry *findEntry(Entry *entries, int capacity,
                        ObjString *key) {
    // 获取到该值在哪个索引中
    uint32_t index = key->hash % capacity;
    //
    Entry *tombstone = NULL;

    for (;;) {
        // 获取该索引
        Entry *entry = &entries[index];
        // 正确处理墓碑（）
        if (entry->key == NULL) {
            // key 为 NULL，值为 NIL，说明这不是墓碑，就是空的，墓碑为空返回 NIL，否则返回上一个空墓碑（用于插入使用）
            if (IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            }
                // key 为 NULL，值为 BOOL_TRUE，说明这是个墓碑，继续寻找，tombstone标记这个墓碑
            else {
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == key) {
            // 如果找到了 key 就返回
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

// 调整 hash 容量
static void adjustCapacity(Table *table, int capacity) {
    // 在堆上分配一个新数组，然后转为Entry类型，长度为capacity
    Entry *entries = ALLOCATE(Entry, capacity);
    // 设置初始值
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }
    table->count = 0;
    // 将原始 hash 表中的值插入到新的表中
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        // 不移植墓碑
        if (entry->key == NULL) continue;
        Entry *dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }
    // 销毁原表
    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

// 将给定的键/值对添加到给定的哈希表中。如果该键的条目已存在，新值将覆盖旧值。如果添加了新条目，则该函数返回`true`。
bool tableSet(Table *table, ObjString *key, Value value) {
    // 检测是否需要调整容量
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }
    // 查找键值对
    Entry *entry = findEntry(table->entries, table->capacity, key);
    // 是否找到值
    bool isNewKey = entry->key == NULL;
    // 如果没有找到值，但是找到了相关键，则插入
    if (isNewKey && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

// 哈希表的所有条目复制到另一个哈希表中
void tableAddAll(Table *from, Table *to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry *entry = &from->entries[i];
        if (entry->key != NULL) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

// 查找表中是否存有该字符串
ObjString *tableFindString(Table *table, const char *chars,
                           int length, uint32_t hash) {
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry *entry = &table->entries[index];
        if (entry->key == NULL) {
            // 遇到墓碑则返回空
            if (IS_NIL(entry->value)) return NULL;
        } else if (entry->key->length == length &&
                   entry->key->hash == hash &&
                   memcmp(entry->key->chars, chars, length) == 0) {
            // 找到字符串
            return entry->key;
        }

        index = (index + 1) % table->capacity;
    }
}


// 垃圾回收时使用，删除这个表中，没有被标记删除的元素
void tableRemoveWhite(Table *table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key != NULL && !entry->key->obj.isMarked) {
            tableDelete(table, entry->key);
        }
    }
}

// 遍历，分别标记里面的键和值
void markTable(Table *table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        markObject((Obj *) entry->key);
        markValue(entry->value);
    }
}
// 从 hash 查询值，将查到的值存入 value 中
bool tableGet(Table *table, ObjString *key, Value *value) {
    // 如果表为空，则返回 false
    if (table->count == 0) return false;
    // 找到目前是否已经存在这个 key
    Entry *entry = findEntry(table->entries, table->capacity, key);

    // 如果没有找到，则返回错误
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}
// 删除表中的 key 元素
bool tableDelete(Table *table, ObjString *key) {
    if (table->count == 0) return false;

    // 找到这个 entry
    Entry *entry = findEntry(table->entries, table->capacity, key);
    // 如果没有找到则返回 false
    if (entry->key == NULL) return false;

    // 将该值替换为一个墓碑
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}
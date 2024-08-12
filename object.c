//
// Created by 臧帅 on 24-7-15.
//

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
//#include "table.h"

#define ALLOCATE_OBJ(type, objectType)  (type*)allocateObject(sizeof(type), objectType)

// 输入对象大小、对象类型。分配一个对象，并传回对象指针（？？？）
static Obj *allocateObject(size_t size, ObjType type) {
    //
    Obj *object = (Obj *) reallocate(NULL, 0, size);
    object->type = type;
    // 标记为 true 才可进行垃圾回收
    object->isMarked = false;
    // 头插法，将新建的对象插入到虚拟机的对象链中
    object->next = vm.objects;
    vm.objects = object;
#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void *) object, size, type);
#endif
    return object;
}

// （？？？）
ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method) {
    //
    ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    //
    bound->receiver = receiver;
    //
    bound->method = method;
    return bound;
}

// 新建一个类
ObjClass *newClass(ObjString *name) {
    ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    initTable(&klass->methods);
    return klass;
}

// 分配一个字符串内存
static ObjString *allocateString(char *chars, int length,
                                 uint32_t hash) {
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    // 将其 push 入虚拟机栈中（垃圾回收时可能会用到，？？？）
    push(OBJ_VAL(string));
    // 键为字符串，值为 NIL（不关心值是多少）
    tableSet(&vm.strings, string, NIL_VAL);

    pop();
    return string;
}

// 新建闭包
ObjClosure *newClosure(ObjFunction *function) {
    // 开辟一个上值数组，传入上值数量
    ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    // 上值数组先为空
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }
    ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

// 新建一个函数
ObjFunction *newFunction() {
    // 分配一个新的函数内存
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    // 参数数量为 0
    function->arity = 0;
    function->upvalueCount = 0;
    // 无名
    function->name = NULL;
    // 初始化
    initChunk(&function->chunk);
    // 返回这个新的函数
    return function;
}

// 输入一个类  新建一个对象
ObjInstance *newInstance(ObjClass *klass) {
    // 开辟一个对象内存
    ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    // 指向该对象的类
    instance->klass = klass;
    // 初始化属性表
    initTable(&instance->fields);
    return instance;
}

// 新建一个本地函数？？？？
ObjNative *newNative(NativeFn function) {
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

// 输入字符数组和长度、计算 hash 值
static uint32_t hashString(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

// 拷贝字符串、如果传入的字符串存在，则不重新分配内存，而是用原来的副本
ObjString *copyString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length,
                                          hash);
    if (interned != NULL) return interned;
    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

// 输入函数对象指针，输出函数名称
static void printFunction(ObjFunction *function) {
    // 函数名为空，说明是主函数
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    // 输出函数名
    printf("<fn %s>", function->name->chars);
}

// 将值转为一个上值对象
ObjUpvalue *newUpvalue(Value *slot) {
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    // 为空，表示先不关闭
    upvalue->closed = NIL_VAL;
    // 指向原本值的位置
    upvalue->location = slot;
    // 初始化链表的下一个值为空
    upvalue->next = NULL;
    return upvalue;
}
// 输出对象名称
void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_CLASS:
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        case OBJ_BOUND_METHOD:
            printFunction(AS_BOUND_METHOD(value)->method->function);
            break;
            // 输出闭包对象
        case OBJ_CLOSURE:
            printFunction(AS_CLOSURE(value)->function);
            break;
        case OBJ_FUNCTION:
            printFunction(AS_FUNCTION(value));
            break;
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_UPVALUE:
            printf("upvalue");
            break;
        case OBJ_INSTANCE:
            printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
    }
}

// 将传入的C 语言字符串转为字符串对象，与其他的对象直接宏转不同、这个需要插入到 hash 表中
ObjString *takeString(char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    // 如果找到了，在返回它之前，我们释放传入的字符串的内存。因为所有权被传递给了这个函数，我们不再需要这个重复的字符串，所以由我们释放它。
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocateString(chars, length, hash);
}

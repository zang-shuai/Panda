//
// Created by 臧帅 on 24-7-14.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "scanner.h"

//#include <stdbool.h>
//#include <stdio.h>
//#include <string.h>
//#include "compiler.h"
//#include "scanner.h"
//#include "chunk.h"

#ifdef DEBUG_PRINT_CODE

#include "debug.h"

#endif
// 转换器，将 token 转为表达式，
typedef struct {
    // 指向当前转换的内容
    Token current;
    // 指向前一个已经转换的内容
    Token previous;
    // 是否有错误
    bool hadError;
    // 代码出现错误，进入panic模式
    bool panicMode;
} Parser;
// 优先排序，越往下，优先级越高，每个符号代表不止一个优先级
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

// 函数指针，用于指向要执行的函数（unary、binary、grouping）
typedef void (*ParseFn)(bool canAssign);

// 规则结构体、表示各种标识符的规则
typedef struct {
    // 前缀规则函数
    ParseFn prefix;
    // 中缀规则函数
    ParseFn infix;
    // 优先级
    Precedence precedence;
} ParseRule;

// 局部变量结构体
typedef struct {
    // 记录这个局部变量名
    Token name;
    // 变量深度、同一深度的变量在同一个代码块中
    int depth;
    bool isCaptured;

} Local;

// 函数类型，区分它在编译顶层代码还是函数主体
typedef enum {
    // 普通函数
    TYPE_FUNCTION,
    // 构造函数
    TYPE_INITIALIZER,
    // 方法
    TYPE_METHOD,
    // 主函数
    TYPE_SCRIPT
} FunctionType;

// 局部变量池，记录同一深度下的所有局部变量

typedef struct Compiler {
    struct Compiler *enclosing;
    // 函数数组
    ObjFunction *function;
    // 函数类型
    FunctionType type;
    Local locals[UINT8_COUNT];
    // 局部变量数量
    int localCount;
    // 局部变量深度范围
    int scopeDepth;
} Compiler;

// 进入、开始编译代码块函数
static void beginScope();

// 退出代码块函数
static void endScope();

Parser parser;
// 默认初试状态的指针为 1
Compiler *current = NULL;


//Chunk *compilingChunk;

// 返回当前的 chunk 指针
static Chunk *currentChunk() {
    return &current->function->chunk;
}


// 编译错误处理过程，输出错误信息
static void errorAt(Token *token, const char *message) {
    // 如果处于 panic 模式，则直接返回，无需后序处理
    if (parser.panicMode) return;
    // 进入panic 模式
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

// 返回previous的错误信息
static void error(const char *message) {
    errorAt(&parser.previous, message);
}

// 返回current的错误信息
static void errorAtCurrent(const char *message) {
    errorAt(&parser.current, message);
}

// 扫描到一个 token，成功后，将扫描到的 token 交予parser.current
static void advance() {
    // 初始化指针
    parser.previous = parser.current;
    // 扫描文件，转为一个一个 token
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
        errorAtCurrent(parser.current.start);
    }
}

// 类似于`advance()`，读取下一个标识。如果current错误，则报错
static void consume(TokenType type, const char *message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

// 检查当前的 TokenType 是否为传入的类型
static bool check(TokenType type) {
    return parser.current.type == type;
}

// 检测当前的 type 与传入的 type 是否相同，如果不同则返回 false
static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

// 将 1 个 byte 写入到chunk 中，一元表达式常用
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

// 向 chunk 中写入跳转语句，用 ff 占位，编译完后面的表达式后，再写回跳转距离，用于跳过某些指令
static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    // 返回当前if 在 chunk 的位置
    return currentChunk()->count - 2;
}

// 将 2 个 byte 写入到chunk 中，二元表达式常用
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

// 向 chunk 中加入 loop 指令（回头）
static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large.（循环体过大）");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}


// 将 OP_RETURN 写入 chunk 中
static void emitReturn() {
/* Calls and Functions return-nil < Methods and Initializers return-this
  emitByte(OP_NIL);
*/
//> Methods and Initializers return-this
    if (current->type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);
    } else {
        emitByte(OP_NIL);
    }

//< Methods and Initializers return-this
    emitByte(OP_RETURN);
}

// 将 value 放入常量池中，返回其位置
static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    // 常量池溢出错误
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    // 返回在常量池中的位置
    return (uint8_t) constant;
}

// 将OP_CONSTANT与常量池索引放入 chunk 中
static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

// 写回要跳转的距离
static void patchJump(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    // jump 表示要跳转的距离，
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.（跳转过多）");
    }
    // 将之前 if 里的跳转位置从 ff 改为实际跳转的距离
    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

// 初始化变量池量+函数编译器
static void initCompiler(Compiler *compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    // 初试变量数量为 0
    compiler->localCount = 0;
    // 初试深度为 0
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start, parser.previous.length);
    }
    Local *local = &current->locals[current->localCount++];
    local->depth = 0;
//    local->name.start = "";
//    local->name.length = 0;
    if (type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

// 传入优先级，转换表达式
static void parsePrecedence(Precedence precedence);

// 将标识符转为常量，加入到常量池中
static uint8_t identifierConstant(Token *name) {
    return makeConstant(OBJ_VAL(copyString(name->start,
                                           name->length)));
}

// 判断两个标识符是否相等
static bool identifiersEqual(Token *a, Token *b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

// 解析本地变量，传入 token 名，如果 local 池中无该 token，则返回-1，否则返回当前索引（如果local->depth为-1 则报错）
static int resolveLocal(Compiler *compiler, Token *name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.（无法获取该局部变量，存在变量，但目前已不可用）");
            }
            return i;
        }
    }
    return -1;
}

// 增加一个局部变量，局部变量的深度为-1
static void addLocal(Token name) {
    // 超出变量池限制则报错
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }
    // 从变量池中找到相应位置，命名，设置初试深度
    Local *local = &current->locals[current->localCount++];
    local->name = name;

    local->depth = -1;
}

// 声明变量（仅局部）
static void declareVariable() {
    // 如果为全局变量，则直接跳过
    if (current->scopeDepth == 0) return;
    // 获取变量名
    Token *name = &parser.previous;
    // 反向遍历变量池，如果遍历过程超出了当前的范围，则代表本池中无同名变量，可以插入
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        // 如果该变量池中的变量深度为不为-1，且深度小于当前深度范围
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }
        // 如果当前代码块，已经有了同名变量，则报错
        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.（代码块中，已经有了该同名变量）");
        }
    }
    // 增加一个局部变量
    addLocal(*name);
}

// 处理变量的过程
static uint8_t parseVariable(const char *errorMessage) {
    // 定义变量一定会有赋值操作，如果没有，则报错
    // 消耗掉变量名，如果没有则报错
    consume(TOKEN_IDENTIFIER, errorMessage);
    // 声明变量（将变量放入到应当的位置上）
    declareVariable();
    // 如果当前深度大于 0，则返回 0，否则返回常量池中的位置
    if (current->scopeDepth > 0) return 0;
    return identifierConstant(&parser.previous);
}

// 变量池中增加 1 个变量，变量深度为当前深度
static void markInitialized() {
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth =
            current->scopeDepth;
}

// 定义变量，如果current->scopeDepth > 0，则表明不是全局变量，否则将全局变量加入到 chunk 中，传入值为全局变量在常量池中的位置
static void defineVariable(uint8_t global) {
    // 设置局部变量的深度
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }
    // 如果为全局变量则直接插入，
    emitBytes(OP_DEFINE_GLOBAL, global);
}

// 解析前进所得的 token
static void expression() {
    // 解析优先级，PREC_ASSIGNMENT 为 = 的优先级
    parsePrecedence(PREC_ASSIGNMENT);
}

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.（参数不能超过 255 个）");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.（缺少反括号）");
    return argCount;
}

static void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void statement();

static void declaration();

static ParseRule *getRule(TokenType type);


// 处理代码块
static void block() {
    // 当代码块没有结束，且文件没有结束时，进行处理
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }
    // 处理完代码块，消耗掉}
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.(代码块结束，但是缺少})");
}

// 结束编译
static ObjFunction *endCompiler() {
    // 加入返回
    emitReturn();
    ObjFunction *function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL
                                         ? function->name->chars : "<script>");
    }
#endif
    current = current->enclosing;
    return function;
}

static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction *function = endCompiler();
    emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
}

static void funDeclaration() {
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

// 处理定义变量的过程（全局变量和局部变量）
static void varDeclaration() {
    // 解析变量名（）
    uint8_t global = parseVariable("Expect variable name.（未得到变量名）");
    // 如果变量名解析完后，后面为 = ，说明将后面的语句赋值给当前变量
    if (match(TOKEN_EQUAL)) {
        expression();
    }
        // 否则变量值为OP_NIL
    else {
        emitByte(OP_NIL);
    }
    // 消耗掉后面的;
    consume(TOKEN_SEMICOLON,
            "Expect ';' after variable declaration.（该语句缺少末尾分号；）");

    defineVariable(global);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}


static void forStatement() {
    // 开始代码块
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    if (match(TOKEN_SEMICOLON)) {
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;

    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Condition.
    }
    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }
    statement();
    emitLoop(loopStart);
    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP); // Condition.
    }
    endScope();
}

// 处理 if 语句
static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.（未得到 if 后的小括号）");
    // 解析 if 中的表达式
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.（未得到 if 后的大括号）");

    // 向 chunk 中加入跳转语句，并使用 ffff 占位，表示跳转距离
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    // 解析if体
    statement();

    int elseJump = emitJump(OP_JUMP);

    // 写回跳转距离
    patchJump(thenJump);
    emitByte(OP_POP);

    //
    if (match(TOKEN_ELSE))
        statement();

    patchJump(elseJump);
}


// 解析输出语句
static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.（缺少末尾;）");
    // 向 chunk 中加入 print 操作
    emitByte(OP_PRINT);
}

// 返回语句
static void returnStatement() {

    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.（主函数无法返回值）");
    }
    // 如果直接返回分号，则返回空
    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        if (current->type == TYPE_INITIALIZER) {
            error("Can't return a value from an initializer.（构造函数无法返回）");
        }

        expression();

        consume(TOKEN_SEMICOLON, "Expect ';' after return value.（缺少；）");
        emitByte(OP_RETURN);

    }
}

// 解析 while 语句
static void whileStatement() {

    // 获取当前的 while 索引
    int loopStart = currentChunk()->count;

    // 解析 while 中的表达式
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.(缺少 while 后的括号)");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.(缺少 while 后的反括号)");

    // 加入 jump 指令
    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
}

static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default:; // Do nothing.
        }

        advance();
    }
}

// 开始解析已经读到的 Token
static void declaration() {
    // 判断是否为定义变量（全局和局部均可）
    if (match(TOKEN_FUN)) {
        funDeclaration();
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else if (match(TOKEN_RETURN)) {

        returnStatement();

    }
        // 判断是否为左括号
    else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        //开始---解析括号内容---结束
        beginScope();
        block();
        endScope();
    } // 是否为普通语句
    else {
        statement();
    }
    // 如果当前为错误状态，则。。。
    if (parser.panicMode) synchronize();
}


// 处理普通语句
static void statement() {
    // 处理输出语句
    if (match(TOKEN_PRINT)) {
        printStatement();
    }
        // 处理 for 语句
    else if (match(TOKEN_FOR)) {
        forStatement();
    }
        // 处理 if 语句
    else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    }
        // 处理while 语句
    else if (match(TOKEN_WHILE)) {
        whileStatement();
    }
        // 处理代码块
    else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
        //其他
    } else {
        expressionStatement();
    }
}

// 二元表达式解析

static void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence) (rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:
            emitBytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
            //+-*/
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
        default:
            return; // Unreachable.
    }
}

static void call(bool canAssign) {
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

// 判断前置标识符的类型
static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emitByte(OP_FALSE);
            break;
        case TOKEN_NIL:
            emitByte(OP_NIL);
            break;
        case TOKEN_TRUE:
            emitByte(OP_TRUE);
            break;
        default:
            return; // Unreachable.
    }
}

// 分组表达式。括号过程
static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}


// 将数字加入到常量池中
static void number(bool canAssign) {
    // 字符串转数字
    double value = strtod(parser.previous.start, NULL);
    // 将OP_CONSTANT放入 chunk 中，并将 value 放入常量池中
    emitConstant(NUMBER_VAL(value));
}

// 将字符串加入到常量池中
static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                    parser.previous.length - 2)));
}

// 命名变量
static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    // 查看当前是否有当前命名的变量，返回为 -1 则表示没有
    int arg = resolveLocal(current, &name);
    // 没有该变量，表明该变量为
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t) arg);
    } else {
        emitBytes(getOp, (uint8_t) arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

// 一元表达式解析
static void unary(bool canAssign) {
    // 获取初试的 token 类型
    TokenType operatorType = parser.previous.type;

    // 编译表达式
    // Compile the operand.

    parsePrecedence(PREC_UNARY);

    // 发出操作指令
    switch (operatorType) {
        case TOKEN_BANG:
            emitByte(OP_NOT);
            break;
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        default:
            return; // Unreachable.
    }
}


// 转换规则
ParseRule rules[] = {
        // (
        [TOKEN_LEFT_PAREN]    = {grouping, call, PREC_CALL},
        // ）
        [TOKEN_RIGHT_PAREN]   = {NULL, NULL, PREC_NONE},
        // {}
        [TOKEN_LEFT_BRACE]    = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE]   = {NULL, NULL, PREC_NONE},
        // ,
        [TOKEN_COMMA]         = {NULL, NULL, PREC_NONE},
        // .
        [TOKEN_DOT]           = {NULL, NULL, PREC_NONE},
        // -
        [TOKEN_MINUS]         = {unary, binary, PREC_TERM},
        // +
        [TOKEN_PLUS]          = {NULL, binary, PREC_TERM},
        // ;
        [TOKEN_SEMICOLON]     = {NULL, NULL, PREC_NONE},
        // /
        [TOKEN_SLASH]         = {NULL, binary, PREC_FACTOR},
        // *
        [TOKEN_STAR]          = {NULL, binary, PREC_FACTOR},
        // !
        [TOKEN_BANG]          = {unary, NULL, PREC_NONE},
        // !=
        [TOKEN_BANG_EQUAL]    = {NULL, binary, PREC_EQUALITY},
        // =
        [TOKEN_EQUAL]         = {NULL, NULL, PREC_NONE},
        // ==
        [TOKEN_EQUAL_EQUAL]   = {NULL, binary, PREC_EQUALITY},
        // >
        [TOKEN_GREATER]       = {NULL, binary, PREC_COMPARISON},
        // >=
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
        // <
        [TOKEN_LESS]          = {NULL, binary, PREC_COMPARISON},
        // <=
        [TOKEN_LESS_EQUAL]    = {NULL, binary, PREC_COMPARISON},
        // 其他
        [TOKEN_IDENTIFIER]    = {variable, NULL, PREC_NONE},
        // 字符串
        [TOKEN_STRING]        = {string, NULL, PREC_NONE},
        // 数字
        [TOKEN_NUMBER]        = {number, NULL, PREC_NONE},
        // and or
        [TOKEN_AND]           = {NULL, and_, PREC_AND},
        [TOKEN_OR]            = {NULL, or_, PREC_OR},

        [TOKEN_CLASS]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE]          = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE]         = {literal, NULL, PREC_NONE},
        [TOKEN_FOR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN]           = {NULL, NULL, PREC_NONE},
        [TOKEN_IF]            = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL]           = {literal, NULL, PREC_NONE},
        [TOKEN_PRINT]         = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN]        = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER]         = {NULL, NULL, PREC_NONE},
        [TOKEN_THIS]          = {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE]          = {literal, NULL, PREC_NONE},
        [TOKEN_VAR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF]           = {NULL, NULL, PREC_NONE},
};

// 解析表达式，将高于precedence优先级的式子
static void parsePrecedence(Precedence precedence) {
    // 再前进一步（获取一个 token）
    advance();
    //  获取 1 号函数，为空则表示该符号不能用于前缀？
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }
    // 执行该函数
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);
    //  pre不断前进， 直到 2 号指针指向的优先级不大于当前的优先级
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }
    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

// 返回这个 token 的规则
static ParseRule *getRule(TokenType type) {
    return &rules[type];
}


// 开始解析大括号，深度加一
static void beginScope() {
    current->scopeDepth++;
}

// 退出大括号，深度减一
static void endScope() {
    current->scopeDepth--;
    // 加入退出指令
    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        emitByte(OP_POP);
        current->localCount--;
    }
}

// 将文件转为chunk
ObjFunction *compile(const char *source) {
    // 初始化扫描仪
    initScanner(source);
    // 定义一个编译器（内有变量池）
    Compiler compiler;
    // 初始化编译器
    initCompiler(&compiler, TYPE_SCRIPT);
//     compilingChunk = chunk;
    // 初试状态无错误
    parser.hadError = false;
    parser.panicMode = false;
    // 仅仅前进一位
    advance();
//    // 解析表达式（将会继续前进）
//    expression();
//    // 判断current是否出现错误，错误则报错并直接停止
//    consume(TOKEN_EOF, "Expect end of expression.");

    // 目前没有到文件的末尾，持续解析
    while (!match(TOKEN_EOF)) {
        // 开始解析 scan 到的 Token
        declaration();
    }

    ObjFunction *function = endCompiler();
    return parser.hadError ? NULL : function;
}
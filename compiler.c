//
// Created by 臧帅 on 24-7-14.
//

#include <stdbool.h>
#include <stdio.h>
#include "compiler.h"
#include "scanner.h"
#include "chunk.h"

#ifdef DEBUG_PRINT_CODE

#include "debug.h"

#endif
// 转换器，将 token 转为表达式，
typedef struct {
    Token current;
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

// 函数指针，用于指向

typedef void (*ParseFn)(bool canAssign);

// 规则
typedef struct {
    // 表达式是否能为前缀
    ParseFn prefix;
    // 表达式能否为中缀
    ParseFn infix;
    // 表达式优先级
    Precedence precedence;
} ParseRule;


Parser parser;

Chunk *compilingChunk;

// 返回当前的 chunk 指针
static Chunk *currentChunk() {
    return compilingChunk;
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

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

// 将 1 个 byte 写入到chunk 中，一元表达式常用
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

// 将 2 个 byte 写入到chunk 中，二元表达式常用
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn() {
    // 将 OP_RETURN 写入 chunk 中
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

static void emitConstant(Value value) {
    // 将OP_CONSTANT与常量池索引放入 chunk 中
    emitBytes(OP_CONSTANT, makeConstant(value));
}


//static void expression();
static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token *name) {
    return makeConstant(OBJ_VAL(copyString(name->start,
                                           name->length)));
}

static uint8_t parseVariable(const char *errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global) {
    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void statement();

static void declaration();

static ParseRule *getRule(TokenType type);


// 解析前进所得的 token
static void expression() {
    // 解析优先级，PREC_ASSIGNMENT 为 = 的优先级
    parsePrecedence(PREC_ASSIGNMENT);
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON,
            "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
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

static void declaration() {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) synchronize();
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
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


static void namedVariable(Token name, bool canAssign) {
    uint8_t arg = identifierConstant(&name);

    if (canAssign && match(TOKEN_EQUAL))  {
        expression();
        emitBytes(OP_SET_GLOBAL, arg);
    } else {
        emitBytes(OP_GET_GLOBAL, arg);
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
        [TOKEN_LEFT_PAREN]    = {grouping, NULL, PREC_NONE},
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
        [TOKEN_BANG_EQUAL]    = {NULL, binary, PREC_NONE},
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
        [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
        // 字符串
        [TOKEN_STRING]        = {string, NULL, PREC_NONE},
        // 数字
        [TOKEN_NUMBER]        = {number, NULL, PREC_NONE},
        [TOKEN_AND]           = {NULL, NULL, PREC_NONE},
        [TOKEN_CLASS]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE]          = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE]         = {literal, NULL, PREC_NONE},
        [TOKEN_FOR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN]           = {NULL, NULL, PREC_NONE},
        [TOKEN_IF]            = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL]           = {literal, NULL, PREC_NONE},
        [TOKEN_OR]            = {NULL, NULL, PREC_NONE},
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

// 解析优先级
static void parsePrecedence(Precedence precedence) {
    // 再前进一步（获取一个 token）
    advance();
    printf("xxxxx----parsePrecedence---advance---Parser: %c,%d  %c,%d\n", parser.previous.start[0],
           parser.previous.length, parser.current.start[0], parser.current.length);
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
        printf("while----parsePrecedence---advance---Parser: %c,%d  %c,%d\n", parser.previous.start[0],
               parser.previous.length, parser.current.start[0], parser.current.length);
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


static void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}


// 将文件转为chunk
bool compile(const char *source, Chunk *chunk) {
    // 初始化扫描仪
    initScanner(source);
    compilingChunk = chunk;
    // 初试状态无错误
    parser.hadError = false;
    parser.panicMode = false;
    // 仅仅前进一位
    advance();
//    // 解析表达式（将会继续前进）
//    expression();
//    // 判断current是否出现错误，错误则报错并直接停止
//    consume(TOKEN_EOF, "Expect end of expression.");

    while (!match(TOKEN_EOF)) {
        declaration();
    }


    // 结束编译
    endCompiler();
    // 返回是否编译成功
    return !parser.hadError;
//    int line = -1;
//    for (;;) {
//        Token token = scanToken();
//        if (token.line != line) {
//            printf("%4d ", token.line);
//            line = token.line;
//        } else {
//            printf("   | ");
//        }
//        printf("%2d '%.*s'\n", token.type, token.length, token.start);
//
//        if (token.type == TOKEN_EOF) break;
//    }
}
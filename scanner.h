#ifndef clox_scanner_h
#define clox_scanner_h
// 关键字枚举
typedef enum {
    // (){},.-+;/*
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    // !  !=,  =,  ==,  >,  >=,  <,  <=
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    // 标识符、字符串、数字
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    // and, class, else, false, for, fun, if, nil, or, print, return, super, this, true, var, while, error, eof
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,
    TOKEN_ERROR, TOKEN_EOF
} TokenType;

// Token结构体，
typedef struct {
//    类型
    TokenType type;
//
    const char *start;
// 当前 token 长度
    int length;
//    位于哪一行
    int line;
} Token;

// 初始化扫描器
void initScanner(const char *source);

// 扫描 token扫描到一个就返回
Token scanToken();

#endif

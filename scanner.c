#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;

// 初始化扫描器，start与current都指向文件初始位置，文件行为 1
void initScanner(const char *source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}


// 是不是字母
static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

// 是不是数字
static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

// 有没有结束
static bool isAtEnd() {
    return *scanner.current == '\0';
}

// 获取当前字符，并前进一步，
static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

// 获取当前字符
static char peek() {
    return *scanner.current;
}

// 获取当前字符的下一个字符，指针不移动
static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

// 判断该字符串是否等于 expected，字符是否异常
static bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

// 创建一个 token
static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    // 这个 token 在代码中的起始位置
    token.start = scanner.start;
    // 这个 token 的长度
    token.length = (int) (scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

// 创建一个错误信息的 token
static Token errorToken(const char *message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int) strlen(message);
    token.line = scanner.line;
    return token;
}

// 跳过空格换行等空字符
static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            // 空格
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
                // 换行
            case '\n':
                scanner.line++;
                advance();
                break;
                // 注释
            case '/':
                if (peekNext() == '/') {
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

// 检查当前扫描的指针与rest指向的字符串是否相同，相同则返回当前范围的指针
static TokenType checkKeyword(int start, int length, const char *rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

// 返回当前 token 的类型
static TokenType identifierType() {
// and class else
    switch (scanner.start[0]) {
        case 'a':
            return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c':
            return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e':
            return checkKeyword(1, 3, "lse", TOKEN_ELSE);
// false, for, fun
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a':
                        return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o':
                        return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u':
                        return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
// if, nil, or, print, return, super
        case 'i':
            return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n':
            return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o':
            return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p':
            return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r':
            return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's':
            return checkKeyword(1, 4, "uper", TOKEN_SUPER);
// this true
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h':
                        return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r':
                        return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
// var while
        case 'v':
            return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w':
            return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }

// 其他
    return TOKEN_IDENTIFIER;
}

// 持续向后扫描，直到遇到非字母或数字
static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

// 持续向后扫描，直到遇到非数组，注意考虑小数点
static Token number() {
    while (isDigit(peek())) advance();
    if (peek() == '.' && isDigit(peekNext())) {
        advance();
        while (isDigit(peek())) advance();
    }
    return makeToken(TOKEN_NUMBER);
}

// 扫描字符串，直到遇到下一个双引号
static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }
    if (isAtEnd()) return errorToken("Unterminated string.");
    advance();
    return makeToken(TOKEN_STRING);
}

// 扫描 token
Token scanToken() {
    // 跳过空字符
    skipWhitespace();
    // 确定初始指针
    scanner.start = scanner.current;
    // 文件为空则结束
    if (isAtEnd()) return makeToken(TOKEN_EOF);
    // 获取一个字符
    char c = advance();
    // 判断这个字符的类型，进行相关处理
//    字符
    if (isAlpha(c)) return identifier();
//    数字
    if (isDigit(c)) return number();

//    符号
    switch (c) {
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case ',':
            return makeToken(TOKEN_COMMA);
        case '.':
            return makeToken(TOKEN_DOT);
        case '-':
            return makeToken(TOKEN_MINUS);
        case '+':
            return makeToken(TOKEN_PLUS);
        case '/':
            return makeToken(TOKEN_SLASH);
        case '*':
            return makeToken(TOKEN_STAR);
// != == <= >=
        case '!':
            return makeToken(
                    match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(
                    match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(
                    match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(
                    match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
// "
        case '"':
            return string();
    }
    return errorToken("Unexpected character.");
}

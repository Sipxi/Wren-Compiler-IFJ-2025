#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>
#include <stdlib.h>

/*===== Структура и функции токенов =====*/

/* Перечисление возможных типов токенов */
typedef enum {
    TOKEN_NULL, 
    TOKEN_STRING,
    TOKEN_MULTI_STRING,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_EXP,
    TOKEN_HEX,
    TOKEN_GLOBAL_IDENTIFIER,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_DOT,
    TOKEN_COMMA,
    TOKEN_EOF,
    TOKEN_EOL,
    TOKEN_ASSIGN,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_GREATER,
    TOKEN_EQUAL_GREATER,
    TOKEN_LESS,
    TOKEN_EQUAL_LESS,
    TOKEN_DIVISION
} TokenType;

/**
 * Структура, представляющая токен в исходном коде.
 *
 * Поля:
 * - type: Тип токена (из перечисления TokenType).
 * - data: Указатель на строковые данные токена.
 * - line: Номер строки, где был найден токен.
 */
typedef struct {
    TokenType type;
    char *data;
    int line;
} Token;

/**
 * Инициализирует структуру Token.
 * Эта функция выделяет память для поля данных
 * и устанавливает тип и номер строки на значения по умолчанию.
 *
 * @return Указатель на инициализированную структуру Token.
 */
Token *token_init();

/**
 * Освобождает память, выделенную для поля данных структуры Token.
 *
 * @param token Указатель на структуру Token для освобождения.
 */
void token_free(Token *token);

/* === Вспомогательные функции === */

/**
 * Преобразует тип токена в строковое представление.
 *
 * @param type Тип токена (из перечисления TokenType).
 * @return Строковое представление типа токена.
 */
char *token_type_to_string(TokenType type);

#endif

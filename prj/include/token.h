/**
 * @file token.h
 * 
 * @brief Definice struktury a funkcí pro práci s tokeny v lexikální analýze.
 * 
 * Author:
 *      - Serhij Čepil (253038)
 */

#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>
#include <stdlib.h>

/*===== Struktura a funkce tokenů =====*/

/* Všechny tokeny */
typedef enum {
    TOKEN_UNDEFINED,            // Nedefinovaný token   
    TOKEN_STRING,               // Řetězcový token,         "něco", """něco"""
    TOKEN_INT,                  // Celé číslo               1,2,3...
    TOKEN_FLOAT,                // Desetinné číslo          1.5, 0.75...
    TOKEN_EXP,                  // Exponenciální číslo      1.5e10, 2.3E-4...
    TOKEN_HEX,                  // Hexadecimální číslo      0x1A3F...
    TOKEN_GLOBAL_IDENTIFIER,    // Globální identifikátor   __global_var
    TOKEN_IDENTIFIER,           // Identifikátor            var_name, funcName...
    TOKEN_KEYWORD,              // Klíčové slovo            if, else, while...
    TOKEN_DOT,                  //                          .
    TOKEN_COMMA,                //                          ,
    TOKEN_EOF,                  //                          EOF
    TOKEN_EOL,                  //                          \n
    TOKEN_ASSIGN,               //                          =
    TOKEN_OPEN_PAREN,           //                          (
    TOKEN_CLOSE_PAREN,          //                          )
    TOKEN_OPEN_BRACE,           //                          {
    TOKEN_CLOSE_BRACE,          //                          }
    TOKEN_PLUS,                 //                          +
    TOKEN_MINUS,                //                          -
    TOKEN_MULTIPLY,             //                          *
    TOKEN_EQUAL,                //                          ==
    TOKEN_NOT_EQUAL,            //                          !=
    TOKEN_GREATER,              //                          >
    TOKEN_EQUAL_GREATER,        //                          >=
    TOKEN_LESS,                 //                          <
    TOKEN_EQUAL_LESS,           //                          <=
    TOKEN_DIVISION              //                          /
} TokenType;

/**
 * Struktura, představující token v zdrojovém kódu.
 *
 * Pole:
 * - type: Typ tokenu (z výčtu TokenType).
 * - data: Ukazatel na řetězcová data tokenu.
 * - line: Číslo řádku, kde byl token nalezen.
 */
typedef struct {
    TokenType type;
    char *data;
    int line;
} Token;

/**
 * @brief Kopíruje data z jednoho tokenu do druhého.
 * 
 * @param dest Ukazatel na cílový token.
 * @param src Ukazatel na zdrojový token.
 * 
 */
void token_copy_data(Token* dest, const Token* src);

/**
 * Inicializuje strukturu Token.
 * Tato funkce alokuje paměť pro pole dat
 * a nastaví typ a číslo řádku na výchozí hodnoty.
 *
 * @return Ukazatel na inicializovanou strukturu Token.
 */
Token *token_init();

/**
 * Uvolňuje paměť alokovanou pro pole dat struktury Token.
 *
 * @param token Ukazatel na strukturu Token k uvolnění.
 */
void token_free(Token *token);

/* === Pomocné funkce === */

/**
 * Převádí typ tokenu na řetězcovou reprezentaci.
 *
 * @param type Typ tokenu (z výčtu TokenType).
 * @return Řetězcová reprezentace typu tokenu.
 */
char *token_type_to_string(TokenType type);

#endif // TOKEN_H

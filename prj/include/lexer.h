/**
 * @file lexer.h
 * 
 * Hlavičkový soubor pro lexer, který zodpovídá za tokenizaci zdrojového kódu.
 * 
 * Tento soubor obsahuje definice struktury Lexer a prototypy funkcí
 * pro inicializaci, uvolnění a zpracování tokenů.
 * Autor:
 *     - Serhij Čepil (253038)
 *     - Dmytro Kravchenko (273125)
 *     - Veronika Turbaievska (273123)
 */

//! Допишите ваши имена и номера

#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <stdio.h>

#define TOKEN_BUFFER_SIZE 2
/*=======================================*/
/*===== Struktura a funkce lexeru =====*/
/*=======================================*/

/**
 * Struktura představující lexer pro tokenizaci zdrojového kódu.
 *
 * Pole:
 * - position: Aktuální pozice v zdrojovém kódu.
 * - line: Aktuální číslo řádku.
 * - current_token: Ukazatel na aktuálně zpracovávaný token.
 */
typedef struct {
    int position;
    int line;
    Token *current_token;
    Token buffered_tokens[TOKEN_BUFFER_SIZE];
    int buffered_count;
} Lexer;

/**
 * Inicializuje strukturu Lexer.
 *
 * Tato funkce alokuje paměť pro strukturu Lexer
 * a inicializuje její pole.
 *
 * @return Ukazatel na inicializovanou strukturu Lexer.
 */
Lexer *lexer_init();

/**
 * Uvolňuje paměť alokovanou pro strukturu Lexer.
 *
 * Tato funkce uvolňuje aktuální token a jakoukoli další alokovanou paměť.
 *
 * @param lexer Ukazatel na strukturu Lexer k uvolnění.
 */
void lexer_free(Lexer *lexer);

/**
 * Zpracovává a nastavuje token chyby v lexeru.
 * 
 * Vypisuje chybovou zprávu s uvedením kódu chyby, zprávy, čísla řádku a pozice.
 * Uvolňuje paměť alokovanou pro lexer a ukončuje program s kódem chyby.
 * 
 * @param lexer Ukazatel na strukturu Lexer.
 * @param error_code Kód chyby pro nastavení.
 * @param message Zpráva o chybě pro výpis.
 */
void lexer_error(Lexer *lexer, int error_code, const char *message);


/**
 * @brief Prohlíží další token ze zdrojového kódu bez jeho odstranění z proudu.
 *
 * Tato funkce čte znaky ze zadaného souboru a vrací další token.
 * 
 * @note Pokud token nebyl přečten, je uložen do bufferu lexéru.
 * 
 * 
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file Ukazatel na soubor obsahující zdrojový kód.
 * @return Další struktura Token.
 */
Token peek_token(Lexer *lexer, FILE *file);

/**
 * @brief Prohlíží další za dalším token ze zdrojového kódu bez jeho odstranění z proudu.
 * 
 * Tato funkce čte znaky ze zadaného souboru a vrací token, který je následující za dalším.
 * 
 * @note Pokud token nebyl přečten, je uložen do bufferu lexéru 2 tokeny.
 * 
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file Ukazatel na soubor obsahující zdrojový kód.
 * @return Token následující za dalším.
 */
Token peek_next_token(Lexer *lexer, FILE *file);

/**
 * Získává další token ze zdrojového kódu.
 *
 * Tato funkce čte znaky ze zadaného souboru
 * a vytváří další token na základě pravidel lexeru.
 * Pokud byl proveden náhled tokenu pomocí peek_token,
 * vrací stejný token bez čtení nového.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file Ukazatel na soubor obsahující zdrojový kód.
 * @return Další struktura Token.
 */
void get_token(Lexer *lexer, FILE *file);


#endif // LEXER_H
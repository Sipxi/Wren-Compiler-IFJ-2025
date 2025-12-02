/**
 * @file stack_precedence.h
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Hlavičkový soubor pro stack_precedence.c
 *
 * @author
 *     - Veronika Turbaievska (273123)
 */

#ifndef PARSER_STACK_H
#define PARSER_STACK_H

#include "token.h"        
#include "precedence.h"   
#include "ast.h"    

#include <stdbool.h>

/**
 * @brief Element uložený na zásobníku precedenčního analyzátoru.
 *
 * Obsahuje nejen gramatický symbol (pro logiku parseru),
 * ale i originální token. To je potřeba, aby při redukci E -> i
 * jsme věděli, *jaký* identifikátor nebo literál to byl.
 */
typedef struct {
    GrammarSymbol symbol; // Gramatický symbol (GS_E, GS_TERM, GS_PLUS, atd.)
    Token token;          // Originální token od lexeru
    AstNode *ast_node;   // Ukazatel na uzel AST spojený s tímto prvkem
    
} PStackItem;


/**
 * @brief Struktura zásobníku.
 * Implementována jako dynamické (rozšiřitelné) pole.
 */
typedef struct {
    PStackItem *items; // Ukazatel na pole prvků
    int top;                // Index vrcholu (-1 = prázdný)
    int capacity;           // Aktuální alokovaná kapacita
} PStack;

// --- Rozhraní zásobníku ---

/**
 * @brief Vytvoří a inicializuje nový prázdný zásobník.
 * @return Ukazatel na novou strukturu zásobníku nebo NULL při chybě alokace.
 * @param s Ukazatel na zásobník, který se má inicializovat.
 */
void PSTACK_init(PStack *s);

/**
 * @brief Uvolní veškerou paměť alokovanou pro zásobník.
 * @param s Ukazatel na zásobník, který se má zničit.
 */
void PSTACK_free(PStack *s);

/**
 * @brief Zkontroluje, zda je zásobník prázdný.
 * @param s Ukazatel na zásobník.
 * @return true, pokud je zásobník prázdný, jinak false.
 */
bool PSTACK_is_empty(PStack *s);

/**
 * @brief Vkládá (push) prvek na vrchol zásobníku.
 * Při potřeby automaticky rozšiřuje kapacitu zásobníku.
 * @param s Ukazatel na zásobník.
 * @param item Prvek, který se má vložit na zásobník.
 * @return true v případě úspěchu, false při chybě alokace.
 */
bool PSTACK_push(PStack *s, PStackItem item);

/**
 * @brief Odebírá (pop) prvek z vrcholu zásobníku.
 * @param s Ukazatel na zásobník.
 * @return Odebraný prvek z vrcholu.
 */
PStackItem PSTACK_pop(PStack *s);
/**
 * @brief "Podívá se dopředu" (peek) prvek na vrcholu zásobníku, aniž by ho odebral.
 * @param s Ukazatel na zásobník.
 * @return Prvek na vrcholu.
 */
PStackItem PSTACK_top(PStack *s);


// --- Specializované funkce pro parser ---

/**
 * @brief Najde nejvyšší *terminál* na zásobníku.
 *
 * To je klíčová funkce pro parser. "Prohlíží" se dolů
 * všechny neterminály (GS_E), dokud nenajde první terminál (GS_PLUS, GS_TERM, GS_DOLLAR atd.)
 *
 * @param s Ukazatel na zásobník.
 * @return Ukazatel na nejvyšší *terminální* prvek na zásobníku.
 * Vrací NULL, pokud je zásobník prázdný nebo se něco pokazilo.
 */
PStackItem* PSTACK_get_top_terminal(PStack *s);


#endif // PARSER_STACK_H
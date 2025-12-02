/**
 * @file precedence.h
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Hlavičkový soubor pro precedence.c
 *
 * @author
 *     - Veronika Turbaievska (273123)
 */

#ifndef PRECEDENCE_H
#define PRECEDENCE_H

#include "lexer.h" 

/**
 * @brief Indexy pro terminály v tabulce precedencí.
 * Používají se jak pro zásobník (S), tak pro vstup (I).
 */
typedef enum {
    // Operátory
    T_PLUS,       // 0: +
    T_MINUS,      // 1: -
    T_MUL,        // 2: *
    T_DIV,        // 3: /
    T_LT,         // 4: <
    T_GT,         // 5: >
    T_LTE,        // 6: <=
    T_GTE,        // 7: >=
    T_IS,         // 8: is
    T_EQ,         // 9: ==
    T_NEQ,        // 10: !=
    T_OPEN_PAREN, // 11: (
    T_CLOSE_PAREN,// 12: )
    
    // Operandy a typy
    T_TERM,       // 13: i (id, int, string, null, ...)
    T_TYPE,       // 14: k (Num, String, Null)
    
    // Speciální symboly
    T_DOLLAR,     // 15: $ (konec výrazu)
    
    T_ERROR       // 16: Chybový token
} TermIndex; // Index terminálu

/**
 * @brief Symboly pro gramatická pravidla (Neterminály + Terminály)
 * GS = Grammar Symbol
 */
typedef enum {
    GS_UNDEF = 0, // Nedefinovaný/prázdný symbol

    // --- Terminály (musí odpovídat TermIndex) ---
    GS_PLUS,        // 1: +
    GS_MINUS,       // 2: -
    GS_MUL,         // 3: *
    GS_DIV,         // 4: /
    GS_LT,          // 5: <
    GS_GT,          // 6: >
    GS_LTE,         // 7: <=
    GS_GTE,         // 8: >=
    GS_IS,          // 9: is
    GS_EQ,          // 10: ==
    GS_NEQ,         // 11: !=
    GS_OPEN_PAREN,  // 12: (
    GS_CLOSE_PAREN, // 13: )
    GS_TERM,        // 14: i (operand)
    GS_TYPE,        // 15: k (typ)
    GS_DOLLAR,      // 16: $
    
    // --- Neterminály ---
    GS_E,           // 17: E (Expression)
    GS_HANDLE_START // 18: < (Speciální marker začátku rukojeti)

} GrammarSymbol;

typedef enum {
    GR_RULE_E_OP_E_START = 0,   // E -> E op E
    GR_RULE_E_OP_E_END   = 9,

    GR_RULE_E_IS_K = 10,        // E -> E is k
    GR_RULE_PAREN_E = 11,     // E -> ( E )
} GrammarRuleIndex;

// --- Konstanta ---
#define PRECEDENCE_TABLE_SIZE 16 // 16x16 (od T_PLUS do T_DOLLAR)
#define NUM_GRAMMAR_RULES 13     // Máme 13 pravidel (0-12)
#define MAX_RULE_LENGTH 4        // Max. délka pravidla: 3 (E -> E op E) + 1 (výsledek)

/**
 * @brief Globální precedence tabulka
 */
extern const char precedence_table[PRECEDENCE_TABLE_SIZE][PRECEDENCE_TABLE_SIZE];

/**
 * @brief Globální pravidla gramatiky
 */
extern const int grammar_rules[NUM_GRAMMAR_RULES][MAX_RULE_LENGTH];

/**
 * @brief Konvertuje token ze scanneru na index pro tabulku.
 * @param token Token ze scanneru.
 * @return TermIndex Index pro precedence_table.
 */
TermIndex token_to_index(Token token);

/**
 * @brief Získá pravidlo ('>', '<', '=', 'E') z tabulky.
 * @param stack_top Index symbolu na vrcholu zásobníku.
 * @param input Index symbolu na vstupu.
 * @return char Pravidlo.
 */
char get_precedence_rule(TermIndex stack_top, TermIndex input);

/**
 * @brief Konvertuje token ze scanneru na symbol gramatiky.
 * Používá se při *posunu* (shift).
 * @param token Token ze scanneru.
 * @return GrammarSymbol Symbol pro zásobník parseru.
 */
GrammarSymbol token_to_grammar_symbol(Token token);


#endif // PRECEDENCE_H

/**
 * @file precedence.c
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Implementace pro precedence.h
 *
 * @author
 *     - Veronika Turbaievska (273123)
 */

#include "precedence.h"

#include <string.h> // pro strcmp

// --- 1. Precedence tabulka (16x16) ---
// S - Řádky (Zásobník)
// I - Sloupce (Vstup)
//
// '<' - Shift (posun)
// '>' - Reduce (snížení)
// '=' - Equal (pro závorky a $)
// 'E' - Error (chyba)
//
// Indeksy:
// 0:+, 1:-, 2:*, 3:/, 4:<, 5:>, 6:<=, 7:>=, 8:is, 9:==, 10:!=, 11:(, 12:), 13:i, 14:k, 15:$

const char precedence_table[PRECEDENCE_TABLE_SIZE][PRECEDENCE_TABLE_SIZE] = {
// I ->   | + | - | * | / | < | > |<= |>= | is| ==| !=| ( | ) | i | k | $ |
/* S +  */{'>','>','<','<','>','>','>','>','>','>','>','<','>','<','E','>'},
/* S -  */{'>','>','<','<','>','>','>','>','>','>','>','<','>','<','E','>'},
/* S *  */{'>','>','>','>','>','>','>','>','>','>','>','<','>','<','E','>'},
/* S /  */{'>','>','>','>','>','>','>','>','>','>','>','<','>','<','E','>'},
/* S <  */{'<','<','<','<','E','E','E','E','>','>','>','<','>','<','E','>'},
/* S >  */{'<','<','<','<','E','E','E','E','>','>','>','<','>','<','E','>'},
/* S <= */{'<','<','<','<','E','E','E','E','>','>','>','<','>','<','E','>'},
/* S >= */{'<','<','<','<','E','E','E','E','>','>','>','<','>','<','E','>'},
/* S is */{'<','<','<','<','<','<','<','<','E','>','>','E','>','E','<','>'},
/* S == */{'<','<','<','<','<','<','<','<','<','E','E','<','>','<','E','>'},
/* S != */{'<','<','<','<','<','<','<','<','<','E','E','<','>','<','E','>'},
/* S (  */{'<','<','<','<','<','<','<','<','<','<','<','<','=','<','E','E'},
/* S )  */{'>','>','>','>','>','>','>','>','>','>','>','E','>','E','E','>'},
/* S i  */{'>','>','>','>','>','>','>','>','>','>','>','E','>','E','E','>'},
/* S k  */{'>','>','>','>','>','>','>','>','>','>','>','E','>','E','E','>'},
/* S $  */{'<','<','<','<','<','<','<','<','<','<','<','<','E','<','E','='}
};



// --- 2. Pravidla gramatiky ---
// {Výsledek, Pravidlo_1, Pravidlo_2, Pravidlo_3}
// GS_UNDEF se používá jako "prázdné" pole.

const int grammar_rules[NUM_GRAMMAR_RULES][MAX_RULE_LENGTH] = {
    // {Výsledek, Prvek 1, Prvek 2, Prvek 3}

    // E -> E op E (3 prvky)
    {GS_E, GS_E, GS_PLUS,  GS_E}, // 0
    {GS_E, GS_E, GS_MINUS, GS_E}, // 1
    {GS_E, GS_E, GS_MUL,   GS_E}, // 2
    {GS_E, GS_E, GS_DIV,   GS_E}, // 3
    {GS_E, GS_E, GS_LT,    GS_E}, // 4
    {GS_E, GS_E, GS_GT,    GS_E}, // 5
    {GS_E, GS_E, GS_LTE,   GS_E}, // 6
    {GS_E, GS_E, GS_GTE,   GS_E}, // 7
    {GS_E, GS_E, GS_EQ,    GS_E}, // 8
    {GS_E, GS_E, GS_NEQ,   GS_E}, // 9

    // E -> E is k (3 prvky)
    {GS_E, GS_E, GS_IS,    GS_TYPE}, // 10

    // E -> ( E ) (3 prvky)
    {GS_E, GS_OPEN_PAREN, GS_E, GS_CLOSE_PAREN}, // 11

    // E -> i (1 prvek)
    {GS_E, GS_TERM, GS_UNDEF, GS_UNDEF} // 12
};


// --- 3. Pomocné funkce ---

TermIndex token_to_index(Token token) {
    switch (token.type) {
        // Operátory
        case TOKEN_PLUS:          return T_PLUS;
        case TOKEN_MINUS:         return T_MINUS;
        case TOKEN_MULTIPLY:      return T_MUL;
        case TOKEN_DIVISION:      return T_DIV;
        case TOKEN_LESS:          return T_LT;
        case TOKEN_GREATER:       return T_GT;
        case TOKEN_EQUAL_LESS:    return T_LTE;
        case TOKEN_EQUAL_GREATER: return T_GTE;
        case TOKEN_EQUAL:         return T_EQ;
        case TOKEN_NOT_EQUAL:     return T_NEQ;
        case TOKEN_OPEN_PAREN:    return T_OPEN_PAREN;
        case TOKEN_CLOSE_PAREN:   return T_CLOSE_PAREN;

        // Operandy (i)
        case TOKEN_IDENTIFIER:
        case TOKEN_GLOBAL_IDENTIFIER:
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_EXP:
        case TOKEN_HEX:
        case TOKEN_STRING:
            return T_TERM; // 'i'

        // Klíčová slova
        case TOKEN_KEYWORD:
            if (token.data == NULL) return T_ERROR;
            
            // Operátor 'is'
            if (strcmp(token.data, "is") == 0) return T_IS;
            
            // Literál 'null' (to je operand 'i')
            if (strcmp(token.data, "null") == 0) return T_TERM; // 'i'
            
            // Klíčová slova typu (to je 'k') [cite: 303]
            if (strcmp(token.data, "Num") == 0) return T_TYPE; // 'k'
            if (strcmp(token.data, "String") == 0) return T_TYPE; // 'k'
            if (strcmp(token.data, "Null") == 0) return T_TYPE; // 'k'
            
            // Jiná klíčová slova (if, while...) by neměla být ve výrazu
            return T_ERROR;

        // Konec výrazu ($)
        case TOKEN_EOL:
        case TOKEN_EOF:
        case TOKEN_OPEN_BRACE:
        case TOKEN_UNDEFINED:
            return T_DOLLAR;

        // Chyba
        default:
            return T_ERROR;
    }
}

GrammarSymbol token_to_grammar_symbol(Token token) {
    switch (token.type) {
        // Operátory
        case TOKEN_PLUS:          return GS_PLUS;
        case TOKEN_MINUS:         return GS_MINUS;
        case TOKEN_MULTIPLY:      return GS_MUL;
        case TOKEN_DIVISION:      return GS_DIV;
        case TOKEN_LESS:          return GS_LT;
        case TOKEN_GREATER:       return GS_GT;
        case TOKEN_EQUAL_LESS:    return GS_LTE;
        case TOKEN_EQUAL_GREATER: return GS_GTE;
        case TOKEN_EQUAL:         return GS_EQ;
        case TOKEN_NOT_EQUAL:     return GS_NEQ;
        case TOKEN_OPEN_PAREN:    return GS_OPEN_PAREN;
        case TOKEN_CLOSE_PAREN:   return GS_CLOSE_PAREN;

        // Operandy (i)
        case TOKEN_IDENTIFIER:
        case TOKEN_GLOBAL_IDENTIFIER:
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_EXP:
        case TOKEN_HEX:
        case TOKEN_STRING:
            return GS_TERM; // 'i'

        // Klíčová slova
        case TOKEN_KEYWORD:
            if (token.data == NULL) return GS_UNDEF; // Chyba
            if (strcmp(token.data, "is") == 0) return GS_IS;
            if (strcmp(token.data, "null") == 0) return GS_TERM; // 'i'
            if (strcmp(token.data, "Num") == 0) return GS_TYPE; // 'k'
            if (strcmp(token.data, "String") == 0) return GS_TYPE; // 'k'
            if (strcmp(token.data, "Null") == 0) return GS_TYPE; // 'k'
            return GS_UNDEF; // Chyba

        // Konec výrazu ($)
        case TOKEN_EOL:
        case TOKEN_EOF:
        case TOKEN_OPEN_BRACE:
            return GS_DOLLAR;

        // Chyba
        default:
            return GS_UNDEF; // Neurčený/chybný symbol
    }
}


char get_precedence_rule(TermIndex stack_top, TermIndex input) {
    if (stack_top == T_ERROR || input == T_ERROR || 
        stack_top >= PRECEDENCE_TABLE_SIZE || input >= PRECEDENCE_TABLE_SIZE) {
        return 'E'; // Chyba
    }
    return precedence_table[stack_top][input];
}

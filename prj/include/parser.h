/**
 * @file parser.h
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Hlavičkový soubor pro parser.c
 *
 * @author
 *     - Veronika Turbaievska (273123)
 */

#if !defined(PARSER_H)
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer* lexer;       // Ukazatel na lexér
    FILE* file;         // Ukazatel na vstupní soubor

} Parser;

/**
 * @brief Spustí parser pro analýzu vstupního souboru a vytvoření AST.
 * 
 * @param file Ukazatel na vstupní soubor pro parsování.
 * @return AstNode* Ukazatel na kořenový uzel vytvořeného AST.
 */
AstNode *parser_run(FILE *file);


#endif // PARSER_H
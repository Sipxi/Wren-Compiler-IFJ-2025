/**
 * @file expression.h
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Hlavičkový soubor pro expression.c
 *
 * @author
 *     - Veronika Turbaievska (273123)
 */

#ifndef _EXPRESSION_H_
#define _EXPRESSION_H_

#include "lexer.h"
#include "ast.h"

#include <stdbool.h>

/**
 * @brief Parsuje výraz pomocí precedenční analýzy a vytváří odpovídající AST uzel
 * 
 * @param lexer Ukazatel na lexer
 * @param file Ukazatel na soubor
 * @param expr_node AST uzel pro přidání výsledku
 * @return true Výraz byl úspěšně parsován
 * @return false Chyba při parsování výrazu
 */
bool parser_expression(Lexer *lexer, FILE *file, AstNode *expr_node);

/**
 * @brief Převádí řetězec na double
 * 
 * @param str Vstupní řetězec
 * @param out_value Ukazatel na výstupní hodnotu
 * @return true Úspěšná konverze
 */
bool char_to_double(const char *str, double *out_value);

/**
 * @brief Vytváří listový uzel AST z tokenu
 * 
 * @param token Vstupní token
 * @param node_type_term Typ uzlu AST
 * @return AstNode* Ukazatel na vytvořený uzel AST
 */
AstNode *create_leaf_node (Token token, NodeType node_type_term);



#endif 
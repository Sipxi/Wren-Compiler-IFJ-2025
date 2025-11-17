/**
 * ast_printer.h
 * * Объявляет функции для отладочной печати AST.
 * Реализация находится в ast_printer.c
 */

#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "ast.h" // Нужно, чтобы знать, что такое 'AstNode'

/**
 * @brief (Для отладки) Печатает дерево, начиная с 'node'.
 * @param node Корневой узел для печати.
 */
void ast_print_debug(AstNode* node);


#endif // AST_PRINTER_H
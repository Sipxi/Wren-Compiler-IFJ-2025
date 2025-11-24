#ifndef _EXPRESSION_H_
#define _EXPRESSION_H_

#include "lexer.h"
#include "ast.h"

#include <stdbool.h>

/**
 * @brief Парсит выражение с использованием алгоритма приоритетного разбора
 * 
 * @param lexer Указатель на лексер
 * @param file Указатель на файл
 * @param expr_node Узел AST для добавления результата
 * @return true Выражение успешно распарсено
 * @return false Ошибка парсинга выражения
 */
bool parser_expression(Lexer *lexer, FILE *file, AstNode *expr_node);

/**
 * @brief Преобразует строку в double
 * 
 * @param str Входная строка
 * @param out_value Указатель на выходное значение
 * @return true Успешная конверсия
 */
bool char_to_double(const char *str, double *out_value);

/**
 * @brief Создает листовой узел AST из токена
 * 
 * @param token Входной токен
 * @param node_type_term Тип узла AST
 * @return AstNode* Указатель на созданный узел AST
 */
AstNode *create_leaf_node (Token token, NodeType node_type_term);



#endif 
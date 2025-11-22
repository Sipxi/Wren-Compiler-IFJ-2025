#if !defined(PARSER_H)
#define PARSER_H

#include "lexer.h"
#include "ast.h"
typedef struct {
    Lexer* lexer;       // Указатель на Лексер
    FILE* file;         // Указатель на исходный файл

} Parser;

/**
 * @brief Запускает парсер для анализа входного файла и построения AST.
 * 
 * @param file Указатель на входной файл для парсинга.
 * @return AstNode* Указатель на корневой узел построенного AST.
 */
AstNode *parser_run(FILE *file);


#endif // PARSER_H
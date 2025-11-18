#if !defined(PARSER_H)
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer* lexer;       // Указатель на Лексер
    FILE* file;         // Указатель на исходный файл

} Parser;

AstNode *parser_run(FILE *file);
void parser_prolog(Lexer *lexer, FILE *file);

#endif // PARSER_H
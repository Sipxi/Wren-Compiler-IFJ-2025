#if !defined(PARSER_H)
#define PARSER_H

#include "lexer.h"

typedef struct {
    Lexer* lexer;       // Указатель на Лексер
    FILE* file;         // Указатель на исходный файл

} Parser;

void parser_run();
void parser_prolog();

#endif // PARSER_H
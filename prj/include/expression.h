#ifndef _EXPRESSION_H_
#define _EXPRESSION_H_

#include "lexer.h"
#include <stdbool.h>

bool parser_expression(Lexer *lexer, FILE *file);

bool is_term(Token token);

#endif 
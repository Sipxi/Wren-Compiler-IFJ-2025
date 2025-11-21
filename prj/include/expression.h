#ifndef _EXPRESSION_H_
#define _EXPRESSION_H_

#include "lexer.h"
#include <stdbool.h>
#include "ast.h"

bool parser_expression(Lexer *lexer, FILE *file, AstNode *expr_node);
bool char_to_double(const char *str, double *out_value);



#endif 
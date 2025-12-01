
#ifndef SEMANTICS_H
#define SEMANTICS_H

#include <stdbool.h>
#include <string.h>
#include "ast.h"
#include "symtable.h"

// Globální tabulka symbolů.
extern Symtable global_table;


// Spouští se hlavní proces sémantické analýzy nad abstraktním syntaktickým stromem
bool analyze_semantics(AstNode* root);

#endif // SEMANTICS_H
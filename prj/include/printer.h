#ifndef PRINTER_H
#define PRINTER_H

#include "tac.h" 
#include "ast.h"
#include "codegen.h"


void ast_print_debug(AstNode* node);


void print_tac_list(TACDLList *tac_list);


void print_single_tac_instruction_gencode(TacInstruction *instr);


void print_single_tac_instruction(TacInstruction *instr);

void free_tac_instruction(TacInstruction *data);

#endif  // PRINTER_H
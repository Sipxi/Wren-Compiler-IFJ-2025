// Структуры и функции генератора целевого кода
//
// Авторы:
// Dmytro Kravchenko (273125)
//

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdbool.h>
#include <symtable.h>
#include <string.h>
#include "tac.h"

#include <stdio.h>

typedef enum {
    GF,
    LF,
    TF,
    INT_FRAME,
    FLOAT_FRAME,
    STRING_FRAME,
    BOOL_FRAME,
    NIL_FRAME,
} FrameType;


void DLL_GetInstr(TACDLList *list, TacInstruction **instr);
void gen_init(Symtable *table);
void gen_push_frame();
void gen_create_frame();
void gen_pop_frame();
void gen_label(char* label_name);
void gen_jump(char* label_name);
void gen_call(char* label_name);
void gen_return(TacInstruction *instr);
void gen_jumpifeq(TacInstruction *instr);
void gen_operand(Operand *op);
void gen_tac(TacInstruction *instr);
void gen_type_check(TacInstruction *instr);
void gen_divide(TacInstruction *instr);
void gen_same_operand_check(TacInstruction *instr);
void gen_convert_result(TacInstruction *instr);
void gen_arithmetic(TacInstruction *instr);
void gen_defvar(Operand *var);
void gen_move(Operand *dest, Operand *src);
void gen_param(TACDLList *instructions);
void gen_mul_str(TacInstruction *instr);
void gen_comprasion(TacInstruction *instr);
void gen_eq_comprasion(TacInstruction *instr);
void gen_not_equal(TacInstruction *instr);
void gen_declare(Operand *result);
void gen_function_begin(TACDLList  *instructions);
void gen_label_from_instr(TACDLList  *instructions);
void gen_is(TacInstruction *instr);
int generate_code(TACDLList *instructions, Symtable *table);
void gen_ifj_fun(TACDLList *instructions);
void gen_read_str();
void gen_read_num();
void gen_write(TACDLList *instructions);
void gen_floor(TACDLList *instructions);
void gen_str(TACDLList *instructions);

#endif // CODEGEN_H
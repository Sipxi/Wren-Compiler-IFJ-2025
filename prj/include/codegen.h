// Структуры и функции генератора целевого кода
//
// Авторы:
// Dmytro Kravchenko (273125)
//

#ifndef CODEGEN_H
#define CODEGEN_H

#include "common.h"
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



void gen_init();
// --- Функции управления фреймами ---
void gen_create_frame();
void gen_push_frame();
void gen_pop_frame();
void gen_param(TacInstruction *instr);

// --- Инструкции работы с данными (пример) ---
void gen_operand(Operand *op);
void gen_defvar(Operand *var);
void gen_move(Operand *dest, Operand *src);

// --- Инструкции управления потоком (пример) ---
void gen_return();
void gen_call(char* label_name);
void gen_label(char* label_name);
void gen_jump(char* label_name);
void gen_jumpifeq(char* label_name);

#endif // CODEGEN_H
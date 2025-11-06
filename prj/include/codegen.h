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
    INT,
    FLOAT,
    STRING,
    BOOL,
    NIL,
} FrameType;



void gen_init();
// --- Функции управления фреймами ---
void gen_create_frame();
void gen_push_frame();
void gen_pop_frame();
void gen_param(Quadruple *instr);

// --- Инструкции работы с данными (пример) ---
void gen_operand(FrameType frame, Operand *op);
void gen_defvar(FrameType frame, Operand *var);
void gen_add(Operand *dest, Operand *src1, Operand *src2); // 3-адресная версия
void gen_move(FrameType frame_dest, Operand *dest, FrameType frame_src, Operand *src);

// --- Инструкции управления потоком (пример) ---
void gen_return();
void gen_call(char* label_name);
void gen_label(char* label_name);
void gen_jump(char* label_name);
void gen_jumpifeq(char* label_name, Operand op1, Operand op2);

#endif // CODEGEN_H
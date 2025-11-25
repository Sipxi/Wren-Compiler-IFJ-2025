// Структуры и функции генератора целевого кода
//
// Авторы:
// Dmytro Kravchenko (273125)
//

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "symtable.h"
#include "tac.h"
#include "utils.h"

// ? Потом если не нужно удалить
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

/**
 * @brief Hlavní funkce pro generování cílového kódu z TAC instrukcí.
 * 
 * @param instructions Seznam TAC instrukcí pro generování kódu.
 * @param table Symbolová tabulka pro generování kódu.
 * @return int Návratový kód (0 při úspěchu).
 */
int generate_code(TACDLList *instructions, Symtable *table);


char* string_to_ifjcode(const char *original);
#endif // CODEGEN_H
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

typedef struct {
    bool is_live;
    int next_use_line; // -1 = 'none'
} LivenessInfo;

typedef struct {
    // ----------------
    // 3AK Код
    // ----------------
    InstructionData data;

    // ----------------
    // TZB / Liveness (заполняется в 1-м проходе)
    // ----------------
    LivenessInfo info_arg1;
    LivenessInfo info_arg2;
    LivenessInfo info_result; // Статус 'result' до его перезаписи

} TZB_Data;

/**
 * Преобразует двусвязный список инструкций 3AK в двусвязный список инструкций TZB,
 * добавляя информацию о "жизни" (Liveness) для каждого операнда.
 * 
 * @param instruction_list указатель на двусвязный список инструкций 3AK
 */
void convert_Instructions_To_TZB(DLList* instruction_list);
#endif // CODEGEN_H
/* tac_playground.h
 * Заголовочный файл для генератора 3AC.
 */
#ifndef TAC_PLAYGROUND_H
#define TAC_PLAYGROUND_H

#include "common.h"     // Для DLList
#include "sample_ast.h" // Для AstNode
#include "symtable.h"   // Для Symtable
#include "tac_example.h"
/* ======================================*/


/**
 * @brief Главная функция генерации 3AC.
 * Обходит AST и заполняет список инструкций.
 * * @param tac_list Пустой, инициализированный DLList, который будет заполнен.
 * @param ast_root Корневой узел AST (AST_PROGRAM).
 * @param global_table Глобальная таблица символов.
 */
void generate_tac(DLList *tac_list, AstNode *ast_root, Symtable *global_table);

/**
 * @brief Печатает содержимое списка 3AC в читаемом виде.
 * * @param tac_list Заполненный список инструкций.
 */
void print_tac_list(DLList *tac_list);


/**
 * @brief Функция для DLL_Dispose для очистки памяти из-под TacInstruction.
 * (Вызывается из dll_list.c)
 */
void free_tac_instruction(void* data);

#endif // TAC_PLAYGROUND_H
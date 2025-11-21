/**
 * @file printer.h
 *
 * @brief Заголовочный файл для функций печати и отладки любых структур данных.
 *
 * Author:
 *     - Serhij Čepil (253038)
 */
#ifndef PRINTER_H
#define PRINTER_H

#include "tac.h" 
#include "ast.h"

/**
 * @brief (Для отладки) Печатает дерево, начиная с 'node'.
 * @param node Корневой узел для печати.
 */
void ast_print_debug(AstNode* node);

 /**
  * @brief Печатает содержимое списка 3AC в читаемом виде.
  *
  * @param tac_list Заполненный список инструкций.
  */
void print_tac_list(TACDLList *tac_list);

 /**
  * @brief Печатает 3AC в читаемом виде.
  *
  * @param instr Инструкция.
  */
void print_single_tac_instruction(TacInstruction *instr);

/**
 * @brief Функция для TACDLL_Dispose для очистки памяти из-под TacInstruction.
 * (Вызывается из dll.c)
 *
 * @param data Указатель на TacInstruction для освобождения.
 */
void free_tac_instruction(TacInstruction *data);

#endif  // PRINTER_H
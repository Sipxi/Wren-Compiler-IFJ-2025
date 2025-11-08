/** 
 * @file tac_printer.h
 *
 * @brief Заголовочный файл для функций печати и очистки трехадресного кода (3AC).
 * 
 * TODO и вопрос по рефакторингу: @Bulcer @legon112 @Nikorya
 * Может для удобства сделать единый заголовочный файл для вывода логов/отладочной информации?
 *
 * 
 * ? Перенести функции очистки в common.h/common.c или tac.h/tac.c?
 * ? Пока оставил здесь, так как они связаны с 3AC.
 * ? Можно будет потом рефакторить.
 * 
 * ? @legon112
 * ? Так как DLL используется пока что только для 3AC, куда лучше поместить
 * ? функцию очистки TacInstruction?
 * 
 * Author:
 *     - Serhij Čepil (253038)
 */
#ifndef TAC_PRINTER_H
#define TAC_PRINTER_H

#include "common.h" 

/**
 * @brief Печатает содержимое списка 3AC в читаемом виде.
 * 
 * @param tac_list Заполненный список инструкций.
 */
void print_tac_list(DLList* tac_list);

/**
 * @brief Функция для DLL_Dispose для очистки памяти из-под TacInstruction.
 * (Вызывается из common.c / dll_list.c)
 * 
 * @param data Указатель на TacInstruction для освобождения.
 */
void free_tac_instruction(void* data);

#endif  // TAC_PRINTER_H
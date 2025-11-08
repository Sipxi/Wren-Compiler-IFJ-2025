/** @file common.h
 * 
 * ! Этот файл был полностью написан GEMINI,
 * ! УБРАТЬ ЕГО КОГДА СДЕЛАЕТСЯ ГОТОВАЯ ВЕРСИЯ DLL
 * 
 * 
 * (Исправленная версия)
 * Структуры и функции для двусвязного списка.
 */
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>



/**
 * Создает дубликат строки в динамически выделенной памяти.
 * Аналог стандартной функции strdup.
 *
 * @param s Исходная строка.
 * @return Указатель на дубликат строки.
 */
char* my_strdup(const char* s);


// Предварительное объявление 'struct DLLElement'
// (tag 'DLLElement')
struct DLLElement; 

// Определение DLLElementPtr как УКАЗАТЕЛЯ на 'struct DLLElement'
typedef struct DLLElement *DLLElementPtr;

// Определение самой СТРУКТУРЫ 'struct DLLElement'
// и 'typedef' для нее как 'DLLElement'
typedef struct DLLElement {
	void *data;
	DLLElementPtr prev_element;
	DLLElementPtr next_element;
} DLLElement; // <-- Теперь 'DLLElement' - это тип структуры, 
              // а 'DLLElementPtr' - тип указателя на нее.

// Определение самого списка
typedef struct {
  DLLElementPtr first_element;
  DLLElementPtr last_element;
  DLLElementPtr active_element;
  int current_length;
} DLList;

/**
 * @brief Внешняя функция для очистки данных, хранимых в списке.
 * Она должна быть определена где-то еще (e.g., в tac_playground.c)
 * Мы просто объявляем ее здесь, чтобы common.c мог ее использовать.
 */
void free_tac_instruction(void* data);

// Прототип функции ошибки
void DLL_Error(void);

// Прототипы функций списка
void DLL_Init(DLList* list);
void DLL_Dispose(DLList* list);
void DLL_InsertFirst(DLList* list, void *data);
void DLL_InsertLast(DLList* list, void *data);
void DLL_First(DLList* list);
void DLL_Last(DLList* list);
void DLL_GetFirst(DLList* list, void **data);
void DLL_GetLast(DLList* list, void **data);
void DLL_DeleteFirst(DLList* list);
void DLL_DeleteLast(DLList* list);
void DLL_DeleteAfter(DLList* list);
void DLL_DeleteBefore(DLList* list);
void DLL_InsertAfter(DLList* list, void *data);
void DLL_InsertBefore(DLList* list, void *data);
void DLL_GetValue(DLList* list, void **data);
void DLL_SetValue(DLList* list, void *data);
void DLL_Next(DLList* list);
void DLL_Previous(DLList* list);
bool DLL_IsActive(DLList* list);

#endif // COMMON_H
/**
 * scopestack.h
 *
 * Стек, который хранит указатели на ScopeMap (`ScopeMap*`)
 * Почему не использую stack.c? там только char, а мне нужны ScopeMap*
 * TODO: Не страдать хуйней и обьединить оба стека
 */

#ifndef SCOPESTACK_H
#define SCOPESTACK_H

#include "scopemap.h" // Нам нужен доступ к 'ScopeMap'
#include <stdbool.h>

#define MAX_STACK_DEPTH 100

/**
 *Структура стека областей видимости.
 */
typedef struct {
    ScopeMap **array;   // Массив указателей на ScopeMap
    int topIndex;       // Индекс верхнего элемента (-1 если пуст)
} ScopeStack;



//* --- Функции ---

/**
 * Инициализирует стек.
 *
 * @param stack Указатель на структуру ScopeStack (которая НЕ в куче).
 */
bool ScopeStack_Init(ScopeStack* stack);


/**
 * Освобождает *внутренний* массив стека.
 * Также вызывает scopemap_free() для всех ScopeMap'ов
 * в стеке.
 *
 * @param stack Указатель на инициализированный стек.
 */
void ScopeStack_Free(ScopeStack* stack);

// Пуст ли стек?
bool ScopeStack_IsEmpty(const ScopeStack* stack);

// Полон ли стек?
bool ScopeStack_IsFull(const ScopeStack* stack);

//Помещает ScopeMap на вершину стека
bool ScopeStack_Push(ScopeStack* stack, ScopeMap* map);

/**  Снимает и возвращает ScopeMap с вершины стека.
 * Эта функция *не* освобождает ScopeMap. Вызывающий должен сам вызвать `scopemap_free()` на
 * возвращенном указателе.
 * */
ScopeMap* ScopeStack_Pop(ScopeStack* stack);

//Возвращает ScopeMap с вершины стека, *не* снимая его.
ScopeMap* ScopeStack_Peek(const ScopeStack* stack);

/**
 *(КЛЮЧЕВАЯ ФУНКЦИЯ) Ищет переменную по имени,
 * двигаясь от вершины стека (текущая область) вниз.
 *
 * @param stack Стек.
 * @param key Простое имя для поиска (напр. "a").
 * @return TableEntry* если найдено, иначе NULL.
 */
TableEntry* ScopeStack_Find_Var(const ScopeStack* stack, const char* key);

#endif //SCOPESTACK_H
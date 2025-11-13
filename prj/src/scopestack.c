#include "scopestack.h"
#include <stdlib.h>
#include <stdio.h>

// Инициализирует стек.
bool ScopeStack_Init(ScopeStack* stack) {
    if (stack == NULL) {
        return false;
    }

    // Выделяем память для массива УКАЗАТЕЛЕЙ
    stack->array = (ScopeMap**)malloc(sizeof(ScopeMap*) * MAX_STACK_DEPTH);
    
    if (stack->array == NULL) {
        return false; 
    }

    stack->topIndex = -1;
    return true;
}


// Освобождает *внутренний* массив стека.
void ScopeStack_Free(ScopeStack* stack) {
    if (stack == NULL || stack->array == NULL) {
        return;
    }

    // Освобождаем каждый ScopeMap, который остался в стеке
    for (int i = 0; i <= stack->topIndex; i++) {
        scopemap_free(stack->array[i]);
    }

    // Освобождаем массив указателей
    free(stack->array);

    stack->array = NULL;
    stack->topIndex = -1;
}

// Пуст ли стек?
bool ScopeStack_IsEmpty(const ScopeStack* stack) {
    return stack->topIndex == -1;
}

// Полон ли стек?
bool ScopeStack_IsFull(const ScopeStack* stack) {
    return stack->topIndex == MAX_STACK_DEPTH - 1;
}


//Помещает ScopeMap на вершину стека
bool ScopeStack_Push(ScopeStack* stack, ScopeMap* map) {
    if (map == NULL) {
        return false;
    }

    // 1. Проверяем, не полон ли стек
    if (ScopeStack_IsFull(stack)) {
        // В отличии от stack.c я просто херачу еррор
        fprintf(stderr, "Semantic Error: Stack overflow (too many nested blocks).\n");
        return false; 
    }

    // 2. Вставляем элемент
    stack->topIndex++;
    stack->array[stack->topIndex] = map;
    return true;
}


// Снимает и возвращает ScopeMap с вершины стека.
ScopeMap* ScopeStack_Pop(ScopeStack* stack) {
    if (ScopeStack_IsEmpty(stack)) {
        return NULL; // Стек пуст
    }

    ScopeMap* map = stack->array[stack->topIndex];
    stack->topIndex--;
    return map;
}

//Возвращает ScopeMap с вершины стека, *не* снимая его.
ScopeMap* ScopeStack_Peek(const ScopeStack* stack) {
    if (ScopeStack_IsEmpty(stack)) {
        return NULL; // Стек пуст
    }
    return stack->array[stack->topIndex];
}


// Ищет переменную по имени, двигаясь от вершины стека вниз.
TableEntry* ScopeStack_Find_Var(const ScopeStack* stack, const char* key) {
    if (key == NULL) {
        return NULL;
    }

    // Идем от вершины (topIndex) вниз до 0
    for (int i = stack->topIndex; i >= 0; i--) {
        ScopeMap* current_map = stack->array[i];
        
        // Ищем в текущей ScopeMap
        TableEntry* entry = scopemap_lookup(current_map, key);
        if (entry != NULL) {
            return entry; // найдено
        }
    }

    return NULL; // не найдено
}
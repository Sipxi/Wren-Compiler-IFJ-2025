#include "stack_token.h"
#include <stdlib.h>

// Определение пустого токена
// Замена глобальной константы
// (Token) структура с типом TOKEN_UNDEFINED, 
// NULL данными и INVALID_INDEX номером строки
#define EMPTY_TOKEN (Token){TOKEN_UNDEFINED, NULL, INVALID_INDEX}


void Stack_Token_Init( Stack *stack ) {
    stack->array = (Token *)malloc( MAX_STACK * sizeof( Token ) );
    stack->topIndex = INVALID_INDEX;
}

bool Stack_Token_IsEmpty( const Stack *stack ) {
    return stack->topIndex == INVALID_INDEX;
}

Token Stack_Token_Top(const Stack *stack) {
    if (Stack_Token_IsEmpty(stack)) {
        
        return EMPTY_TOKEN; // Возвращаем пустой токен
    }
    return stack->array[stack->topIndex];
}

void Stack_Token_Pop(Stack *stack) {  
    if (Stack_Token_IsEmpty(stack)) {
        //! обработка ошибки: стек пустой
        return; // Возвращаем пустой токен
    }
    stack->topIndex--;
}

void Stack_Token_Push(Stack *stack, Token token) {
    // Проверка на переполнение стека
    // Если стек достиг максимального размера до добавления, 
    // обработать ошибку
    if (stack->topIndex >= MAX_STACK - 1) {
        //! обработка ошибки: стек переполнен
        return;
    }
    stack->topIndex++;
    stack->array[stack->topIndex] = token;
}

void Stack_Token_Dispose( Stack *stack ) {
    // Освободить память, выделяем её только для массива
    free( stack->array );
    stack->array = NULL;
    stack->topIndex = INVALID_INDEX;
}
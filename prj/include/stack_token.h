#ifndef STACK_H
#define STACK_H

#include "token.h"
#include <stdio.h>
#include <stdbool.h>

#define MAX_STACK 1000000

#define INVALID_INDEX -1

// int STACK_SIZE;

typedef struct {
	Token *array;

	int topIndex;
} Stack;

void Stack_Token_Init( Stack * );

bool Stack_Token_IsEmpty( const Stack * );
// Возвращает верхний элемент стека без его удаления
Token Stack_Token_Top(const Stack *stack);
// Удаляет верхний элемент стека
void Stack_Token_Pop(Stack *stack);  

void Stack_Token_Push(Stack *stack, Token token);

void Stack_Token_Dispose( Stack * );

#endif // STACK_H
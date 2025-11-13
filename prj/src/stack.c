#include "stack.h"
#include <stdlib.h>

int STACK_SIZE = MAX_STACK;
bool error_flag;
bool solved;

/**
 * Vytiskne upozornění, že došlo k chybě při práci se zásobníkem.
 *
 * TUTO FUNKCI, PROSÍME, NEUPRAVUJTE!
 *
 * @param error_code Interní identifikátor chyby
 */
void Stack_Error( int error_code ) {
	static const char *SERR_STRINGS[MAX_SERR + 1] = {
			"Unknown error",
			"Stack error: INIT",
			"Stack error: PUSH",
			"Stack error: TOP"
	};

	if (error_code <= 0 || error_code > MAX_SERR)
	{
		error_code = 0;
	}

	printf("%s\n", SERR_STRINGS[error_code]);
	error_flag = true;
}

/**
 * Provede inicializaci zásobníku - nastaví vrchol zásobníku.
 * Hodnoty v dynamickém poli neupravujte - po inicializaci
 * zásobníku jsou nedefinované.
 *
 * V případě, že funkce dostane jako parametr stack == NULL,
 * volejte funkci Stack_Error(SERR_INIT).
 * U ostatních funkcí pro zjednodušení předpokládejte, že tato situace
 * nenastane.
 *
 * @param stack Ukazatel na strukturu zásobníku
 */
void Stack_Init(Stack *stack) {
    if (stack == NULL) {
        Stack_Error(SERR_INIT);
        return;
    }

    // Аллоцируем массив указателей на Token
    stack->array = malloc(sizeof(Token *) * STACK_SIZE);
    if (stack->array == NULL) {
        Stack_Error(SERR_INIT);
        return;
    }

    stack->topIndex = -1;
}

/**
 * Vrací nenulovou hodnotu, je-li zásobník prázdný, jinak vrací hodnotu 0.
 * Funkci implementujte jako jediný příkaz.
 * Vyvarujte se zejména konstrukce typu "if ( cond ) b=true else b=false".
 *
 * @param stack Ukazatel na inicializovanou strukturu zásobníku
 *
 * @returns true v případě, že je zásobník prázdný, jinak false
 */
bool Stack_IsEmpty( const Stack *stack ) {
	return (stack->topIndex == -1);
}


/**
 * Vrací znak z vrcholu zásobníku prostřednictvím parametru dataPtr.
 * Tato operace ale prvek z vrcholu zásobníku neodstraňuje.
 * Volání operace Top při prázdném zásobníku je nekorektní a ošetřete ho voláním
 * procedury Stack_Error(SERR_TOP).
 *
 * Pro ověření, zda je zásobník prázdný, použijte dříve definovanou funkci
 * Stack_IsEmpty.
 *
 * @param stack Ukazatel na inicializovanou strukturu zásobníku
 * @param dataPtr Ukazatel na cílovou proměnnou
 */
Token *Stack_Top(const Stack *stack) {
    if (Stack_IsEmpty(stack)) {
        Stack_Error(SERR_TOP);
        return NULL;
    }

    return stack->array[stack->topIndex];
}


/**
 * Odstraní prvek z vrcholu zásobníku. Pro ověření, zda je zásobník prázdný,
 * použijte dříve definovanou funkci Stack_IsEmpty.
 *
 * Vyvolání operace Pop při prázdném zásobníku je sice podezřelé a může
 * signalizovat chybu v algoritmu, ve kterém je zásobník použit, ale funkci
 * pro ošetření chyby zde nevolejte (můžeme zásobník ponechat prázdný).
 * Spíše než volání chyby by se zde hodilo vypsat varování, což však pro
 * jednoduchost neděláme.
 *
 * @param stack Ukazatel na inicializovanou strukturu zásobníku
 */
Token *Stack_Pop(Stack *stack) {
    if (Stack_IsEmpty(stack)) {
        return NULL;
    }

    Token *top = stack->array[stack->topIndex];
    stack->topIndex--;
    return top;
}


/**
 * Vloží znak na vrchol zásobníku. Pokus o vložení prvku do plného zásobníku
 * je nekorektní a ošetřete ho voláním procedury Stack_Error(SERR_PUSH).
 *
 * Pro ověření, zda je zásobník plný, použijte dříve definovanou
 * funkci Stack_IsFull.
 *
 * @param stack Ukazatel na inicializovanou strukturu zásobníku
 * @param data Znak k vložení
 */
void Stack_Push(Stack *stack, Token *token) {
    if (stack->topIndex >= STACK_SIZE - 1) {
        Stack_Error(SERR_PUSH);
        return;
    }

    stack->topIndex++;
    stack->array[stack->topIndex] = token;
}


/**
 * Zruší a uvolní dynamicky alokované prostředky struktury.
 * Uvede zásobník do prázdného stavu.
 *
 * @param stack Ukazatel na inicializovanou strukturu zásobníku
 */
void Stack_Dispose(Stack *stack) {
    free(stack->array);
    stack->array = NULL;
    stack->topIndex = -1;
}


/**
 * @file stack_precedence.c
 * @team Tým 253038
 * @project Implementace překladače imperativního jazyka IFJ25 (varianta TRP-izp)
 * @year 2025
 *
 * @brief Implementace pro stack_precedence.h
 *
 * @author
 *     - Veronika Turbaievska (273123)
 */

#include "stack_precedence.h"
#include "utils.h" // pro strdup_c99

#include <stdlib.h> // pro malloc, realloc, free
#include <stdio.h>  // pro fprintf, stderr (pro případ chyb)
#include <assert.h> // pro ladicí kontroly

// Vychozí kapacita zásobníku
#define PSTACK_INITIAL_CAPACITY 8

/**
 * @brief Pomocná funkce pro rozšíření zásobníku.
 */
static bool PSTACK_resize(PStack *s) {
    // Pokud je kapacita 0, nastavíme výchozí, jinak zdvojnásobíme
    int new_capacity = (s->capacity == 0) ? PSTACK_INITIAL_CAPACITY : s->capacity * 2;
    
    PStackItem *new_items = realloc(s->items, sizeof(PStackItem) * new_capacity);
    
    if (new_items == NULL) {
        // Chyba: nepodařilo se alokovat paměť
        fprintf(stderr, "Chyba: nepodařilo se rozšířit zásobník parseru.\n");
        return false;
    }
    
    s->items = new_items;
    s->capacity = new_capacity;
    return true;
}


void PSTACK_init(PStack *s) {
    s->items = malloc(sizeof(PStackItem) * PSTACK_INITIAL_CAPACITY);
    
    if (s->items == NULL) {
        // Chyba alokace
        s->capacity = 0;
        s->top = -1;
        fprintf(stderr, "Chyba: nepodařilo se inicializovat zásobník parseru.\n");
    } else {
        s->capacity = PSTACK_INITIAL_CAPACITY;
        s->top = -1; // -1 znamená, že zásobník je prázdný
    }
}

void PSTACK_free(PStack *s) {
    if (s->items != NULL) {
        // Uvolňujeme data všech zbývajících tokenů ve zásobníku
        for (int i = 0; i <= s->top; i++) {
            if (s->items[i].token.data != NULL) {
                free(s->items[i].token.data);
            }
        }
        free(s->items);
    }
    s->items = NULL;
    s->capacity = 0;
    s->top = -1;
}

bool PSTACK_is_empty(PStack *s) {
    return s->top == -1;
}

bool PSTACK_push(PStack *s, PStackItem item) {
    // Zkontrolujeme, zda je místo na zásobníku
    if (s->top + 1 >= s->capacity) {
        // Pokud není místo, pokusíme se zásobník rozšířit
        if (!PSTACK_resize(s)) {
            return false; // Chyba rozšíření
        }
    }
    
    // Místo je, kopírujeme token s jeho daty
    s->top++;
    s->items[s->top] = item;
    
    // Kopírujeme data tokenu, pokud jsou přítomna
    if (item.token.data != NULL) {
        s->items[s->top].token.data = strdup_c99(item.token.data);
        if (s->items[s->top].token.data == NULL) {
            // Chyba alokace paměti, vracíme zpět
            s->top--;
            return false;
        }
    }
    
    return true;
}

PStackItem PSTACK_pop(PStack *s) {
    if (PSTACK_is_empty(s)) {
        // Chyba: pokus o pop z prázdného zásobníku
        fprintf(stderr, "Chyba: pokus o odebrání prvku z prázdného zásobníku parseru.\n");
        exit(99); // Nebo jiný vhodný kód chyby
    }
    PStackItem item = s->items[s->top--];
    
    // Uvolňujeme zkopírovaná data tokenu
    if (item.token.data != NULL) {
        free(item.token.data);
        item.token.data = NULL; // Nullujeme ukazatel pro bezpečnost
    }
    
    return item;
}

PStackItem PSTACK_top(PStack *s) {
    if (PSTACK_is_empty(s)) {
        // Chyba: pokus o peek z prázdného zásobníku
        fprintf(stderr, "Chyba: pokus o přístup k prvku z prázdného zásobníku parseru.\n");
        exit(99); // Nebo jiný vhodný kód chyby
    }
    return s->items[s->top];
}



// --- Specializované funkce ---

PStackItem *PSTACK_get_top_terminal(PStack *s) {
    if (PSTACK_is_empty(s)) {
        return NULL;
    }
    
    // Procházíme se od vrcholu dolů
    for (int i = s->top; i >= 0; i--) {
        // Získáme symbol
        GrammarSymbol sym = s->items[i].symbol;
        if (sym != GS_E && sym != GS_HANDLE_START) {
            // Našli jsme první symbol, který není neterminálem ani značkou rukojeti
            // - to je náš terminál.
            return &(s->items[i]);
        }
    }
    
    // Zásobník je prázdný nebo obsahuje pouze neterminály
    return NULL;
}

#include <stdlib.h> // для malloc, realloc, free
#include <stdio.h>  // для fprintf, stderr (на случай ошибок)
#include <assert.h> // для отладочных проверок

#include "stack_precedence.h"

// Начальная емкость стека по умолчанию
#define PSTACK_INITIAL_CAPACITY 8

/**
 * @brief Вспомогательная функция для расширения стека.
 */
static bool PSTACK_resize(PStack *s) {
    // Если емкость 0, ставим начальную, иначе удваиваем
    int new_capacity = (s->capacity == 0) ? PSTACK_INITIAL_CAPACITY : s->capacity * 2;
    
    PStackItem *new_items = realloc(s->items, sizeof(PStackItem) * new_capacity);
    
    if (new_items == NULL) {
        // Ошибка: не удалось выделить память
        fprintf(stderr, "Ошибка: не удалось расширить стек парсера.\n");
        return false; 
    }
    
    s->items = new_items;
    s->capacity = new_capacity;
    return true;
}


void PSTACK_init(PStack *s) {
    s->items = malloc(sizeof(PStackItem) * PSTACK_INITIAL_CAPACITY);
    
    if (s->items == NULL) {
        // Обработка ошибки, если malloc не сработал
        s->capacity = 0;
        s->top = -1;
        fprintf(stderr, "Ошибка: не удалось инициализировать стек парсера.\n");
    } else {
        s->capacity = PSTACK_INITIAL_CAPACITY;
        s->top = -1; // -1 означает, что стек пуст
    }
}

void PSTACK_free(PStack *s) {
    if (s->items != NULL) {
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
    // Проверяем, есть ли место
    if (s->top + 1 >= s->capacity) {
        // Если места нет, пытаемся расширить стек
        if (!PSTACK_resize(s)) {
            return false; // Ошибка расширения
        }
    }
    
    // Место есть, добавляем элемент
    s->top++;
    s->items[s->top] = item;
    return true;
}

PStackItem PSTACK_pop(PStack *s) {
    // В реальном коде стоит проверять на пустоту перед вызовом pop
    // assert(!PSTACK_is_empty(s)); 
    return s->items[s->top--];
}

PStackItem PSTACK_top(PStack *s) {
    // assert(!PSTACK_is_empty(s));
    return s->items[s->top];
}

void PSTACK_empty(PStack *s) {
    // Просто сбрасываем верхушку, не освобождая память
    s->top = -1; 
}


// --- Специализированные функции ---

PStackItem *PSTACK_get_top_terminal(PStack *s) {
    if (PSTACK_is_empty(s)) {
        return NULL;
    }
    
    // Идем сверху вниз
    for (int i = s->top; i >= 0; i--) {
        //
        // **ВАЖНОЕ ПРЕДПОЛОЖЕНИЕ:**
        // Я предполагаю, что в твоем "precedence.h" все *терминалы*
        // (GS_PLUS, GS_MINUS, GS_TERM, GS_DOLLAR и т.д.) 
        // имеют значения в enum, которые МЕНЬШЕ, чем у *нетерминалов* (GS_E)
        // и *специальных символов* (GS_HANDLE_START).
        //
        // Например, если GS_DOLLAR - последний терминал:
        // if (s->items[i].symbol <= GS_DOLLAR) {
        //
        // Если у тебя только один нетерминал (GS_E) и маркер (GS_HANDLE_START),
        // можно сделать и так:
        
        GrammarSymbol sym = s->items[i].symbol;
        if (sym != GS_E && sym != GS_HANDLE_START) {
            // Нашли первый символ, который не 'E' и не '<' 
            // - это и есть наш терминал.
            return &(s->items[i]);
        }
    }
    
    // Стек пуст или содержит только нетерминалы (что маловероятно)
    return NULL;
}



bool PSTACK_insert_handle_start(PStack *s) {
    int terminal_index = -1;
    
    // 1. Находим индекс самого верхнего терминала
    for (int i = s->top; i >= 0; i--) {
        GrammarSymbol sym = s->items[i].symbol;
        if (sym != GS_E && sym != GS_HANDLE_START) {
            terminal_index = i;
            break;
        }
    }
    
    if (terminal_index == -1) {
        // Этого не должно случиться, если парсер работает
        // (на стеке всегда должен быть хотя бы '$')
        fprintf(stderr, "Ошибка: не найден терминал для вставки рукоятки.\n");
        return false;
    }
    
    // 2. Проверяем, есть ли место для нового элемента '<'
    if (s->top + 1 >= s->capacity) {
        if (!PSTACK_resize(s)) {
            return false; // Ошибка расширения
        }
    }
    
    // 3. Сдвигаем все, что *выше* терминала, на одну позицию вверх
    // (это могут быть только нетерминалы 'E')
    for (int i = s->top; i > terminal_index; i--) {
        s->items[i + 1] = s->items[i];
    }
    
    // 4. Создаем и вставляем маркер начала рукоятки '<'
    PStackItem handle_item;
    handle_item.symbol = GS_HANDLE_START;
    handle_item.ast_node = NULL; 
    // .token можно не инициализировать, он не будет использоваться
    
    s->items[terminal_index + 1] = handle_item;
    
    // 5. Обновляем верхушку стека
    s->top++;
    
    return true;
}

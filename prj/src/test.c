#include "symtable.h"
#include <stdio.h>
/*
Игровая площадка для тестирования чего угодно
Пожалуйста, не удаляйте этот файл, он нам еще пригодится
Если хотите запустить этот файл:

make test-pg
*/

// Print token data safely, visualizing special characters
void print_token_data(const char *data) {
    for (const char *p = data; *p; p++) {
        switch (*p) {
            case '\n': printf("\\n"); break;
            case '\t': printf("\\t"); break;
            case '\r': printf("\\r"); break;
            default:   putchar(*p); break;
        }
    }
}

int main() {
    Symtable table;
    symtable_init(&table);
    symtable_print(&table);

    printf("Вставка символов...\n");
    SymbolData data1 = {KIND_VAR, TYPE_INT, true, NULL};
    symtable_insert(&table, "var1", &data1);
    SymbolData data2 = {KIND_FUNC, TYPE_INT, false, NULL};
    symtable_insert(&table, "func1", &data2);



    symtable_print(&table);

    printf("Удаление символа 'var1'...\n");
    symtable_delete(&table, "var1");
    symtable_print(&table);

    printf("Проверка переполнения таблицы...\n");
    for (int i = 0; i < 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "var%d", i);
        SymbolData data = {KIND_VAR, TYPE_INT, true, NULL};
        if (!symtable_insert(&table, key, &data)) {
            printf("Ошибка вставки символа '%s'\n", key);
        }
    }
    symtable_print(&table);

    printf("Проверка вставление существующего символа...\n");
    SymbolData data = {KIND_VAR, TYPE_INT, true, NULL};
    symtable_insert(&table, "func1", &data); // Перезапись существующего

    symtable_print(&table);
    symtable_free(&table);
    return 0;
}

#include "symtable.h"
#include "lexer.h"
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
void test_symtable() {
    
    Symtable table;
    symtable_init(&table, 0);
    symtable_print(&table);

    printf("Вставка символов...\n");
    Symtable local_table;
    symtable_init(&local_table, 1);
    Symtable local_local_table;
    symtable_init(&local_local_table, 2);
    SymbolData data_local = {KIND_FUNC, TYPE_NUM, true, &local_local_table};
    symtable_insert(&local_table, "local_var", &data_local);
    SymbolData data1 = {KIND_VAR, TYPE_NUM, true, &local_table};
    symtable_insert(&table, "var1", &data1);
    SymbolData data2 = {KIND_FUNC, TYPE_NUM, false, NULL};
    symtable_insert(&table, "func1", &data2);



    symtable_print(&table);

    printf("Удаление символа 'var1'...\n");
    symtable_delete(&table, "var1");
    symtable_print(&table);

    printf("Проверка переполнения таблицы...\n");
    for (int i = 0; i < 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "var%d", i);
        SymbolData data = {KIND_VAR, TYPE_NUM, true, NULL};
        if (!symtable_insert(&table, key, &data)) {
            printf("Ошибка вставки символа '%s'\n", key);
        }
    }
    symtable_print(&table);

    printf("Проверка вставки существующего символа...\n");
    SymbolData data = {KIND_VAR, TYPE_NUM, true, NULL};
    symtable_insert(&table, "func1", &data); // Перезапись существующего

    symtable_print(&table);
    symtable_free(&table);
}

// void test_3AC(){
    
//     printf("--- 3AC Generation Playground ---\n");

//     // 1. Инициализация
//     Symtable global_table;
//     if (!symtable_init(&global_table)) {
//         fprintf(stderr, "Failed to init symtable.\n");
//         return 1;
//     }

//     DLList tac_list;
//     DLL_Init(&tac_list);

//     // 2. Создаем AST и заполняем Symtable
//     // (Это делают парсер и семантика)
//     printf("1. Building Sample AST and Symtable...\n");
//     AstNode *ast_root = create_sample_program(&global_table);
    
//     printf("\n--- Symbol Table (After semantic pass) ---\n");
//     symtable_print(&global_table);

//     // 3. Генерируем 3AC
//     printf("\n2. Generating 3-Address Code...\n");
//     generate_tac(&tac_list, ast_root, &global_table);

//     // 4. Печатаем результат
//     printf("\n--- Generated 3-Address Code (Quadruples) ---\n");
//     print_tac_list(&tac_list);
//     printf("-------------------------------------------------\n");

//     // 5. Очистка
//     printf("\n3. Cleaning up resources...\n");
//     symtable_free(&global_table);
//     DLL_Dispose(&tac_list); // Это вызовет free_tac_instruction для каждого элемента
//     free_ast(ast_root);

//     printf("Done.\n");
// }

int test_lexer(){
    // Use stdin for input (supports redirection like: ./test < input.wren)
    FILE *file = fopen("example.wren", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return 1;
    }

    Lexer *lexer = lexer_init();
    if (lexer == NULL) {
        fprintf(stderr, "Error initializing lexer.\n");
        fclose(file);
        return 1;
    }
    while (lexer->current_token->type != TOKEN_EOF) {
        get_next_token(lexer, file);

        printf("Token Type: %s, Data: ",
               token_type_to_string(lexer->current_token->type));
        
        print_token_data(lexer->current_token->data);

        printf(", Line: %d\n", lexer->current_token->line);
    }
    // Don't close stdin
    lexer_free(lexer);
    if (fclose(file) != 0) { // обработка ошибки закрытия файла
        fprintf(stderr, "Error closing file.\n");
    }
    return 0;
}
int main() {
    int result = 0;
    test_symtable();
    // int result = test_lexer();
    return result;
}
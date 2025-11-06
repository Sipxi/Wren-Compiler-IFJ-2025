/* test.c
 * Главный тестовый файл.
 */
#include <stdio.h>
#include "symtable.h"
#include "common.h"
#include "sample_ast.h"
#include "tac_playground.h"

int main() {
    printf("--- 3AC Generation Playground ---\n");

    // 1. Инициализация
    Symtable global_table;
    if (!symtable_init(&global_table)) {
        fprintf(stderr, "Failed to init symtable.\n");
        return 1;
    }

    DLList tac_list;
    DLL_Init(&tac_list);

    // 2. Создаем AST и заполняем Symtable
    // (Это делают парсер и семантика)
    printf("1. Building Sample AST and Symtable...\n");
    AstNode *ast_root = create_sample_program(&global_table);
    
    printf("\n--- Symbol Table (After semantic pass) ---\n");
    symtable_print(&global_table);

    // 3. Генерируем 3AC
    printf("\n2. Generating 3-Address Code...\n");
    generate_tac(&tac_list, ast_root, &global_table);

    // 4. Печатаем результат
    printf("\n--- Generated 3-Address Code (Quadruples) ---\n");
    print_tac_list(&tac_list);
    printf("-------------------------------------------------\n");

    // 5. Очистка
    printf("\n3. Cleaning up resources...\n");
    symtable_free(&global_table);
    DLL_Dispose(&tac_list); // Это вызовет free_tac_instruction для каждого элемента
    free_ast(ast_root);

    printf("Done.\n");

    return 0;
}
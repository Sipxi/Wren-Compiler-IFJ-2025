#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"          // Твой ast.h
#include "common.h"       // Твой DLL
#include "symtable.h"     // Твоя хеш-таблица
#include "tac.h"          // Твой tac.h
#include "tac_printer.h"  // Мой принтер
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


 // === Вспомогательные функции для создания AST ===
 // (Прототипы из ast.c, чтобы test.c их "видел")
AstNode *ast_new_id_node(NodeType type, int line, const char *id,
    TableEntry *entry);
AstNode *ast_new_num_node(double value, int line);
AstNode *ast_new_bin_op(NodeType type, int line, AstNode *left, AstNode *right);

// Вспомогательная функция для регистрации переменной в symtable
static TableEntry *define_symbol(Symtable *table, const char *name,
    SymbolKind kind) {
    SymbolData *data = (SymbolData *)calloc(1, sizeof(SymbolData));
    data->kind = kind;

    data->data_type =
        (kind == KIND_VAR) ? TYPE_NIL : TYPE_NUM;  // Просто для примера
    data->is_defined = true;

    if (!symtable_insert(table, name, data)) {
        fprintf(stderr, "Failed to insert '%s' into symtable.\n", name);
        exit(1);
    }
    return symtable_lookup(table, name);
}

/**
 * @brief Строит фейковый AST для кода:
 * static main() {
 * var a
 * a = 10
 * var b
 * b = a + 5
 * }
 */
static AstNode *create_test_ast(Symtable *global_table) {
    printf("1. Building Fake AST and Symtable...\n");

    // --- 1. Симуляция семантики: заполняем Symtable ---
    TableEntry *func_main = define_symbol(global_table, "main", KIND_FUNC);
    TableEntry *var_a = define_symbol(global_table, "a", KIND_VAR);
    TableEntry *var_b = define_symbol(global_table, "b", KIND_VAR);

    // --- 2. Строим AST (снизу вверх) ---

    // a = 10
    AstNode *assign1 = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assign1, ast_new_id_node(NODE_ID, 3, "a", var_a));
    ast_node_add_child(assign1, ast_new_num_node(10.0, 3));

    // a + 5
    AstNode *op_plus =
        ast_new_bin_op(NODE_OP_PLUS, 5, ast_new_id_node(NODE_ID, 5, "a", var_a),
            ast_new_num_node(5.0, 5));

    // b = a + 5
    AstNode *assign2 = ast_node_create(NODE_ASSIGNMENT, 5);
    ast_node_add_child(assign2, ast_new_id_node(NODE_ID, 5, "b", var_b));
    ast_node_add_child(assign2, op_plus);

    // --- 3. Собираем стейтменты в блок ---
    AstNode *block = ast_node_create(NODE_BLOCK, 1);
    ast_node_add_child(block, ast_new_id_node(NODE_VAR_DEF, 2, "a", var_a));
    ast_node_add_child(block, assign1);
    ast_node_add_child(block, ast_new_id_node(NODE_VAR_DEF, 4, "b", var_b));
    ast_node_add_child(block, assign2);

    // --- 4. Собираем функцию ---
    AstNode *func_def =
        ast_new_id_node(NODE_FUNCTION_DEF, 1, "main", func_main);
    ast_node_add_child(func_def,
        ast_node_create(NODE_PARAM_LIST, 1));  // Пустой список
    ast_node_add_child(func_def, block);                      // Тело функции

    // --- 5. Собираем программу ---
    AstNode *program = ast_node_create(NODE_PROGRAM, 0);
    ast_node_add_child(program, func_def);

    printf("   ...AST Built.\n");
    return program;
}

// Начало
void test_3AC() {
    printf("--- 3AC Generator Test ---\n");

    // 1. Инициализация
    Symtable global_table;
    symtable_init(&global_table);

    DLList tac_list;
    DLL_Init(&tac_list);

    // 2. Создаем AST и заполняем Symtable
    AstNode *ast_root = create_test_ast(&global_table);

    printf("\n--- Symbol Table (Simulated Pass 2) ---\n");
    symtable_print(&global_table);  // (Если у тебя есть эта функция)

    // 3. === ЗАПУСКАЕМ ТВОЙ ГЕНЕРАТОР ===
    printf("\n2. Calling generate_tac()...\n");
    generate_tac(ast_root, &tac_list, &global_table);
    printf("   ...generate_tac() finished.\n");

    // 4. Печатаем результат
    print_tac_list(&tac_list);

    // 5. Очистка
    printf("\n3. Cleaning up resources...\n");
    ast_node_free_recursive(ast_root);
    symtable_free(&global_table);
    DLL_Dispose(&tac_list);  // Это вызовет free_tac_instruction

    printf("Done.\n");
}
// Конец

// === ГЛАВНАЯ ТЕСТОВАЯ ФУНКЦИЯ ===

int main() {
    int result = 0;
    test_symtable();
    // int result = test_lexer();
    return result;
}
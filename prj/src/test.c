/**
 * @file test.c
 * Главный тестовый файл для проекта IFJ-2025.
 */

#include "ast.h"
#include "dll.h"
#include "symtable.h" 
#include "tac.h"      
#include "printer.h"  
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

 /*=======================================*/
 /*===== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ========*/
 /*=======================================*/

 /**
  * Вспомогательная функция для определения символа для симуляции семантики.
  *
  * @param table Таблица символов.
  * @param name Имя символа.
  * @param kind Вид символа (переменная, функция и т.д).
  * @return Указатель на созданную запись таблицы символов.
  * @note Вызывает exit(1) при ошибке вставки.
  */
static TableEntry *define_symbol(Symtable *table, const char *name,
    SymbolKind kind) {
    SymbolData *data = (SymbolData *)calloc(1, sizeof(SymbolData));
    data->kind = kind;
    data->data_type = (kind == KIND_VAR) ? TYPE_NIL : TYPE_NUM; // По умолчанию
    data->is_defined = true;

    if (!symtable_insert(table, name, data)) {
        fprintf(stderr, "Failed to insert '%s' into symtable.\n", name);
        free(data); // symtable не завладел 'data', чистим
        exit(EXIT_FAILURE);
    }

    // Free data after successful insertion, since symtable makes its own copy
    free(data);
    return symtable_lookup(table, name);
}


/**
 * @brief Строит фейковый AST для СТРЕСС-ТЕСТА
 * (Тест для генерации большого кол-ва $tN)
 */
static AstNode *create_test_ast(Symtable *global_table) {
    printf("1. Building Fake AST and Symtable...\n");

    // --- 1. (СИМУЛЯЦИЯ Pass 2) ---
    // Определяем все символы (функции и переменные)
    TableEntry *func_main = define_symbol(global_table, "main", KIND_FUNC);
    TableEntry *func_a = define_symbol(global_table, "func_A", KIND_FUNC);
    TableEntry *func_b = define_symbol(global_table, "func_B", KIND_FUNC);
    TableEntry *var_a = define_symbol(global_table, "a", KIND_VAR);
    TableEntry *var_b = define_symbol(global_table, "b", KIND_VAR);
    TableEntry *param_p1 = define_symbol(global_table, "p1", KIND_VAR);
    TableEntry *param_p2 = define_symbol(global_table, "p2", KIND_VAR);


    // --- 2. Строим func_A (Тут "много тн" #1) ---
    AstNode *func_def_a = ast_new_id_node(NODE_FUNCTION_DEF, 1, "func_A");
    func_def_a->table_entry = func_a; // (Pass 2)
    {
        AstNode *param_list = ast_node_create(NODE_PARAM_LIST, 1);
        AstNode *p1_node = ast_new_id_node(NODE_PARAM, 1, "p1");
        p1_node->table_entry = param_p1; // (Pass 2)
        AstNode *p2_node = ast_new_id_node(NODE_PARAM, 1, "p2");
        p2_node->table_entry = param_p2; // (Pass 2)
        ast_node_add_child(param_list, p1_node);
        ast_node_add_child(param_list, p2_node);

        AstNode *func_body = ast_node_create(NODE_BLOCK, 2);

        // return (p1 + 1) * (p2 + 2);
        AstNode *p1_id = ast_new_id_node(NODE_ID, 2, "p1");
        p1_id->table_entry = param_p1; // (Pass 2)
        AstNode *p2_id = ast_new_id_node(NODE_ID, 2, "p2");
        p2_id->table_entry = param_p2; // (Pass 2)

        AstNode *plus_1 = ast_new_bin_op(NODE_OP_PLUS, 2, p1_id, ast_new_num_node(1.0, 2));
        AstNode *plus_2 = ast_new_bin_op(NODE_OP_PLUS, 2, p2_id, ast_new_num_node(2.0, 2));
        AstNode *op_mul = ast_new_bin_op(NODE_OP_MUL, 2, plus_1, plus_2);

        AstNode *return_stmt = ast_node_create(NODE_RETURN, 2);
        ast_node_add_child(return_stmt, op_mul);
        ast_node_add_child(func_body, return_stmt);

        ast_node_add_child(func_def_a, param_list);
        ast_node_add_child(func_def_a, func_body);
    }

    // --- 3. Строим func_B (Тут "много тн" #2) ---
    AstNode *func_def_b = ast_new_id_node(NODE_FUNCTION_DEF, 5, "func_B");
    func_def_b->table_entry = func_b; // (Pass 2)
    {
        AstNode *param_list = ast_node_create(NODE_PARAM_LIST, 5);
        AstNode *p1_node = ast_new_id_node(NODE_PARAM, 5, "p1");
        p1_node->table_entry = param_p1; // (Pass 2)
        AstNode *p2_node = ast_new_id_node(NODE_PARAM, 5, "p2");
        p2_node->table_entry = param_p2; // (Pass 2)
        ast_node_add_child(param_list, p1_node);
        ast_node_add_child(param_list, p2_node);

        AstNode *func_body = ast_node_create(NODE_BLOCK, 6);

        // return (p1 / 3) - (p2 / 4);
        AstNode *p1_id = ast_new_id_node(NODE_ID, 6, "p1");
        p1_id->table_entry = param_p1; // (Pass 2)
        AstNode *p2_id = ast_new_id_node(NODE_ID, 6, "p2");
        p2_id->table_entry = param_p2; // (Pass 2)

        AstNode *div_1 = ast_new_bin_op(NODE_OP_DIV, 6, p1_id, ast_new_num_node(3.0, 6));
        AstNode *div_2 = ast_new_bin_op(NODE_OP_DIV, 6, p2_id, ast_new_num_node(4.0, 6));
        AstNode *op_sub = ast_new_bin_op(NODE_OP_MINUS, 6, div_1, div_2);

        AstNode *return_stmt = ast_node_create(NODE_RETURN, 6);
        ast_node_add_child(return_stmt, op_sub);
        ast_node_add_child(func_body, return_stmt);

        ast_node_add_child(func_def_b, param_list);
        ast_node_add_child(func_def_b, func_body);
    }

    // --- 4. Строим 'main()' (Тут "много тн" #3, #4, #5, #6) ---
    AstNode *main_def = ast_new_id_node(NODE_FUNCTION_DEF, 10, "main");
    main_def->table_entry = func_main; // (Pass 2)
    {
        AstNode *main_block = ast_node_create(NODE_BLOCK, 10);

        AstNode *def_a = ast_new_id_node(NODE_VAR_DEF, 11, "a");
        def_a->table_entry = var_a; // (Pass 2)
        ast_node_add_child(main_block, def_a);

        AstNode *def_b = ast_new_id_node(NODE_VAR_DEF, 12, "b");
        def_b->table_entry = var_b; // (Pass 2)
        ast_node_add_child(main_block, def_b);

        // --- "много тн" #3 ---
        // a = (10 + 20) * (30 + 40)
        AstNode *assign_a = ast_node_create(NODE_ASSIGNMENT, 13);
        AstNode *a_id_1 = ast_new_id_node(NODE_ID, 13, "a");
        a_id_1->table_entry = var_a; // (Pass 2)

        AstNode *plus_1 = ast_new_bin_op(NODE_OP_PLUS, 13, ast_new_num_node(10.0, 13), ast_new_num_node(20.0, 13));
        AstNode *plus_2 = ast_new_bin_op(NODE_OP_PLUS, 13, ast_new_num_node(30.0, 13), ast_new_num_node(40.0, 13));
        AstNode *op_mul = ast_new_bin_op(NODE_OP_MUL, 13, plus_1, plus_2);

        ast_node_add_child(assign_a, a_id_1);
        ast_node_add_child(assign_a, op_mul);
        ast_node_add_child(main_block, assign_a);

        // --- "много тн" #4 ---
        // b = (100 - 50) / (90 - 80)
        AstNode *assign_b = ast_node_create(NODE_ASSIGNMENT, 14);
        AstNode *b_id_1 = ast_new_id_node(NODE_ID, 14, "b");
        b_id_1->table_entry = var_b; // (Pass 2)

        AstNode *sub_1 = ast_new_bin_op(NODE_OP_MINUS, 14, ast_new_num_node(100.0, 14), ast_new_num_node(50.0, 14));
        AstNode *sub_2 = ast_new_bin_op(NODE_OP_MINUS, 14, ast_new_num_node(90.0, 14), ast_new_num_node(80.0, 14));
        AstNode *op_div = ast_new_bin_op(NODE_OP_DIV, 14, sub_1, sub_2);

        ast_node_add_child(assign_b, b_id_1);
        ast_node_add_child(assign_b, op_div);
        ast_node_add_child(main_block, assign_b);

        // --- "много тн" #5 ---
        // if ((a + b) > (100 + 200)) { a = 1 } else { b = 2 }
        AstNode *if_stmt = ast_node_create(NODE_IF, 15);
        {
            AstNode *a_id_2 = ast_new_id_node(NODE_ID, 15, "a");
            a_id_2->table_entry = var_a; // (Pass 2)
            AstNode *b_id_2 = ast_new_id_node(NODE_ID, 15, "b");
            b_id_2->table_entry = var_b; // (Pass 2)

            AstNode *plus_cond_1 = ast_new_bin_op(NODE_OP_PLUS, 15, a_id_2, b_id_2);
            AstNode *plus_cond_2 = ast_new_bin_op(NODE_OP_PLUS, 15, ast_new_num_node(100.0, 15), ast_new_num_node(200.0, 15));
            AstNode *cond_gt = ast_new_bin_op(NODE_OP_GT, 15, plus_cond_1, plus_cond_2);

            AstNode *then_block = ast_node_create(NODE_BLOCK, 16);
            AstNode *assign_a_1 = ast_node_create(NODE_ASSIGNMENT, 16);
            AstNode *a_id_3 = ast_new_id_node(NODE_ID, 16, "a");
            a_id_3->table_entry = var_a; // (Pass 2)
            ast_node_add_child(assign_a_1, a_id_3);
            ast_node_add_child(assign_a_1, ast_new_num_node(1.0, 16));
            ast_node_add_child(then_block, assign_a_1);

            AstNode *else_block = ast_node_create(NODE_BLOCK, 18);
            AstNode *assign_b_2 = ast_node_create(NODE_ASSIGNMENT, 18);
            AstNode *b_id_3 = ast_new_id_node(NODE_ID, 18, "b");
            b_id_3->table_entry = var_b; // (Pass 2)
            ast_node_add_child(assign_b_2, b_id_3);
            ast_node_add_child(assign_b_2, ast_new_num_node(2.0, 18));
            ast_node_add_child(else_block, assign_b_2);

            ast_node_add_child(if_stmt, cond_gt);
            ast_node_add_child(if_stmt, then_block);
            ast_node_add_child(if_stmt, else_block);
        }
        ast_node_add_child(main_block, if_stmt);

        // --- "много тн" #6 ---
        // func_A(a + 1, b + 2)
        AstNode *call_stmt = ast_node_create(NODE_CALL_STATEMENT, 20);
        AstNode *func_id = ast_new_id_node(NODE_ID, 20, "func_A");
        func_id->table_entry = func_a; // (Pass 2)
        ast_node_add_child(call_stmt, func_id);

        AstNode *arg_list = ast_node_create(NODE_ARGUMENT_LIST, 20);

        AstNode *a_id_4 = ast_new_id_node(NODE_ID, 20, "a");
        a_id_4->table_entry = var_a; // (Pass 2)
        AstNode *b_id_4 = ast_new_id_node(NODE_ID, 20, "b");
        b_id_4->table_entry = var_b; // (Pass 2)

        ast_node_add_child(arg_list, ast_new_bin_op(NODE_OP_PLUS, 20, a_id_4, ast_new_num_node(1.0, 20)));
        ast_node_add_child(arg_list, ast_new_bin_op(NODE_OP_PLUS, 20, b_id_4, ast_new_num_node(2.0, 20)));

        ast_node_add_child(call_stmt, arg_list);
        ast_node_add_child(main_block, call_stmt);

        // --- Собираем 'main' ---
        ast_node_add_child(main_def, ast_node_create(NODE_PARAM_LIST, 10)); // Пустой список
        ast_node_add_child(main_def, main_block); // Тело
    }

    // --- 5. Собираем программу ---
    AstNode *program = ast_node_create(NODE_PROGRAM, 0);
    ast_node_add_child(program, func_def_a); // Добавляем func_A
    ast_node_add_child(program, func_def_b); // Добавляем func_B
    ast_node_add_child(program, main_def);   // Добавляем main

    printf("   ...AST Built (Stress Test for $tN).\n");
    return program;
}

/**
 * @brief Печатает данные токена, экранируя специальные символы.
 * @param data Данные токена (строка).
 */
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

/*=======================================*/
// === ТЕСТОВЫЕ ФУНКЦИИ ===
/*=======================================*/

void test_symtable() {

    Symtable table;
    symtable_init(&table);
    symtable_print(&table);

    printf("Вставка символов...\n");
    Symtable local_table;
    symtable_init(&local_table);
    Symtable local_local_table;
    symtable_init(&local_local_table);
    SymbolData data_local = { KIND_FUNC, TYPE_NUM, true, &local_local_table };
    symtable_insert(&local_table, "local_var", &data_local);
    SymbolData data1 = { KIND_VAR, TYPE_NUM, true, &local_table };
    symtable_insert(&table, "var1", &data1);
    SymbolData data2 = { KIND_FUNC, TYPE_NUM, false, NULL };
    symtable_insert(&table, "func1", &data2);



    symtable_print(&table);

    printf("Удаление символа 'var1'...\n");
    symtable_delete(&table, "var1");
    symtable_print(&table);

    printf("Проверка переполнения таблицы...\n");
    for (int i = 0; i < 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "var%d", i);
        SymbolData data = { KIND_VAR, TYPE_NUM, true, NULL };
        if (!symtable_insert(&table, key, &data)) {
            printf("Ошибка вставки символа '%s'\n", key);
        }
    }
    symtable_print(&table);

    printf("Проверка вставки существующего символа...\n");
    SymbolData data = { KIND_VAR, TYPE_NUM, true, NULL };
    symtable_insert(&table, "func1", &data); // Перезапись существующего

    symtable_print(&table);
    symtable_free(&table);
}

int test_lexer() {
    // Use stdin for input (supports redirection like: ./test < input.wren)
    FILE *file = fopen("example.wren", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return EXIT_FAILURE;
    }

    Lexer *lexer = lexer_init();
    if (lexer == NULL) {
        fprintf(stderr, "Error initializing lexer.\n");
        fclose(file);
        return EXIT_FAILURE;
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
    if (fclose(file) != EXIT_SUCCESS) { // обработка ошибки закрытия файла
        fprintf(stderr, "Error closing file.\n");
    }
    return EXIT_SUCCESS;
}

void test_tac_generator() {

    printf("--- 3AC Generator Test ---\n");

    // 1. Инициализация
    Symtable global_table;
    symtable_init(&global_table);

    DLList tac_list;
    DLL_Init(&tac_list);

    // 2. Создаем AST и заполняем Symtable
    AstNode *ast_root = create_test_ast(&global_table);

    // Раскомментируй, если у тебя есть symtable_print
    // printf("\n--- Symbol Table (Simulated Pass 2) ---\n");
    // symtable_print(&global_table);

    // 3. === ЗАПУСКАЕМ ТВОЙ ГЕНЕРАТОР ===
    printf("\n2. Calling generate_tac()...\n");
    generate_tac(ast_root, &tac_list, &global_table);
    printf("   ...generate_tac() finished.\n");

    // 4. Печатаем результат
    // (Убедись, что у тебя есть 'printer.c' и 'printer.h' с этой функцией)
    printf("\n--- Generated 3-Address Code ---\n");
    print_tac_list(&tac_list);

    // 5. Очистка
    printf("\n3. Cleaning up resources...\n");
    ast_node_free_recursive(ast_root);
    symtable_free(&global_table);
    DLL_Dispose(&tac_list); // Это вызовет free_tac_instruction

    printf("Done.\n");

}

/*=======================================*/
// === ГЛАВНАЯ ФУНКЦИЯ ===
/*=======================================*/

int main() {
    printf("=== IFJ-2025 Test Suite ===\n\n");

    test_tac_generator();

    return EXIT_SUCCESS;
}
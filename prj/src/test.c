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
#include "optimizer.h"

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
 * @brief Строит фейковый AST для кода:
 * (Тест для Peephole Jump-NOP)
 *
 * // 1. Тест "Пустой 'else'" (Контроль)
 * static func_empty_else(a, b) {
 * if (a > b) { a = 1; } else { }
 * }
 *
 * // 2. Тест "Пустой 'if'" (Контроль)
 * static func_empty_if(a, b) {
 * if (a > b) { } else { b = 2; }
 * }
 *
 * // 3. Тест "Полный" (Контроль)
 * static func_full(a, b) {
 * if (a > b) { a = 1; } else { b = 2; }
 * }
 *
 * // 4. НАСТОЯЩИЙ ТЕСТ (Пустой 'then', нет 'else')
 * static func_nop_jump(a, b) {
 * if (a > b) { }
 * }
 *
 */
static AstNode *create_test_ast(Symtable *global_table) {
    printf("1. Building Fake AST and Symtable...\n");

    // --- 1. (СИМУЛЯЦИЯ Pass 2) ---
    TableEntry *func_main = define_symbol(global_table, "main", KIND_FUNC);
    TableEntry *func_ee = define_symbol(global_table, "func_empty_else", KIND_FUNC);
    TableEntry *func_ei = define_symbol(global_table, "func_empty_if", KIND_FUNC);
    TableEntry *func_f = define_symbol(global_table, "func_full", KIND_FUNC);
    TableEntry *func_nop = define_symbol(global_table, "func_nop_jump", KIND_FUNC);
    // (Переменные для 'main', чтобы было что передавать)
    TableEntry *var_a = define_symbol(global_table, "a", KIND_VAR);
    TableEntry *var_b = define_symbol(global_table, "b", KIND_VAR);
    // (Параметры для функций)
    TableEntry *param_p1 = define_symbol(global_table, "p1", KIND_VAR);
    TableEntry *param_p2 = define_symbol(global_table, "p2", KIND_VAR);

    AstNode *program = ast_node_create(NODE_PROGRAM, 0);

    // --- 2. 'main()' (просто вызывает остальные) ---
    AstNode *main_def = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    main_def->table_entry = func_main; // (Pass 2)
    ast_node_add_child(program, main_def);
    {
        ast_node_add_child(main_def, ast_node_create(NODE_PARAM_LIST, 1));
        ast_node_add_child(main_def, ast_node_create(NODE_BLOCK, 1)); // Пустое тело
    }

    // --- 3. 'func_empty_else' (Пустой else) ---
    AstNode *func_def_ee = ast_new_id_node(NODE_FUNCTION_DEF, 10, "func_empty_else");
    func_def_ee->table_entry = func_ee; // (Pass 2)
    ast_node_add_child(program, func_def_ee);
    {
        // Параметры
        AstNode *param_list = ast_node_create(NODE_PARAM_LIST, 10);
        ast_node_add_child(param_list, ast_new_id_node(NODE_PARAM, 10, "p1"));
        ast_node_add_child(param_list, ast_new_id_node(NODE_PARAM, 10, "p2"));
        param_list->child->table_entry = param_p1; // (Pass 2)
        param_list->child->sibling->table_entry = param_p2; // (Pass 2)
        ast_node_add_child(func_def_ee, param_list);
        
        // Тело
        AstNode *body = ast_node_create(NODE_BLOCK, 10);
        AstNode *if_stmt = ast_node_create(NODE_IF, 11);
        
        // (cond: p1 > p2)
        AstNode* p1_id_1 = ast_new_id_node(NODE_ID, 11, "p1");
        p1_id_1->table_entry = param_p1; // (Pass 2)
        AstNode* p2_id_1 = ast_new_id_node(NODE_ID, 11, "p2");
        p2_id_1->table_entry = param_p2; // (Pass 2)
        ast_node_add_child(if_stmt, ast_new_bin_op(NODE_OP_GT, 11, p1_id_1, p2_id_1));
        
        // (then: { p1 = 1; })
        AstNode *then_block = ast_node_create(NODE_BLOCK, 12);
        AstNode *assign_p1 = ast_node_create(NODE_ASSIGNMENT, 12);
        AstNode* p1_id_2 = ast_new_id_node(NODE_ID, 12, "p1");
        p1_id_2->table_entry = param_p1; // (Pass 2)
        ast_node_add_child(assign_p1, p1_id_2);
        ast_node_add_child(assign_p1, ast_new_num_node(1.0, 12));
        ast_node_add_child(then_block, assign_p1);
        ast_node_add_child(if_stmt, then_block);

        // (else: { })
        ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 14));
        
        ast_node_add_child(body, if_stmt);
        ast_node_add_child(func_def_ee, body);
    }

    // --- 4. 'func_empty_if' (Пустой if) ---
    AstNode *func_def_ei = ast_new_id_node(NODE_FUNCTION_DEF, 20, "func_empty_if");
    func_def_ei->table_entry = func_ei; // (Pass 2)
    ast_node_add_child(program, func_def_ei);
    {
        // Параметры
        AstNode *param_list = ast_node_create(NODE_PARAM_LIST, 20);
        ast_node_add_child(param_list, ast_new_id_node(NODE_PARAM, 20, "p1"));
        ast_node_add_child(param_list, ast_new_id_node(NODE_PARAM, 20, "p2"));
        param_list->child->table_entry = param_p1; // (Pass 2)
        param_list->child->sibling->table_entry = param_p2; // (Pass 2)
        ast_node_add_child(func_def_ei, param_list);
        
        // Тело
        AstNode *body = ast_node_create(NODE_BLOCK, 20);
        AstNode *if_stmt = ast_node_create(NODE_IF, 21);
        
        // (cond: p1 > p2)
        AstNode* p1_id_1 = ast_new_id_node(NODE_ID, 21, "p1");
        p1_id_1->table_entry = param_p1; // (Pass 2)
        AstNode* p2_id_1 = ast_new_id_node(NODE_ID, 21, "p2");
        p2_id_1->table_entry = param_p2; // (Pass 2)
        ast_node_add_child(if_stmt, ast_new_bin_op(NODE_OP_GT, 21, p1_id_1, p2_id_1));
        
        // (then: { })
        ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 22));

        // (else: { p2 = 2; })
        AstNode *else_block = ast_node_create(NODE_BLOCK, 24);
        AstNode *assign_p2 = ast_node_create(NODE_ASSIGNMENT, 24);
        AstNode* p2_id_2 = ast_new_id_node(NODE_ID, 24, "p2");
        p2_id_2->table_entry = param_p2; // (Pass 2)
        ast_node_add_child(assign_p2, p2_id_2);
        ast_node_add_child(assign_p2, ast_new_num_node(2.0, 24));
        ast_node_add_child(else_block, assign_p2);
        ast_node_add_child(if_stmt, else_block);
        
        ast_node_add_child(body, if_stmt);
        ast_node_add_child(func_def_ei, body);
    }
    
    // --- 5. 'func_full' (Полный) ---
    // (Этот код почти идентичен 'func_empty_else' + 'func_empty_if')
    // (Я его пропущу, он не нужен для теста, но ты можешь добавить)


    // --- 6. 'func_nop_jump' (НАСТОЯЩИЙ ТЕСТ) ---
    AstNode *func_def_nop = ast_new_id_node(NODE_FUNCTION_DEF, 30, "func_nop_jump");
    func_def_nop->table_entry = func_nop; // (Pass 2)
    ast_node_add_child(program, func_def_nop);
    {
        // Параметры
        AstNode *param_list = ast_node_create(NODE_PARAM_LIST, 30);
        ast_node_add_child(param_list, ast_new_id_node(NODE_PARAM, 30, "p1"));
        ast_node_add_child(param_list, ast_new_id_node(NODE_PARAM, 30, "p2"));
        param_list->child->table_entry = param_p1; // (Pass 2)
        param_list->child->sibling->table_entry = param_p2; // (Pass 2)
        ast_node_add_child(func_def_nop, param_list);
        
        // Тело
        AstNode *body = ast_node_create(NODE_BLOCK, 30);
        AstNode *if_stmt = ast_node_create(NODE_IF, 31);
        
        // (cond: p1 > p2)
        AstNode* p1_id_1 = ast_new_id_node(NODE_ID, 31, "p1");
        p1_id_1->table_entry = param_p1; // (Pass 2)
        AstNode* p2_id_1 = ast_new_id_node(NODE_ID, 31, "p2");
        p2_id_1->table_entry = param_p2; // (Pass 2)
        ast_node_add_child(if_stmt, ast_new_bin_op(NODE_OP_GT, 31, p1_id_1, p2_id_1));
        
        // (then: { })
        ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 32));

        // (else: НЕТУ)
        // (child->sibling->sibling == NULL)
        
        ast_node_add_child(body, if_stmt);
        ast_node_add_child(func_def_nop, body);
    }

    printf("   ...AST Built (Test for Peephole Jump-NOP).\n");
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
    // optimize_tac(&tac_list);


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
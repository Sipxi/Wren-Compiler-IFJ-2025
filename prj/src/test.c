/**
 * @file test.c
 * Главный тестовый файл для проекта IFJ-2025.
 */

#include "ast.h"
#include "symtable.h" 
#include "tac.h"      
#include "printer.h"  
#include "lexer.h"
#include "codegen.h"
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
 * (Тест для 'a = fun()')
 *
 * static fun() {
 * return;
 * }
 *
 * static main() {
 * var a
 * a = fun()
 * }
 */
static AstNode *create_test_ast(Symtable *global_table) {
    printf("1. Building Fake AST and Symtable...\n");

    // --- 1. (СИМУЛЯЦИЯ Pass 2) ---
    TableEntry *func_main = define_symbol(global_table, "main", KIND_FUNC);
    TableEntry *func_fun = define_symbol(global_table, "fun", KIND_FUNC);
    TableEntry *var_a = define_symbol(global_table, "a", KIND_VAR);

    // (Pass 2) Симулируем, что семантика определила:
    // 'fun' возвращает TYPE_NIL
    func_fun->data->data_type = TYPE_NIL;
    // 'a' пока не имеет типа (TYPE_NIL)
    var_a->data->data_type = TYPE_NIL;


    // --- 2. Строим 'fun()' ---
    AstNode *fun_def = ast_new_id_node(NODE_FUNCTION_DEF, 1, "fun");
    fun_def->table_entry = func_fun; // (Pass 2) Линкуем
    {
        // (Pass 1) Пустой список параметров
        ast_node_add_child(fun_def, ast_node_create(NODE_PARAM_LIST, 1));
        
        // (Pass 1) Тело: { return; }
        AstNode *body = ast_node_create(NODE_BLOCK, 2);
        ast_node_add_child(body, ast_node_create(NODE_RETURN, 2)); // Пустой return
        ast_node_add_child(fun_def, body);
    }

    // --- 3. Строим 'main()' ---
    AstNode *main_def = ast_new_id_node(NODE_FUNCTION_DEF, 5, "main");
    main_def->table_entry = func_main; // (Pass 2) Линкуем
    {
        AstNode *main_block = ast_node_create(NODE_BLOCK, 5);

        // --- var a ---
        AstNode* def_a = ast_new_id_node(NODE_VAR_DEF, 6, "a");
        def_a->table_entry = var_a; // (Pass 2) Линкуем
        ast_node_add_child(main_block, def_a);

        // --- a = fun() ---
        
        // (Pass 1) Узел '='
        AstNode *assign_a = ast_node_create(NODE_ASSIGNMENT, 7);
        
        // (Pass 1) LHS: 'a'
        AstNode* a_id = ast_new_id_node(NODE_ID, 7, "a");
        a_id->table_entry = var_a; // (Pass 2) Линкуем
        a_id->data_type = TYPE_NIL; // (Pass 2) Тип 'a' *до* присваивания
        ast_node_add_child(assign_a, a_id);

        // (Pass 1) RHS: 'fun()'
        AstNode *call_expr = ast_node_create(NODE_CALL_STATEMENT, 7);
        
        // Child 1: ID "fun" (кого вызываем)
        AstNode* fun_id = ast_new_id_node(NODE_ID, 7, "fun");
        fun_id->table_entry = func_fun; // (Pass 2) Линкуем
        fun_id->data_type = func_fun->data->data_type; // (Pass 2) Тип самой функции
        ast_node_add_child(call_expr, fun_id);
        
        // Child 2: Arg list (список аргументов)
        ast_node_add_child(call_expr, ast_node_create(NODE_ARGUMENT_LIST, 7));
        
        // (Pass 2) Семантика установила тип *всего выражения*
        // (Тип выражения 'fun()' равен типу, который возвращает 'fun')
        call_expr->data_type = func_fun->data->data_type; // (TYPE_NIL)
        
        // Добавляем RHS (вызов) в присваивание
        ast_node_add_child(assign_a, call_expr);
        
        // Добавляем стейтмент 'a = fun()' в блок
        ast_node_add_child(main_block, assign_a);
        
        // (Pass 2) Симуляция: 'a' в symtable теперь тоже TYPE_NIL
        var_a->data->data_type = TYPE_NIL;

        // --- Собираем 'main' ---
        ast_node_add_child(main_def, ast_node_create(NODE_PARAM_LIST, 5));
        ast_node_add_child(main_def, main_block);
    }

    // --- 4. Собираем программу ---
    AstNode *program = ast_node_create(NODE_PROGRAM, 0);
    ast_node_add_child(program, fun_def); // Добавляем 'fun'
    ast_node_add_child(program, main_def); // Добавляем 'main'

    printf("   ...AST Built (Test for 'a = fun()').\n");
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
    SymbolData data_local = { KIND_FUNC, TYPE_NUM, true, &local_local_table, 0 };
    symtable_insert(&local_table, "local_var", &data_local);
    SymbolData data1 = { KIND_VAR, TYPE_NUM, true, &local_table, 0 };
    symtable_insert(&table, "var1", &data1);
    SymbolData data2 = { KIND_FUNC, TYPE_NUM, false, NULL, 0 };
    symtable_insert(&table, "func1", &data2);



    symtable_print(&table);

    printf("Удаление символа 'var1'...\n");
    symtable_delete(&table, "var1");
    symtable_print(&table);

    printf("Проверка переполнения таблицы...\n");
    for (int i = 0; i < 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "var%d", i);
        SymbolData data = { KIND_VAR, TYPE_NUM, true, NULL, 0};
        if (!symtable_insert(&table, key, &data)) {
            printf("Ошибка вставки символа '%s'\n", key);
        }
    }
    symtable_print(&table);

    printf("Проверка вставки существующего символа...\n");
    SymbolData data = { KIND_VAR, TYPE_NUM, true, NULL, 0 };
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

    TACDLList tac_list;
    TACDLL_Init(&tac_list);

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
    TACDLL_Dispose(&tac_list); // Это вызовет free_tac_instruction

    printf("Done.\n");

}
void test_gen_code() {

    Symtable global_table;
    symtable_init(&global_table);
    DLList tac_list;
    DLL_Init(&tac_list);
    AstNode *ast_root = create_test_ast(&global_table);
    generate_tac(ast_root, &tac_list, &global_table);
    
    printf("\n--- Generated Code ---\n");
    generate_code(&tac_list, &global_table);

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
    test_gen_code();

    return EXIT_SUCCESS;
}
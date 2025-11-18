/**
 * test_semantics.c
 *
 * Тестовый фреймворк для семантического анализатора (Pass 2).
 *
 * Как использовать:
 * 1. Убедись, что у тебя есть РАБОЧИЕ 'ast.c' и 'symtable.c'.
 * 2. Реализуй функцию 'semantic_check()' в 'semantics.c'.
 * 3. Скомпилируй и запусти:
 * gcc -o test_semantics test_semantics.c ast.c symtable.c semantics.c -std=c11 -Wall
 * 4.    ./test_semantics
 */

#include <stdio.h>
#include <stdlib.h>
#include "semantics.h" // Наш модуль, который мы тестируем
#include "ast.h"        // Наш ast.h
#include "symtable.h"   // Наша таблица символов
#include "tac.h"   // Наша таблица символов
#include "codegen.h"   // Наша таблица символов
#include "optimizer.h"   // Наша таблица символов


 // --- "Красивый визуал" (ANSI цвета) ---
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_CYAN    "\x1b[36m"

int total_tests = 0;
int passed_tests = 0;

/**
 * @brief "Запускатор" тестов.
 *
 * @param test_name         Имя теста для вывода.
 * @param test_ast_root     Фейковый AST, который нужно проверить.
 * @param expected_error    Ожидаемый код ошибки (0 для успеха).
 */
void run_test(const char *test_name, AstNode *test_ast_root, bool expected_error) {
    total_tests++;
    printf(ANSI_COLOR_CYAN "Тест: %s" ANSI_COLOR_RESET " (ожидаем код %d)...\n", test_name, expected_error);

    // 1. Создаем свежую глобальную таблицу символов для теста
    // (ВАЖНО: symtable_create() должен быть из твоей IAL реализации)


    // 2. ВЫЗЫВАЕМ ТВОЮ ФУНКЦИЮ СЕМАНТИКИ
    bool returned_error = analyze_semantics(test_ast_root);

    // 3. Проверяем результат
    if (returned_error == expected_error) {
        printf("  " ANSI_COLOR_GREEN "[PASS]" ANSI_COLOR_RESET " Получен ожидаемый код ошибки: %d\n", returned_error);
        passed_tests++;
    }
    else {
        printf("  " ANSI_COLOR_RED "[FAIL]" ANSI_COLOR_RESET " Ожидался код %d, но получен %d\n", expected_error, returned_error);
    }

    // 4. Очистка
    // (ast_node_free_recursive должен также освобождать строки, как в твоем .h)
    if (test_ast_root) {
        ast_node_free_recursive(test_ast_root);
    }

    printf("------------------------------------------------------------\n");
}

/* ================================================================== */
/* ============= ФУНКЦИИ-ГЕНЕРАТОРЫ ФЕЙКОВЫХ AST ================== */
/* ================================================================== */

/**
 * @brief Тест: Ошибка 4 (Редефиниция) [cite: 39]
 * Код:
 * static main() {
 * var a
 * var a  // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_redefinition_error() {
    // Используем твои API-функции из ast.h
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1)); // Пустой список параметров
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // var a  <-- Ошибка 4
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 4, "a"));

    return root;
}

/**
 * @brief Тест: Ошибка 3 (Неопределенная переменная) [cite: 38]
 * Код:
 * static main() {
 * a = 10  // <-- ОШИБКА ЗДЕСЬ ('a' не определена)
 * }
 */
AstNode *create_test_undefined_variable_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // a = 10
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a")); // <-- Ошибка 3
    ast_node_add_child(assignment, ast_new_num_node(10.0, 3));

    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 6 (Несовместимость типов) [cite: 41]
 * Код:
 * static main() {
 * var a = "hello" + 10 // <-- ОШИБКА ЗДЕСЬ (String + Num)
 * }
 * (Мы симулируем `var a; a = "hello" + 10;` т.к. `var a = ...` это расширение)
 */
AstNode *create_test_type_mismatch_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // "hello" + 10
    AstNode *plus_op = ast_new_bin_op(NODE_OP_PLUS, 3,
        ast_new_string_node("hello", 3), // левый операнд (String)
        ast_new_num_node(10.0, 3)         // правый операнд (Num)
    );

    // a = ...
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a"));
    ast_node_add_child(assignment, plus_op);

    ast_node_add_child(main_block, assignment);

    return root;
}


/**
 * @brief Тест: Ошибка 5 (Неверное кол-во аргументов) [cite: 40]
 * Код:
 * static foo(param1) { }
 * static main() {
 * foo(1, 2) // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_arg_count_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // 1. static foo(param1) { }
    AstNode *func_foo = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    AstNode *foo_params = ast_node_create(NODE_PARAM_LIST, 1);
    ast_node_add_child(foo_params, ast_new_id_node(NODE_PARAM, 1, "param1")); // 1 параметр

    ast_node_add_child(func_foo, foo_params);
    ast_node_add_child(func_foo, ast_node_create(NODE_BLOCK, 1)); // Пустое тело
    ast_node_add_child(root, func_foo); // Добавляем foo в программу

    // 2. static main() { ... }
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 2, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 2));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 3);
    ast_node_add_child(func_main, main_block);

    // foo(1, 2)
    AstNode *call = ast_node_create(NODE_CALL_STATEMENT, 4);
    ast_node_add_child(call, ast_new_id_node(NODE_ID, 4, "foo")); // Имя функции

    AstNode *arg_list = ast_node_create(NODE_ARGUMENT_LIST, 4);
    ast_node_add_child(arg_list, ast_new_num_node(1.0, 4)); // Аргумент 1
    ast_node_add_child(arg_list, ast_new_num_node(2.0, 4)); // Аргумент 2

    ast_node_add_child(call, arg_list); // 2 аргумента
    ast_node_add_child(main_block, call);

    return root;
}

/**
 * @brief Тест: OK (Правильное использование переменной)
 * Код:
 * static main() {
 * var a
 * a = 10
 * var b = a + 20
 * }
 * (Симулируем: var a; a = 10; var b; b = a + 20;)
 */
AstNode *create_test_valid_program() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a = 10
    AstNode *assign_a = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign_a, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assign_a, ast_new_num_node(10.0, 4));
    ast_node_add_child(main_block, assign_a);

    // var b
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 5, "b"));

    // a + 20
    AstNode *plus_op = ast_new_bin_op(NODE_OP_PLUS, 6,
        ast_new_id_node(NODE_ID, 6, "a"), // левый операнд (переменная)
        ast_new_num_node(20.0, 6)         // правый операнд (литерал)
    );

    // b = ...
    AstNode *assign_b = ast_node_create(NODE_ASSIGNMENT, 6);
    ast_node_add_child(assign_b, ast_new_id_node(NODE_ID, 6, "b"));
    ast_node_add_child(assign_b, plus_op);
    ast_node_add_child(main_block, assign_b);

    return root;
}

/**
 * @brief Тест: OK (Правильное вложенное затенение - shadowing с выражением)
 * Код:
 * static main() {
 * var a
 * if (5 == 10) {
 * var a // <-- OK, это новая 'a' в новой области видимости
 * }
 * }
 */
AstNode *create_test_valid_shadowing() {
    // Корень программы
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1)); // Пустой список параметров ()
    ast_node_add_child(root, func_main);

    // { ... } (Блок main)
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a (на 2-й строке)
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 2, "a"));

    // if (5 == 10) (на 3-й строке)
    AstNode *if_stmt = ast_node_create(NODE_IF, 3);
    ast_node_add_child(main_block, if_stmt);

    // ---
    // Создание узла условия (5 == 10)
    // ---
    // 1. Создаем узел для операции '=='
    AstNode *condition_op = ast_node_create(NODE_OP_EQ, 3); // Предполагаемое имя узла

    // 2. Создаем и добавляем левый операнд (5)
    ast_node_add_child(condition_op, ast_new_num_node(5.0, 3));

    // 3. Создаем и добавляем правый операнд (10)
    ast_node_add_child(condition_op, ast_new_num_node(10.0, 3));

    // 4. Добавляем узел '==' (со всем его поддеревом) как условие в if
    ast_node_add_child(if_stmt, condition_op);
    // --- Конец изменений ---

    // { ... } (Блок if, на 4-й строке)
    AstNode *if_block = ast_node_create(NODE_BLOCK, 4);
    ast_node_add_child(if_stmt, if_block);

    // var a (внутри if, на 4-й строке)
    ast_node_add_child(if_block, ast_new_id_node(NODE_VAR_DEF, 4, "a")); // <-- OK

    // else { } (Пустой else, строка 5)
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 5));

    return root;
}

/* ================================================================== */
/* ============= ДОПОЛНИТЕЛЬНЫЕ ФУНКЦИИ-ГЕНЕРАТОРЫ AST ============= */
/* ================================================================== */

/**
 * [cite_start]@brief Тест: Ошибка 3 (Неопределенная функция) [cite: 38]
 * Код:
 * static main() {
 * неопределеннаяФункция()
 * }
 */
AstNode *create_test_undefined_function_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // неопределеннаяФункция()
    AstNode *call = ast_node_create(NODE_CALL_STATEMENT, 3);
    ast_node_add_child(call, ast_new_id_node(NODE_ID, 3, "неопределеннаяФункция"));
    ast_node_add_child(call, ast_node_create(NODE_ARGUMENT_LIST, 3)); // Пустой список аргументов

    ast_node_add_child(main_block, call);

    return root;
}

/**
 * [cite_start]@brief Тест: Ошибка 4 (Редефиниция функции) [cite: 39]
 * Код:
 * static foo() { }
 * static foo() { } // <-- ОШИБКА ЗДЕСЬ
 * static main() { }
 */
AstNode *create_test_function_redefinition_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static foo() { }
    AstNode *func_foo1 = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    ast_node_add_child(func_foo1, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(func_foo1, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func_foo1);

    // static foo() { }  <-- Ошибка 4 (такое же имя, такая же арность)
    AstNode *func_foo2 = ast_new_id_node(NODE_FUNCTION_DEF, 2, "foo");
    ast_node_add_child(func_foo2, ast_node_create(NODE_PARAM_LIST, 2));
    ast_node_add_child(func_foo2, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, func_foo2);

    // static main() { } (Нужен для запуска)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func_main);

    return root;
}


/**
 * [cite_start]@brief Тест: Ошибка 5 (Слишком мало аргументов) [cite: 40]
 * Код:
 * static foo(a, b) { }
 * static main() {
 * foo(1) // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_too_few_args_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static foo(a, b)
    AstNode *func_foo = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    AstNode *foo_params = ast_node_create(NODE_PARAM_LIST, 1);
    ast_node_add_child(foo_params, ast_new_id_node(NODE_PARAM, 1, "a"));
    ast_node_add_child(foo_params, ast_new_id_node(NODE_PARAM, 1, "b")); // 2 параметра
    ast_node_add_child(func_foo, foo_params);
    ast_node_add_child(func_foo, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func_foo);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 2, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 2));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 3);
    ast_node_add_child(func_main, main_block);

    // foo(1)
    AstNode *call = ast_node_create(NODE_CALL_STATEMENT, 4);
    ast_node_add_child(call, ast_new_id_node(NODE_ID, 4, "foo"));
    AstNode *arg_list = ast_node_create(NODE_ARGUMENT_LIST, 4);
    ast_node_add_child(arg_list, ast_new_num_node(1.0, 4)); // 1 аргумент
    ast_node_add_child(call, arg_list);
    ast_node_add_child(main_block, call);

    return root;
}

/**
 * [cite_start]@brief Тест: Ошибка 6 (Арифметика со строками) [cite: 41, 288]
 * Код:
 * static main() {
 * var a = "a" * "b" // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_string_arithmetic_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // "a" * "b"
    AstNode *mul_op = ast_new_bin_op(NODE_OP_MUL, 3,
        ast_new_string_node("a", 3),
        ast_new_string_node("b", 3)
    );

    // a = ...
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a"));
    ast_node_add_child(assignment, mul_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * [cite_start]@brief Тест: Ошибка 6 (Неверное реляционное сравнение) [cite: 41, 296, 297]
 * Код:
 * static main() {
 * if ("a" < "b") { } // <-- ОШИБКА ЗДЕСЬ (только Num < Num)
 * }
 */
AstNode *create_test_invalid_relational_op_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // "a" < "b"
    AstNode *lt_op = ast_new_bin_op(NODE_OP_LT, 3,
        ast_new_string_node("a", 3),
        ast_new_string_node("b", 3)
    );

    // if (...) { } else { }
    AstNode *if_stmt = ast_node_create(NODE_IF, 3);
    ast_node_add_child(if_stmt, lt_op); // Условие
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 3)); // if-блок
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 3)); // else-блок
    ast_node_add_child(main_block, if_stmt);

    return root;
}

/**
 * [cite_start]@brief Тест: Ошибка 4 (Редефиниция геттера) [cite: 39, 219]
 * Код:
 * static myVar { }
 * static myVar { } // <-- ОШИБКА ЗДЕСЬ
 */
AstNode *create_test_getter_redefinition_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static myVar { }
    AstNode *getter1 = ast_new_id_node(NODE_GETTER_DEF, 1, "myVar");
    ast_node_add_child(getter1, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, getter1);

    // static myVar { } <-- Ошибка 4
    AstNode *getter2 = ast_new_id_node(NODE_GETTER_DEF, 2, "myVar");
    ast_node_add_child(getter2, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, getter2);

    // static main() { } (Нужен для запуска)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func_main);

    return root;
}

/**
 * [cite_start]@brief Тест: Ошибка 4 (Редефиниция сеттера) [cite: 39, 219]
 * Код:
 * static myVar = (a) { }
 * static myVar = (b) { } // <-- ОШИБКА ЗДЕСЬ
 */
AstNode *create_test_setter_redefinition_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static myVar = (a) { }
    AstNode *setter1 = ast_new_id_node(NODE_SETTER_DEF, 1, "myVar");
    ast_node_add_child(setter1, ast_new_id_node(NODE_PARAM, 1, "a"));
    ast_node_add_child(setter1, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, setter1);

    // static myVar = (b) { } <-- Ошибка 4
    AstNode *setter2 = ast_new_id_node(NODE_SETTER_DEF, 2, "myVar");
    ast_node_add_child(setter2, ast_new_id_node(NODE_PARAM, 2, "b"));
    ast_node_add_child(setter2, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, setter2);

    // static main() { } (Нужен для запуска)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func_main);

    return root;
}

/**
 * [cite_start]@brief Тест: OK (Валидная перегрузка) [cite: 191]
 * Код:
 * static foo() { }
 * static foo(a) { }
 * static main() {
 * foo()
 * foo(1)
 * }
 */
AstNode *create_test_valid_overloading() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static foo() { }
    AstNode *func_foo1 = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    ast_node_add_child(func_foo1, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(func_foo1, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func_foo1);

    // static foo(a) { }
    AstNode *func_foo2 = ast_new_id_node(NODE_FUNCTION_DEF, 2, "foo");
    AstNode *foo_params2 = ast_node_create(NODE_PARAM_LIST, 2);
    ast_node_add_child(foo_params2, ast_new_id_node(NODE_PARAM, 2, "a"));
    ast_node_add_child(func_foo2, foo_params2);
    ast_node_add_child(func_foo2, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, func_foo2);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(root, func_main);

    // { ... }
    AstNode *main_block = ast_node_create(NODE_BLOCK, 4);
    ast_node_add_child(func_main, main_block);

    // foo()
    AstNode *call1 = ast_node_create(NODE_CALL_STATEMENT, 5);
    ast_node_add_child(call1, ast_new_id_node(NODE_ID, 5, "foo"));
    ast_node_add_child(call1, ast_node_create(NODE_ARGUMENT_LIST, 5));
    ast_node_add_child(main_block, call1);

    // foo(1)
    AstNode *call2 = ast_node_create(NODE_CALL_STATEMENT, 6);
    ast_node_add_child(call2, ast_new_id_node(NODE_ID, 6, "foo"));
    AstNode *arg_list2 = ast_node_create(NODE_ARGUMENT_LIST, 6);
    ast_node_add_child(arg_list2, ast_new_num_node(1.0, 6));
    ast_node_add_child(call2, arg_list2);
    ast_node_add_child(main_block, call2);

    return root;
}

/**
 * [cite_start]@brief Тест: OK (Валидное сравнение с null) [cite: 295]
 * Код:
 * static main() {
 * var a
 * if (a == null) { }
 * }
 */
AstNode *create_test_valid_null_comparison() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a == null
    AstNode *eq_op = ast_new_bin_op(NODE_OP_EQ, 4,
        ast_new_id_node(NODE_ID, 4, "a"),
        ast_new_null_node(4)
    );

    // if (...) { } else { }
    AstNode *if_stmt = ast_node_create(NODE_IF, 4);
    ast_node_add_child(if_stmt, eq_op);
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 4));
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 4));
    ast_node_add_child(main_block, if_stmt);

    return root;
}

/**
 * [cite_start]@brief Тест: OK (Валидное сравнение разных типов) [cite: 294]
 * Код:
 * static main() {
 * var a = 10
 * if (a == "hello") { } // (Всегда false, но валидно)
 * }
 */
AstNode *create_test_valid_mixed_type_equality() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a = 10
    AstNode *assign = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assign, ast_new_num_node(10.0, 4));
    ast_node_add_child(main_block, assign);

    // a == "hello"
    AstNode *eq_op = ast_new_bin_op(NODE_OP_EQ, 5,
        ast_new_id_node(NODE_ID, 5, "a"),
        ast_new_string_node("hello", 5)
    );

    // if (...) { } else { }
    AstNode *if_stmt = ast_node_create(NODE_IF, 5);
    ast_node_add_child(if_stmt, eq_op);
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 5));
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 5));
    ast_node_add_child(main_block, if_stmt);

    return root;
}

/**
 * [cite_start]@brief Тест: OK (Валидное объявление геттера, сеттера и функции) [cite: 220]
 * Код:
 * static val { }
 * static val = (newVal) { }
 * static val(a, b) { }
 * static main() { }
 */
AstNode *create_test_valid_getter_setter_func() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static val { }
    AstNode *getter = ast_new_id_node(NODE_GETTER_DEF, 1, "val");
    ast_node_add_child(getter, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, getter);

    // static val = (newVal) { }
    AstNode *setter = ast_new_id_node(NODE_SETTER_DEF, 2, "val");
    ast_node_add_child(setter, ast_new_id_node(NODE_PARAM, 2, "newVal"));
    ast_node_add_child(setter, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, setter);

    // static val(a, b) { }
    AstNode *func = ast_new_id_node(NODE_FUNCTION_DEF, 3, "val");
    AstNode *func_params = ast_node_create(NODE_PARAM_LIST, 3);
    ast_node_add_child(func_params, ast_new_id_node(NODE_PARAM, 3, "a"));
    ast_node_add_child(func_params, ast_new_id_node(NODE_PARAM, 3, "b"));
    ast_node_add_child(func, func_params);
    ast_node_add_child(func, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func);

    // static main() { } (Нужен для запуска)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 4, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 4));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 4));
    ast_node_add_child(root, func_main);

    return root;
}


/**
 * @brief Тест: Ошибка 6 (Вычитание строк)
 * Код:
 * static main() {
 * var a = "a" - "b" // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_string_subtraction_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    AstNode *sub_op = ast_new_bin_op(NODE_OP_MINUS, 3,
        ast_new_string_node("a", 3),
        ast_new_string_node("b", 3)
    );

    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a"));
    ast_node_add_child(assignment, sub_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 6 (Деление с null)
 * Код:
 * static main() {
 * var a = 10 / null // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_division_by_null_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    AstNode *div_op = ast_new_bin_op(NODE_OP_DIV, 3,
        ast_new_num_node(10.0, 3),
        ast_new_null_node(3)
    );

    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a"));
    ast_node_add_child(assignment, div_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 6 (Реляционное сравнение с null)
 * Код:
 * static main() {
 * if (null < 10) {} // <-- ОШИБКА ЗДЕСЬ (только == or !=)
 * }
 */
AstNode *create_test_null_relational_op_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    AstNode *lt_op = ast_new_bin_op(NODE_OP_LT, 3,
        ast_new_null_node(3),
        ast_new_num_node(10.0, 3)
    );

    AstNode *if_stmt = ast_node_create(NODE_IF, 3);
    ast_node_add_child(if_stmt, lt_op);
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(main_block, if_stmt);

    return root;
}

/**
 * @brief Тест: Ошибка 3 (Нет функции main)
 * Код:
 * static foo() { } // (main отсутствует)
 */
AstNode *create_test_no_main_function_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    AstNode *func_foo = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    ast_node_add_child(func_foo, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(func_foo, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func_foo);

    return root;
}

/**
 * @brief Тест: Ошибка 3 (main с параметрами)
 * Код:
 * static main(a) { } // <-- ОШИБКА ЗДЕСЬ (main должен быть без параметров)
 */
AstNode *create_test_main_with_params_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    AstNode *params = ast_node_create(NODE_PARAM_LIST, 1);
    ast_node_add_child(params, ast_new_id_node(NODE_PARAM, 1, "a")); // <-- Ошибка
    ast_node_add_child(func_main, params);
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func_main);

    return root;
}

/**
 * @brief Тест: OK (main с параметрами И main без параметров)
 * Это валидно по заданию
 * Код:
 * static main(a) { }
 * static main() { } // <-- OK
 */
AstNode *create_test_valid_main_overload() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main(a) { }
    AstNode *func_main_p = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    AstNode *params = ast_node_create(NODE_PARAM_LIST, 1);
    ast_node_add_child(params, ast_new_id_node(NODE_PARAM, 1, "a"));
    ast_node_add_child(func_main_p, params);
    ast_node_add_child(func_main_p, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func_main_p);

    // static main() { }
    AstNode *func_main_0 = ast_new_id_node(NODE_FUNCTION_DEF, 2, "main");
    ast_node_add_child(func_main_0, ast_node_create(NODE_PARAM_LIST, 2));
    ast_node_add_child(func_main_0, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, func_main_0);

    return root;
}

/**
 * @brief Тест: OK (Использование переменной из внешнего скоупа)
 * Код:
 * static main() {
 * var a = 10
 * if (true) {
 * var b = a + 5 // <-- OK
 * }
 * }
 */
AstNode *create_test_valid_outer_scope_usage() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));
    // a = 10
    AstNode *assign_a = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign_a, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assign_a, ast_new_num_node(10.0, 4));
    ast_node_add_child(main_block, assign_a);

    // if (true)
    AstNode *if_stmt = ast_node_create(NODE_IF, 5);
    ast_node_add_child(if_stmt, ast_new_num_node(1.0, 5)); // Условие
    ast_node_add_child(main_block, if_stmt);

    // { ... } (if-блок)
    AstNode *if_block = ast_node_create(NODE_BLOCK, 6);
    ast_node_add_child(if_stmt, if_block);

    // var b
    ast_node_add_child(if_block, ast_new_id_node(NODE_VAR_DEF, 7, "b"));
    // b = a + 5
    AstNode *plus_op = ast_new_bin_op(NODE_OP_PLUS, 8,
        ast_new_id_node(NODE_ID, 8, "a"), // <-- 'a' из внешнего скоупа
        ast_new_num_node(5.0, 8)
    );
    AstNode *assign_b = ast_node_create(NODE_ASSIGNMENT, 8);
    ast_node_add_child(assign_b, ast_new_id_node(NODE_ID, 8, "b"));
    ast_node_add_child(assign_b, plus_op);
    ast_node_add_child(if_block, assign_b);

    // else { }
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 9));

    return root;
}

/**
 * @brief Тест: Ошибка 10 (Попытка присвоить значение геттеру)
 * Код:
 * static myGetter { }
 * static main() {
 * myGetter = 10 // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_assign_to_getter_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static myGetter { }
    AstNode *getter = ast_new_id_node(NODE_GETTER_DEF, 1, "myGetter");
    ast_node_add_child(getter, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, getter);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 2, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 2));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 3);
    ast_node_add_child(func_main, main_block);

    // myGetter = 10
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 4, "myGetter")); // <-- Ошибка 10
    ast_node_add_child(assignment, ast_new_num_node(10.0, 4));

    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 10 (Попытка прочитать значение из сеттера)
 * Код:
 * static mySetter = (a) { }
 * static main() {
 * var b = mySetter // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_read_from_setter_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static mySetter = (a) { }
    AstNode *setter = ast_new_id_node(NODE_SETTER_DEF, 1, "mySetter");
    ast_node_add_child(setter, ast_new_id_node(NODE_PARAM, 1, "a"));
    ast_node_add_child(setter, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, setter);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 2, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 2));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 3);
    ast_node_add_child(func_main, main_block);

    // var b
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 4, "b"));

    // b = mySetter
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 5);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 5, "b"));
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 5, "mySetter")); // <-- Ошибка 10

    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: OK (Валидное использование 'is')
 * Код:
 * static main() {
 * var a
 * if (a is Num) { }
 * }
 */
AstNode *create_test_valid_is_operator() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a is Num
    AstNode *is_op = ast_new_bin_op(NODE_OP_IS, 4,
        ast_new_id_node(NODE_ID, 4, "a"),
        ast_new_id_node(NODE_TYPE_NAME, 4, "Num") // Правая часть 'is'
    );

    // if (...) { } else { }
    AstNode *if_stmt = ast_node_create(NODE_IF, 4);
    ast_node_add_child(if_stmt, is_op);
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 4));
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 4));
    ast_node_add_child(main_block, if_stmt);

    return root;
}

/**
 * @brief Тест: Ошибка 10 (Невалидный тип в 'is')
 * Код:
 * static main() {
 * if (10 is "String") { } // <-- ОШИБКА (правая часть должна быть NODE_TYPE_NAME)
 * }
 * (Твой парсер должен был создать NODE_LITERAL_STRING, а семантика это отвергнет)
 */
AstNode *create_test_invalid_is_rhs_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // 10 is "String"
    AstNode *is_op = ast_new_bin_op(NODE_OP_IS, 3,
        ast_new_num_node(10.0, 3),
        ast_new_string_node("String", 3) // <-- Ошибка, неверный тип узла
    );

    AstNode *if_stmt = ast_node_create(NODE_IF, 3);
    ast_node_add_child(if_stmt, is_op);
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(main_block, if_stmt);

    return root;
}

/**
 * @brief Тест: Ошибка 6 (Ошибка типа в условии 'while')
 * Код:
 * static main() {
 * while (10 + "a") { } // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_while_condition_type_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // 10 + "a"
    AstNode *plus_op = ast_new_bin_op(NODE_OP_PLUS, 3,
        ast_new_num_node(10.0, 3),
        ast_new_string_node("a", 3)
    );

    // while (...) { }
    AstNode *while_stmt = ast_node_create(NODE_WHILE, 3);
    ast_node_add_child(while_stmt, plus_op); // Условие с ошибкой
    ast_node_add_child(while_stmt, ast_node_create(NODE_BLOCK, 3)); // Тело
    ast_node_add_child(main_block, while_stmt);

    return root;
}

/**
 * @brief Тест: OK (Валидное присваивание глобальной переменной)
 * Глобальные переменные не нужно объявлять через 'var'.
 * Код:
 * static main() {
 * __myGlobal = 10
 * }
 */
AstNode *create_test_valid_global_assign() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // __myGlobal = 10
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    // NODE_ID для __myGlobal - это ОK
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "__myGlobal"));
    ast_node_add_child(assignment, ast_new_num_node(10.0, 3));

    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: OK (Валидное чтение глобальной переменной)
 * Код:
 * static main() {
 * var a = __myGlobal // (Читаем __myGlobal, даже если он не был присвоен)
 * }
 */
AstNode *create_test_valid_global_read() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a = __myGlobal
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 4, "__myGlobal")); // <-- OK

    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: OK (Валидная итерация строки)
 * Код:
 * static main() {
 * var a = "ha" * 3
 * }
 */
AstNode *create_test_valid_string_iteration() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // "ha" * 3
    AstNode *mul_op = ast_new_bin_op(NODE_OP_MUL, 3,
        ast_new_string_node("ha", 3), // String
        ast_new_num_node(3.0, 3)        // Num (целое)
    );

    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a"));
    ast_node_add_child(assignment, mul_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 6 (Невалидная итерация строки - неверный порядок)
 * Код:
 * static main() {
 * var a = 3 * "ha" // <-- ОШИБКА (String должен быть слева)
 * }
 */
AstNode *create_test_string_iteration_order_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // 3 * "ha"
    AstNode *mul_op = ast_new_bin_op(NODE_OP_MUL, 3,
        ast_new_num_node(3.0, 3),        // Num
        ast_new_string_node("ha", 3)     // String
    );

    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a"));
    ast_node_add_child(assignment, mul_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 6 (Невалидная итерация строки - не целое число)
 * Код:
 * static main() {
 * var a = "ha" * 1.5 // <-- ОШИБКА (Num должен быть целым)
 * }
 */
AstNode *create_test_string_iteration_float_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // "ha" * 1.5
    AstNode *mul_op = ast_new_bin_op(NODE_OP_MUL, 3,
        ast_new_string_node("ha", 3), // String
        ast_new_num_node(1.5, 3)        // Num (не целое)
    );

    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 3);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 3, "a"));
    ast_node_add_child(assignment, mul_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 5 (Встроенная функция - неверная арность)
 * Код:
 * static main() {
 * Ifj.write("a", "b") // <-- ОШИБКА (Ifj.write ожидает 1 арг)
 * }
 */
AstNode *create_test_builtin_wrong_arity_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // Ifj.write("a", "b")
    AstNode *call = ast_node_create(NODE_CALL_STATEMENT, 3);
    ast_node_add_child(call, ast_new_id_node(NODE_ID, 3, "Ifj.write"));

    AstNode *arg_list = ast_node_create(NODE_ARGUMENT_LIST, 3);
    ast_node_add_child(arg_list, ast_new_string_node("a", 3));
    ast_node_add_child(arg_list, ast_new_string_node("b", 3)); // <-- Слишком много

    ast_node_add_child(call, arg_list);
    ast_node_add_child(main_block, call);

    return root;
}

/**
 * @brief Тест: Ошибка 3 (Встроенная функция - неверное имя)
 * Код:
 * static main() {
 * Ifj.nonexistent() // <-- ОШИБКА
 * }
 */
AstNode *create_test_builtin_nonexistent_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // Ifj.nonexistent()
    AstNode *call = ast_node_create(NODE_CALL_STATEMENT, 3);
    ast_node_add_child(call, ast_new_id_node(NODE_ID, 3, "Ifj.nonexistent"));
    ast_node_add_child(call, ast_node_create(NODE_ARGUMENT_LIST, 3));
    ast_node_add_child(main_block, call);

    return root;
}
/**
 * @brief Тест: OK (return из main)
 * Код:
 * static main() {
 * return 10
 * }
 */
AstNode *create_test_valid_return_from_main() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // return 10
    AstNode *ret_stmt = ast_node_create(NODE_RETURN, 3);
    ast_node_add_child(ret_stmt, ast_new_num_node(10.0, 3));
    ast_node_add_child(main_block, ret_stmt);

    return root;
}

/**
 * @brief Тест: OK (Пустой return)
 * (Это расширение EXTSTAT, но твой AST это позволяет: child -> NULL)
 * Код:
 * static main() {
 * return
 * }
 */
AstNode *create_test_valid_empty_return() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // return
    AstNode *ret_stmt = ast_node_create(NODE_RETURN, 3);
    // ret_stmt->child остается NULL
    ast_node_add_child(main_block, ret_stmt);

    return root;
}


/**
 * @brief Тест: Ошибка 6 (Ошибка типа в 'return')
 * Код:
 * static main() {
 * return "a" / 10 // <-- ОШИБКА ЗДЕСЬ
 * }
 */
AstNode *create_test_return_type_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // "a" / 10
    AstNode *div_op = ast_new_bin_op(NODE_OP_DIV, 3,
        ast_new_string_node("a", 3),
        ast_new_num_node(10.0, 3)
    );

    // return ...
    AstNode *ret_stmt = ast_node_create(NODE_RETURN, 3);
    ast_node_add_child(ret_stmt, div_op);
    ast_node_add_child(main_block, ret_stmt);

    return root;
}

/**
 * @brief Тест: Ошибка 4 (Коллизия имени переменной и функции)
 * Код:
 * static main() {
 * var foo
 * }
 * static foo() { } // <-- ОШИБКА ЗДЕСЬ
 */
AstNode *create_test_var_func_collision_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static main() { var foo; }
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);
    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "foo")); // <-- Определение 'foo' как переменной

    // static foo() { }
    AstNode *func_foo = ast_new_id_node(NODE_FUNCTION_DEF, 4, "foo"); // <-- Ошибка 4
    ast_node_add_child(func_foo, ast_node_create(NODE_PARAM_LIST, 4));
    ast_node_add_child(func_foo, ast_node_create(NODE_BLOCK, 4));
    ast_node_add_child(root, func_foo);

    return root;
}

/**
 * @brief Тест: Ошибка 4 (Коллизия имени функции и геттера)
 * Код:
 * static foo() { }
 * static foo { } // <-- ОШИБКА ЗДЕСЬ
 */
AstNode *create_test_func_getter_collision_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static foo() { }
    AstNode *func_foo = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    ast_node_add_child(func_foo, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(func_foo, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func_foo);

    // static foo { }
    AstNode *getter_foo = ast_new_id_node(NODE_GETTER_DEF, 2, "foo"); // <-- Ошибка 4
    ast_node_add_child(getter_foo, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, getter_foo);

    // main (для валидности)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func_main);

    return root;
}

/**
 * @brief Тест: OK (Сложное валидное выражение)
 * Код:
 * static main() {
 * var a = 10
 * var b = 20
 * var c = (a + b) * (b - 5)
 * }
 */
AstNode *create_test_valid_complex_expression() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a = 10 (симулируем)
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));
    AstNode *assign_a = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign_a, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assign_a, ast_new_num_node(10.0, 4));
    ast_node_add_child(main_block, assign_a);

    // var b = 20 (симулируем)
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 5, "b"));
    AstNode *assign_b = ast_node_create(NODE_ASSIGNMENT, 6);
    ast_node_add_child(assign_b, ast_new_id_node(NODE_ID, 6, "b"));
    ast_node_add_child(assign_b, ast_new_num_node(20.0, 6));
    ast_node_add_child(main_block, assign_b);

    // (a + b)
    AstNode *plus_op = ast_new_bin_op(NODE_OP_PLUS, 7,
        ast_new_id_node(NODE_ID, 7, "a"),
        ast_new_id_node(NODE_ID, 7, "b")
    );
    // (b - 5)
    AstNode *minus_op = ast_new_bin_op(NODE_OP_MINUS, 7,
        ast_new_id_node(NODE_ID, 7, "b"),
        ast_new_num_node(5.0, 7)
    );
    // ( ... ) * ( ... )
    AstNode *mul_op = ast_new_bin_op(NODE_OP_MUL, 7, plus_op, minus_op);

    // var c = ... (симулируем)
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 7, "c"));
    AstNode *assign_c = ast_node_create(NODE_ASSIGNMENT, 7);
    ast_node_add_child(assign_c, ast_new_id_node(NODE_ID, 7, "c"));
    ast_node_add_child(assign_c, mul_op);
    ast_node_add_child(main_block, assign_c);

    return root;
}

/**
 * @brief Тест: Ошибка 6 (Сложное невалидное выражение)
 * Код:
 * static main() {
 * var a = 10
 * var b = "hello"
 * var c = (a + b) * (b - 5) // <-- ОШИБКА (a+b) это String, String * Num
 * }
 */
AstNode *create_test_invalid_complex_expression_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var a = 10
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));
    AstNode *assign_a = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign_a, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assign_a, ast_new_num_node(10.0, 4));
    ast_node_add_child(main_block, assign_a);

    // var b = "hello"
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 5, "b"));
    AstNode *assign_b = ast_node_create(NODE_ASSIGNMENT, 6);
    ast_node_add_child(assign_b, ast_new_id_node(NODE_ID, 6, "b"));
    ast_node_add_child(assign_b, ast_new_string_node("hello", 6));
    ast_node_add_child(main_block, assign_b);

    // (a + b)
    AstNode *plus_op = ast_new_bin_op(NODE_OP_PLUS, 7,
        ast_new_id_node(NODE_ID, 7, "a"), // Num
        ast_new_id_node(NODE_ID, 7, "b")  // String
    ); // <-- Ошибка 6 уже здесь, но анализатор может пойти дальше

    // (b - 5)
    AstNode *minus_op = ast_new_bin_op(NODE_OP_MINUS, 7,
        ast_new_id_node(NODE_ID, 7, "b"), // String
        ast_new_num_node(5.0, 7)         // Num
    ); // <-- И здесь ошибка 6

    // ( ... ) * ( ... )
    AstNode *mul_op = ast_new_bin_op(NODE_OP_MUL, 7, plus_op, minus_op);

    // var c = ...
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 7, "c"));
    AstNode *assign_c = ast_node_create(NODE_ASSIGNMENT, 7);
    ast_node_add_child(assign_c, ast_new_id_node(NODE_ID, 7, "c"));
    ast_node_add_child(assign_c, mul_op);
    ast_node_add_child(main_block, assign_c);

    return root;
}

/**
 * @brief Тест: OK (Валидное использование Геттера в выражении)
 * Код:
 * static myGetter { }
 * static main() {
 * var a = myGetter + 10 // <-- OK
 * }
 */
AstNode *create_test_valid_getter_use() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static myGetter { }
    AstNode *getter = ast_new_id_node(NODE_GETTER_DEF, 1, "myGetter");
    ast_node_add_child(getter, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, getter);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 2, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 2));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 3);
    ast_node_add_child(func_main, main_block);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 4, "a"));

    // myGetter + 10
    AstNode *plus_op = ast_new_bin_op(NODE_OP_PLUS, 5,
        ast_new_id_node(NODE_ID, 5, "myGetter"), // <-- Валидное чтение геттера
        ast_new_num_node(10.0, 5)
    );

    // a = ...
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 5);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 5, "a"));
    ast_node_add_child(assignment, plus_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: OK (Валидное использование Сеттера в присваивании)
 * Код:
 * static mySetter = (val) { }
 * static main() {
 * mySetter = 10 // <-- OK
 * }
 */
AstNode *create_test_valid_setter_use() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static mySetter = (val) { }
    AstNode *setter = ast_new_id_node(NODE_SETTER_DEF, 1, "mySetter");
    ast_node_add_child(setter, ast_new_id_node(NODE_PARAM, 1, "val"));
    ast_node_add_child(setter, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, setter);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 2, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 2));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 3);
    ast_node_add_child(func_main, main_block);

    // mySetter = 10
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 4, "mySetter")); // <-- Валидное присваивание сеттеру
    ast_node_add_child(assignment, ast_new_num_node(10.0, 4));

    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 5 (Встроенная функция - неверная арность 'read_num')
 * Код:
 * static main() {
 * var a = Ifj.read_num(10) // <-- ОШИБКА
 * }
 */
AstNode *create_test_builtin_arity_read_num_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // Ifj.read_num(10)
    AstNode *call = ast_node_create(NODE_CALL_STATEMENT, 4);
    ast_node_add_child(call, ast_new_id_node(NODE_ID, 4, "Ifj.read_num"));
    AstNode *arg_list = ast_node_create(NODE_ARGUMENT_LIST, 4);
    ast_node_add_child(arg_list, ast_new_num_node(10.0, 4)); // <-- Ошибка 5
    ast_node_add_child(call, arg_list);

    // Простая проверка NODE_CALL_STATEMENT с неправильной арностью
    ast_node_add_child(main_block, call); // `Ifj.read_num(10)` как отдельный стейтмент

    return root;
}


/**
 * @brief Тест: Ошибка 5 (Встроенная функция - неверный тип литерала)
 * Код:
 * static main() {
 * var a = Ifj.length(123) // <-- ОШИБКА 5 (ожидает String)
 * }
 */
AstNode *create_test_builtin_type_length_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // Симуляция `a = Ifj.length(123)` - создаем только нужные узлы

    AstNode *call_expr = ast_node_create(NODE_CALL_STATEMENT, 6);
    ast_node_add_child(call_expr, ast_new_id_node(NODE_ID, 6, "Ifj.length"));
    AstNode *args = ast_node_create(NODE_ARGUMENT_LIST, 6);
    ast_node_add_child(args, ast_new_num_node(123.0, 6)); // <-- Ошибка 5
    ast_node_add_child(call_expr, args);

    AstNode *assign = ast_node_create(NODE_ASSIGNMENT, 6);
    ast_node_add_child(assign, ast_new_id_node(NODE_ID, 6, "a"));
    ast_node_add_child(assign, call_expr); // Семантика должна проверить это
    ast_node_add_child(main_block, assign);

    return root;
}


/**
 * @brief Тест: Ошибка 3 (Использование переменной из `if` во `else`)
 * Код:
 * static main() {
 * if (true) { var a } else { var b = a } // <-- ОШИБКА
 * }
 */
AstNode *create_test_sibling_scope_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    AstNode *if_stmt = ast_node_create(NODE_IF, 3);
    ast_node_add_child(if_stmt, ast_new_num_node(1.0, 3)); // Условие

    // if { var a }
    AstNode *if_block = ast_node_create(NODE_BLOCK, 4);
    ast_node_add_child(if_block, ast_new_id_node(NODE_VAR_DEF, 5, "a"));
    ast_node_add_child(if_stmt, if_block);

    // else { var b = a }
    AstNode *else_block = ast_node_create(NODE_BLOCK, 6);
    ast_node_add_child(else_block, ast_new_id_node(NODE_VAR_DEF, 7, "b"));
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 8);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 8, "b"));
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 8, "a")); // <-- Ошибка 3
    ast_node_add_child(else_block, assignment);
    ast_node_add_child(if_stmt, else_block);

    ast_node_add_child(main_block, if_stmt);

    return root;
}

/**
 * @brief Тест: Ошибка 3 (Использование переменной после выхода из скоупа)
 * Код:
 * static main() {
 * if (true) { var a }
 * var b = a // <-- ОШИБКА
 * }
 */
AstNode *create_test_scope_exit_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // if (true) { var a }
    AstNode *if_stmt = ast_node_create(NODE_IF, 3);
    ast_node_add_child(if_stmt, ast_new_num_node(1.0, 3));
    AstNode *if_block = ast_node_create(NODE_BLOCK, 4);
    ast_node_add_child(if_block, ast_new_id_node(NODE_VAR_DEF, 5, "a"));
    ast_node_add_child(if_stmt, if_block);
    ast_node_add_child(if_stmt, ast_node_create(NODE_BLOCK, 6)); // else
    ast_node_add_child(main_block, if_stmt);

    // var b = a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 7, "b"));
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 8);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 8, "b"));
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 8, "a")); // <-- Ошибка 3
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: OK (Валидная простая рекурсия)
 * Код:
 * static foo() { foo() }
 * static main() { foo() }
 */
AstNode *create_test_valid_recursion() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static foo() { foo() }
    AstNode *func_foo = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    ast_node_add_child(func_foo, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_foo);
    AstNode *foo_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_foo, foo_block);
    AstNode *call_foo = ast_node_create(NODE_CALL_STATEMENT, 3);
    ast_node_add_child(call_foo, ast_new_id_node(NODE_ID, 3, "foo"));
    ast_node_add_child(call_foo, ast_node_create(NODE_ARGUMENT_LIST, 3));
    ast_node_add_child(foo_block, call_foo);

    // static main() { foo() }
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 4, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 4));
    ast_node_add_child(root, func_main);
    AstNode *main_block = ast_node_create(NODE_BLOCK, 5);
    ast_node_add_child(func_main, main_block);
    AstNode *call_foo_main = ast_node_create(NODE_CALL_STATEMENT, 6);
    ast_node_add_child(call_foo_main, ast_new_id_node(NODE_ID, 6, "foo"));
    ast_node_add_child(call_foo_main, ast_node_create(NODE_ARGUMENT_LIST, 6));
    ast_node_add_child(main_block, call_foo_main);

    return root;
}

/**
 * @brief Тест: OK (Валидная взаимная рекурсия)
 * Код:
 * static foo() { bar() }
 * static bar() { foo() }
 * static main() { foo() }
 */
AstNode *create_test_valid_mutual_recursion() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static foo() { bar() }
    AstNode *func_foo = ast_new_id_node(NODE_FUNCTION_DEF, 1, "foo");
    ast_node_add_child(func_foo, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_foo);
    AstNode *foo_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_foo, foo_block);
    AstNode *call_bar = ast_node_create(NODE_CALL_STATEMENT, 3);
    ast_node_add_child(call_bar, ast_new_id_node(NODE_ID, 3, "bar"));
    ast_node_add_child(call_bar, ast_node_create(NODE_ARGUMENT_LIST, 3));
    ast_node_add_child(foo_block, call_bar);

    // static bar() { foo() }
    AstNode *func_bar = ast_new_id_node(NODE_FUNCTION_DEF, 4, "bar");
    ast_node_add_child(func_bar, ast_node_create(NODE_PARAM_LIST, 4));
    ast_node_add_child(root, func_bar);
    AstNode *bar_block = ast_node_create(NODE_BLOCK, 5);
    ast_node_add_child(func_bar, bar_block);
    AstNode *call_foo = ast_node_create(NODE_CALL_STATEMENT, 6);
    ast_node_add_child(call_foo, ast_new_id_node(NODE_ID, 6, "foo"));
    ast_node_add_child(call_foo, ast_node_create(NODE_ARGUMENT_LIST, 6));
    ast_node_add_child(bar_block, call_foo);

    // static main() { bar() }
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 7, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 7));
    ast_node_add_child(root, func_main);
    AstNode *main_block = ast_node_create(NODE_BLOCK, 8);
    ast_node_add_child(func_main, main_block);
    // Создаем новый узел вместо переиспользования
    AstNode *call_bar_main = ast_node_create(NODE_CALL_STATEMENT, 9);
    ast_node_add_child(call_bar_main, ast_new_id_node(NODE_ID, 9, "bar"));
    ast_node_add_child(call_bar_main, ast_node_create(NODE_ARGUMENT_LIST, 9));
    ast_node_add_child(main_block, call_bar_main);

    return root;
}

/**
 * @brief Тест: Ошибка 4 (Редефиниция функции с той же арностью)
 * Код:
 * static myFunc(a) { }
 * static myFunc(b) { } // <-- ОШИБКА ЗДЕСЬ
 * static main() { }
 */
AstNode *create_test_func_func_arity_collision_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static myFunc(a) { }
    AstNode *func1 = ast_new_id_node(NODE_FUNCTION_DEF, 1, "myFunc");
    AstNode *params1 = ast_node_create(NODE_PARAM_LIST, 1);
    ast_node_add_child(params1, ast_new_id_node(NODE_PARAM, 1, "a"));
    ast_node_add_child(func1, params1);
    ast_node_add_child(func1, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, func1);

    // static myFunc(b) { } <-- Ошибка 4 (то же имя, та же арность)
    AstNode *func2 = ast_new_id_node(NODE_FUNCTION_DEF, 2, "myFunc");
    AstNode *params2 = ast_node_create(NODE_PARAM_LIST, 2);
    ast_node_add_child(params2, ast_new_id_node(NODE_PARAM, 2, "b"));
    ast_node_add_child(func2, params2);
    ast_node_add_child(func2, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, func2);

    // main (для валидности)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func_main);

    return root;
}


/**
 * @brief Тест: Ошибка 4 (Редефиниция Геттера)
 * (Этот тест уже был, create_test_getter_redefinition_error,
 * но я оставлю его для ясности под новым именем)
 * Код:
 * static myProp { }
 * static myProp { } // <-- ОШИБКА ЗДЕСЬ
 * static main() { }
 */
AstNode *create_test_getter_getter_collision_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static myProp { }
    AstNode *getter1 = ast_new_id_node(NODE_GETTER_DEF, 1, "myProp");
    ast_node_add_child(getter1, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, getter1);

    // static myProp { } <-- Ошибка 4
    AstNode *getter2 = ast_new_id_node(NODE_GETTER_DEF, 2, "myProp");
    ast_node_add_child(getter2, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, getter2);

    // main (для валидности)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func_main);

    return root;
}

/**
 * @brief Тест: Ошибка 4 (Редефиниция Сеттера)
 * (Этот тест также уже был, create_test_setter_redefinition_error)
 * Код:
 * static myProp = (a) { }
 * static myProp = (b) { } // <-- ОШИБКА ЗДЕСЬ
 * static main() { }
 */
AstNode *create_test_setter_setter_collision_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static myProp = (a) { }
    AstNode *setter1 = ast_new_id_node(NODE_SETTER_DEF, 1, "myProp");
    ast_node_add_child(setter1, ast_new_id_node(NODE_PARAM, 1, "a"));
    ast_node_add_child(setter1, ast_node_create(NODE_BLOCK, 1));
    ast_node_add_child(root, setter1);

    // static myProp = (b) { } <-- Ошибка 4
    AstNode *setter2 = ast_new_id_node(NODE_SETTER_DEF, 2, "myProp");
    ast_node_add_child(setter2, ast_new_id_node(NODE_PARAM, 2, "b"));
    ast_node_add_child(setter2, ast_node_create(NODE_BLOCK, 2));
    ast_node_add_child(root, setter2);

    // main (для валидности)
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 3));
    ast_node_add_child(root, func_main);

    return root;
}

/**
 * @brief Тест: Ошибка 5 (Встроенная 'substring' - неверная арность)
 * Код:
 * static main() {
 * var a = Ifj.substring("hello", 1) // <-- ОШИБКА (ожидает 3)
 * }
 */
AstNode *create_test_builtin_substring_arity_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a = Ifj.substring("hello", 1)
    AstNode *call_expr = ast_node_create(NODE_CALL_STATEMENT, 4);
    ast_node_add_child(call_expr, ast_new_id_node(NODE_ID, 4, "Ifj.substring"));
    AstNode *args = ast_node_create(NODE_ARGUMENT_LIST, 4);
    ast_node_add_child(args, ast_new_string_node("hello", 4));
    ast_node_add_child(args, ast_new_num_node(1.0, 4)); // <-- Только 2 арг.
    ast_node_add_child(call_expr, args);

    AstNode *assign = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assign, call_expr);
    ast_node_add_child(main_block, assign);

    return root;
}

/**
 * @brief Тест: Ошибка 5 (Встроенная 'substring' - неверный тип литерала)
 * Код:
 * static main() {
 * var a = Ifj.substring(1, 2, 3) // <-- ОШИБКА (1-й должен быть String)
 * }
 */
AstNode *create_test_builtin_substring_type_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a = Ifj.substring(1, 2, 3)
    AstNode *call_expr = ast_node_create(NODE_CALL_STATEMENT, 4);
    ast_node_add_child(call_expr, ast_new_id_node(NODE_ID, 4, "Ifj.substring"));
    AstNode *args = ast_node_create(NODE_ARGUMENT_LIST, 4);
    ast_node_add_child(args, ast_new_num_node(1.0, 4)); // <-- Ошибка 5 (должен быть String)
    ast_node_add_child(args, ast_new_num_node(2.0, 4));
    ast_node_add_child(args, ast_new_num_node(3.0, 4));
    ast_node_add_child(call_expr, args);

    AstNode *assign = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assign, call_expr);
    ast_node_add_child(main_block, assign);

    return root;
}

/**
 * @brief Тест: Ошибка 5 (Встроенная 'chr' - неверный тип литерала)
 * Код:
 * static main() {
 * var a = Ifj.chr("hello") // <-- ОШИБКА (ожидает Num)
 * }
 */
AstNode *create_test_builtin_chr_type_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a = Ifj.chr("hello")
    AstNode *call_expr = ast_node_create(NODE_CALL_STATEMENT, 4);
    ast_node_add_child(call_expr, ast_new_id_node(NODE_ID, 4, "Ifj.chr"));
    AstNode *args = ast_node_create(NODE_ARGUMENT_LIST, 4);
    ast_node_add_child(args, ast_new_string_node("hello", 4)); // <-- Ошибка 5
    ast_node_add_child(call_expr, args);

    AstNode *assign = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assign, call_expr);
    ast_node_add_child(main_block, assign);

    return root;
}

/**
 * @brief Тест: Ошибка 5 (Встроенная 'ord' - неверный тип литерала)
 * Код:
 * static main() {
 * var a = Ifj.ord(1, "a") // <-- ОШИБКА (1-й String, 2-й Num)
 * }
 */
AstNode *create_test_builtin_ord_type_error() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // a = Ifj.ord(1, "a")
    AstNode *call_expr = ast_node_create(NODE_CALL_STATEMENT, 4);
    ast_node_add_child(call_expr, ast_new_id_node(NODE_ID, 4, "Ifj.ord"));
    AstNode *args = ast_node_create(NODE_ARGUMENT_LIST, 4);
    ast_node_add_child(args, ast_new_num_node(1.0, 4));       // <-- Ошибка 5 (должен быть String)
    ast_node_add_child(args, ast_new_string_node("a", 4)); // <-- Ошибка 5 (должен быть Num)
    ast_node_add_child(call_expr, args);

    AstNode *assign = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assign, call_expr);
    ast_node_add_child(main_block, assign);

    return root;
}

/**
 * @brief Тест: OK (Валидный 'return' в Геттере)
 * Код:
 * static myGetter { return "hello" }
 * static main() { }
 */
AstNode *create_test_valid_return_in_getter() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);

    // static myGetter { return "hello" }
    AstNode *getter = ast_new_id_node(NODE_GETTER_DEF, 1, "myGetter");
    AstNode *getter_block = ast_node_create(NODE_BLOCK, 1);
    AstNode *ret_stmt = ast_node_create(NODE_RETURN, 2);
    ast_node_add_child(ret_stmt, ast_new_string_node("hello", 2));
    ast_node_add_child(getter_block, ret_stmt);
    ast_node_add_child(getter, getter_block);
    ast_node_add_child(root, getter);

    // static main()
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 3, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 3));
    ast_node_add_child(root, func_main);
    ast_node_add_child(func_main, ast_node_create(NODE_BLOCK, 3));

    return root;
}

/**
 * @brief Тест: OK (Валидное распространение типа в выражении)
 * Код:
 * static main() {
 * var a = ("a" * 3) + "b" // (String * Num) -> String. String + String -> String.
 * }
 */
AstNode *create_test_valid_type_propagation() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // ("a" * 3)
    AstNode *mul_op = ast_new_bin_op(NODE_OP_MUL, 4,
        ast_new_string_node("a", 4),
        ast_new_num_node(3.0, 4)
    );

    // ( ... ) + "b"
    AstNode *plus_op = ast_new_bin_op(NODE_OP_PLUS, 4,
        mul_op,
        ast_new_string_node("b", 4)
    );

    // a = ...
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assignment, plus_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: Ошибка 6 (Невалидное распространение типа в выражении)
 * Код:
 * static main() {
 * var a = (1 + 1) - "hello" // (Num + Num) -> Num. Num - String -> ОШИБКА
 * }
 */
AstNode *create_test_invalid_type_propagation() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // (1 + 1)
    AstNode *plus_op = ast_new_bin_op(NODE_OP_PLUS, 4,
        ast_new_num_node(1.0, 4),
        ast_new_num_node(1.0, 4)
    );

    // ( ... ) - "hello"
    AstNode *minus_op = ast_new_bin_op(NODE_OP_MINUS, 4,
        plus_op,
        ast_new_string_node("hello", 4) // <-- Ошибка 6
    );

    // a = ...
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assignment, minus_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: OK (Деление на 0-литерал)
 * Это семантически ВАЛИДНО. Ошибка (57) произойдет
 * только во время выполнения в интерпретаторе.
 * Код:
 * static main() {
 * var a = 10 / 0
 * }
 */
AstNode *create_test_valid_division_by_zero_literal() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "a"));

    // 10 / 0
    AstNode *div_op = ast_new_bin_op(NODE_OP_DIV, 4,
        ast_new_num_node(10.0, 4),
        ast_new_num_node(0.0, 4) // <-- Семантически OK
    );

    // a = ...
    AstNode *assignment = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assignment, ast_new_id_node(NODE_ID, 4, "a"));
    ast_node_add_child(assignment, div_op);
    ast_node_add_child(main_block, assignment);

    return root;
}

/**
 * @brief Тест: OK (Деление на 0-переменную)
 * Это тем более семантически ВАЛИДНО.
 * Код:
 * static main() {
 * var z = 0
 * var a = 10 / z
 * }
 */
AstNode *create_test_valid_division_by_zero_variable() {
    AstNode *root = ast_node_create(NODE_PROGRAM, 1);
    AstNode *func_main = ast_new_id_node(NODE_FUNCTION_DEF, 1, "main");
    ast_node_add_child(func_main, ast_node_create(NODE_PARAM_LIST, 1));
    ast_node_add_child(root, func_main);

    AstNode *main_block = ast_node_create(NODE_BLOCK, 2);
    ast_node_add_child(func_main, main_block);

    // var z = 0
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 3, "z"));
    AstNode *assign_z = ast_node_create(NODE_ASSIGNMENT, 4);
    ast_node_add_child(assign_z, ast_new_id_node(NODE_ID, 4, "z"));
    ast_node_add_child(assign_z, ast_new_num_node(0.0, 4));
    ast_node_add_child(main_block, assign_z);

    // var a
    ast_node_add_child(main_block, ast_new_id_node(NODE_VAR_DEF, 5, "a"));

    // 10 / z
    AstNode *div_op = ast_new_bin_op(NODE_OP_DIV, 6,
        ast_new_num_node(10.0, 6),
        ast_new_id_node(NODE_ID, 6, "z") // <-- Семантически OK
    );

    // a = ...
    AstNode *assign_a = ast_node_create(NODE_ASSIGNMENT, 6);
    ast_node_add_child(assign_a, ast_new_id_node(NODE_ID, 6, "a"));
    ast_node_add_child(assign_a, div_op);
    ast_node_add_child(main_block, assign_a);

    return root;
}


/* ================================================================== */
/* ==================== ГЛАВНАЯ ФУНКЦИЯ ТЕСТА ======================= */
/* ================================================================== */

int test_semantics() {
    printf("============================================================\n");
    printf(" Запуск тестов для Семантического Анализатора (Pass 2)...\n");
    printf("============================================================\n");

    // --- Тесты на Ошибки (ожидаем false) ---
    printf(ANSI_COLOR_CYAN "--- Тесты на Ошибки (ожидаем 'false') ---\n" ANSI_COLOR_RESET);
    // Ошибки Определений (3, 4)
    run_test("Ошибка 4: Редефиниция 'var a; var a;'", create_test_redefinition_error(), false);
    run_test("Ошибка 4: Редефиниция 'static foo(a)' и 'static foo(b)'", create_test_func_func_arity_collision_error(), false);
    run_test("Ошибка 4: Редефиниция 'static g {}' и 'static g {}'", create_test_getter_getter_collision_error(), false);
    run_test("Ошибка 4: Редефиниция 'static s=(a)' и 'static s=(b)'", create_test_setter_setter_collision_error(), false);

    run_test("Ошибка 3: Использование 'a = 10' до определения", create_test_undefined_variable_error(), false);
    run_test("Ошибка 3: Вызов неопределенной функции 'foo()'", create_test_undefined_function_error(), false);
    run_test("Ошибка 3: Нет функции main", create_test_no_main_function_error(), false);
    run_test("Ошибка 3: main с параметрами (но нет main без параметров)", create_test_main_with_params_error(), false);
    run_test("Ошибка 3: Встроенная функция - неверное имя 'Ifj.nonexistent()'", create_test_builtin_nonexistent_error(), false);
    run_test("Ошибка 3: Использование переменной из `if` во `else`", create_test_sibling_scope_error(), false);
    run_test("Ошибка 3: Использование переменной после выхода из скоупа", create_test_scope_exit_error(), false);

    // Ошибки Арности (5)
    run_test("Ошибка 5: Неверное кол-во аргументов 'foo(1, 2)'", create_test_arg_count_error(), false);
    run_test("Ошибка 5: Слишком мало аргументов 'foo(1)'", create_test_too_few_args_error(), false);
    run_test("Ошибка 5: Встроенная - арность 'Ifj.write(\"a\", \"b\")'", create_test_builtin_wrong_arity_error(), false);
    run_test("Ошибка 5: Встроенная - арность 'Ifj.read_num(10)'", create_test_builtin_arity_read_num_error(), false);
    run_test("Ошибка 5: Встроенная - арность 'Ifj.substring(\"a\", 1)'", create_test_builtin_substring_arity_error(), false);

    // Ошибки Типов во Встроенных функциях (5)
    run_test("Ошибка 5: Встроенная - тип 'Ifj.length(123)'", create_test_builtin_type_length_error(), false);
    run_test("Ошибка 5: Встроенная - тип 'Ifj.substring(1, 2, 3)'", create_test_builtin_substring_type_error(), false);
    run_test("Ошибка 5: Встроенная - тип 'Ifj.chr(\"a\")'", create_test_builtin_chr_type_error(), false);
    run_test("Ошибка 5: Встроенная - тип 'Ifj.ord(1, \"a\")'", create_test_builtin_ord_type_error(), false);

    // Ошибки Типов в Выражениях (6)
    run_test("Ошибка 6: Несовместимость типов '\"hello\" + 10'", create_test_type_mismatch_error(), false);
    run_test("Ошибка 6: Арифметика со строками '\"a\" * \"b\"'", create_test_string_arithmetic_error(), false);
    run_test("Ошибка 6: Вычитание строк '\"a\" - \"b\"'", create_test_string_subtraction_error(), false);
    run_test("Ошибка 6: Деление с null '10 / null'", create_test_division_by_null_error(), false);
    run_test("Ошибка 6: Неверное сравнение '\"a\" < \"b\"'", create_test_invalid_relational_op_error(), false);
    run_test("Ошибка 6: Неверное сравнение с null 'null < 10'", create_test_null_relational_op_error(), false);
    run_test("Ошибка 6: Невалидная итерация строки (порядок) '3 * \"ha\"'", create_test_string_iteration_order_error(), false);
    run_test("Ошибка 6: Невалидная итерация строки (не целое) '\"ha\" * 1.5'", create_test_string_iteration_float_error(), false);
    run_test("Ошибка 6: Ошибка типа в условии 'while (10 + \"a\")'", create_test_while_condition_type_error(), false);
    run_test("Ошибка 6: Ошибка типа в 'return \"a\" / 10'", create_test_return_type_error(), false);
    run_test("Ошибка 6: Сложное невалидное выражение '(a + \"b\") * ...'", create_test_invalid_complex_expression_error(), false);
    run_test("Ошибка 6: Невалидное распространение типа '(1 + 1) - \"hello\"'", create_test_invalid_type_propagation(), false);

    // Прочие семантические ошибки (10)
    run_test("Ошибка 10: Присваивание геттеру 'myGetter = 10'", create_test_assign_to_getter_error(), false);
    run_test("Ошибка 10: Чтение из сеттера 'a = mySetter'", create_test_read_from_setter_error(), false);
    run_test("Ошибка 10: Невалидный тип в 'is' (RHS не NODE_TYPE_NAME)", create_test_invalid_is_rhs_error(), false);
    run_test("Ошибка 10: Деление на 0-литерал '10 / 0'", create_test_valid_division_by_zero_literal(), false);


    // --- Тесты на Успех (ожидаем true) ---
    printf(ANSI_COLOR_CYAN "\n--- Тесты на Успех (ожидаем 'true') ---\n" ANSI_COLOR_RESET);
    run_test("Успех: Валидная программа 'var a; a=10; var b; b=a+20;'", create_test_valid_program(), true);
    run_test("Успех: Валидное затенение 'var a; if(true){ var a; }'", create_test_valid_shadowing(), true);
    run_test("Успех: Валидная перегрузка 'foo()' и 'foo(a)'", create_test_valid_overloading(), true);
    run_test("Успех: Валидная перегрузка 'main(a)' и 'main()'", create_test_valid_main_overload(), true);
    run_test("Успех: Валидное сравнение 'a == null'", create_test_valid_null_comparison(), true);
    run_test("Успех: Валидное сравнение 'num == string'", create_test_valid_mixed_type_equality(), true);
    run_test("Успех: Валидное объявление getter/setter/func 'val'", create_test_valid_getter_setter_func(), true);
    run_test("Успех: Валидное использование внешнего скоупа 'b = a + 5'", create_test_valid_outer_scope_usage(), true);
    run_test("Успех: Валидное использование 'is' 'a is Num'", create_test_valid_is_operator(), true);
    run_test("Успех: Валидное присваивание глобальной '__g = 10'", create_test_valid_global_assign(), true);
    run_test("Успех: Валидное чтение глобальной 'a = __g'", create_test_valid_global_read(), true);
    run_test("Успех: Валидная итерация строки '\"ha\" * 3'", create_test_valid_string_iteration(), true);
    run_test("Успех: Валидный 'return 10' из main", create_test_valid_return_from_main(), true);
    run_test("Успех: Валидный пустой 'return'", create_test_valid_empty_return(), true);
    run_test("Успех: Сложное валидное выражение '(a + b) * (b - 5)'", create_test_valid_complex_expression(), true);
    run_test("Успех: Валидное затенение 'var foo' (локальн.) и 'static foo()' (глобальн.)", create_test_var_func_collision_error(), true);
    run_test("Успех: Валидное объявление 'static foo()' (функ.) и 'static foo {}' (геттер)", create_test_func_getter_collision_error(), true);
    run_test("Успех: Валидное использование Геттера 'a = myGetter + 10'", create_test_valid_getter_use(), true);
    run_test("Успех: Валидное использование Сеттера 'mySetter = 10'", create_test_valid_setter_use(), true);
    run_test("Успех: Валидная простая рекурсия 'static foo() { foo() }'", create_test_valid_recursion(), true);
    run_test("Успех: Семантически валидное деление на 0-переменную 'a = 10 / z'", create_test_valid_division_by_zero_variable(), true);



    run_test("Успех: Валидная взаимная рекурсия 'foo() { bar() } bar() { foo() }'", create_test_valid_mutual_recursion(), true);
    run_test("Успех: Валидный 'return' в Геттере", create_test_valid_return_in_getter(), true);
    run_test("Успех: Валидное распространение типа '(\"a\" * 3) + \"b\"'", create_test_valid_type_propagation(), true);


    // --- Итоги ---
    printf("============================================================\n");
    if (passed_tests == total_tests) {
        printf(ANSI_COLOR_GREEN "Все %d тестов пройдены успешно!\n" ANSI_COLOR_RESET, total_tests);
    }
    else {
        printf(ANSI_COLOR_RED "Провалено %d из %d тестов.\n" ANSI_COLOR_RESET, total_tests - passed_tests, total_tests);
    }
    printf("============================================================\n");

    return (passed_tests == total_tests) ? 0 : 1;
}

void test_gen_code(){
    AstNode *root = create_test_valid_main_overload();
    analyze_semantics(root);
    TACDLList tac_list;
    TACDLL_Init(&tac_list);
    generate_tac(root, &tac_list, &global_table);
    optimize_tac(&tac_list);
    print_tac_list(&tac_list);
    generate_code(&tac_list, &global_table);
    TACDLL_Dispose(&tac_list);
    symtable_free(&global_table);
}
int main() {
    test_gen_code();
    return 0;
}
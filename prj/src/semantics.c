/** 
 * semantics.c - Семантический анализ
 * ШАГ 0 - подготовка, заполнение глобальной таблицы символов встроенными функциями
 * ШАГ 2A - сбор функций, проход по АСТ добавить в global_table
 * ШАГ 2Б - семантический анализ тела функций
 */

#include "symtable.h"
#include "ast.h"
#include <stdio.h> 
#include <stdlib.h>

Symtable global_table; // Глобальная таблица символов

/**
 * TODO: Главная функция семантического анализатора.
 */
bool analyze_semantics(AstNode* root) {
    
    // 1. Инициализируем нашу global_table
    if (!symtable_init(&global_table)) {
        // Это внутренняя ошибка компилятора (Ошибка 99)
        fprintf(stderr, "Internal Compiler Error: Failed to init global_table.\n");
        return false;
    }

    //* --- ШАГ 0: Регистрация Встроенных Функций ---

    // static Ifj.read_str() -> String | Null
    if (!register_builtin_function("Ifj.read_str", 0, TYPE_STR)) return false;
    
    // static Ifj.read_num() -> Num | Null
    if (!register_builtin_function("Ifj.read_num", 0, TYPE_NUM)) return false;

    // static Ifj.write(term) -> Null
    if (!register_builtin_function("Ifj.write", 1, TYPE_NIL)) return false;

    // static Ifj.floor(term: Num) -> Num
    if (!register_builtin_function("Ifj.floor", 1, TYPE_NUM)) return false;

    // static Ifj.substring(s: String, i: Num, j: Num) -> String | Null
    if (!register_builtin_function("Ifj.substring", 3, TYPE_STR)) return false;

    // static Ifj.strcmp(s1: String, s2: String) -> Num
    if (!register_builtin_function("Ifj.strcmp", 2, TYPE_NUM)) return false;

    // static Ifj.ord(s: String, i: Num) -> Num
    if (!register_builtin_function("Ifj.ord", 2, TYPE_NUM)) return false;

    // static Ifj.chr(i: Num) -> String
    if (!register_builtin_function("Ifj.chr", 1, TYPE_STR)) return false;

    // static Ifj.str(term) -> String
    if (!register_builtin_function("Ifj.str", 1, TYPE_STR)) return false;

    // static Ifj.length(s: String) -> Num
    if (!register_builtin_function("Ifj.length", 1, TYPE_NUM)) return false;

    printf("DEBUG: Step 0 completed. Global table populated with ALL built-in functions.\n");



    //* --- ШАГ 2a: Сбор Пользовательских Функций ---

    if (root == NULL || root->type != NODE_PROGRAM) {
        fprintf(stderr, "Internal Error: AST root is not NODE_PROGRAM.\n");
        symtable_free(&global_table);
        return false;
    }

    // 2. Идем по всем дочерним узлам NODE_PROGRAM
    for (AstNode* node = root->child; node != NULL; node = node->sibling) {
        
        // Нас интересуют только определения функций
        if (node->type == NODE_FUNCTION_DEF || 
            node->type == NODE_GETTER_DEF || 
            node->type == NODE_SETTER_DEF) 
        {
            
            // TODO Вся магия происходит здесь
            if (!process_function_declaration(node)) {
                // Ошибка
                symtable_free(&global_table);
                return false;
            }
        }
    }
    
    printf("DEBUG: Step 2a completed. User functions collected.\n");

    // В конце чистим
    symtable_free(&global_table);
    return true;
}


/**
 * Вспомогательная функция для регистрации встроенной функции в global_table.
 *
 * @param name Имя функции (напр. "Ifj.write")
 * @param arity Количество параметров
 * @param return_type Тип возвращаемого значения (напр. TYPE_NIL)
 */
static bool register_builtin_function(const char* name, int arity, DataType return_type) {
    
    // 1. Создаем "искаженное имя"
    char mangled_name[256];
    sprintf(mangled_name, "%s@%d", name, arity);

    // 2. Создаем SymbolData для этой функции
    SymbolData data;
    data.kind = KIND_FUNC;       // Это функция
    data.data_type = return_type; // Что она возвращает
    data.is_defined = true;      // Встроенные функции всегда "определены"
    data.local_table = NULL;     // У них нет локальной таблицы IFJ25

    // 3. Вставляем в global_table
    if (!symtable_insert(&global_table, mangled_name, &data)) {
        // Ошибка 99
        fprintf(stderr, "Internal Error: Failed to insert builtin '%s'\n", mangled_name);
        return false;
    }

    return true;
}


/**
 * Вспомогательная функция для подсчета параметров функции.
 * @param func_node Узел NODE_FUNCTION_DEF (или SETTER).
 * @return Количество параметров.
 */
static int count_parameters(AstNode* func_node) {
    if (func_node->type == NODE_SETTER_DEF) {
        return 1; // Сеттер всегда имеет 1 параметр
    }
    if (func_node->type == NODE_GETTER_DEF) {
        return 0; // Геттер всегда имеет 0 параметров
    }

    // Для NODE_FUNCTION_DEF 
    AstNode* param_list = func_node->child;
    if (param_list == NULL || param_list->type != NODE_PARAM_LIST) {
        // Это геттер или тело функции, у него нет списка параметров
        return 0; 
    }

    int arity = 0;
    // Идем по списку NODE_PARAM
    for (AstNode* param = param_list->child; param != NULL; param = param->sibling) {
        if (param->type == NODE_PARAM) {
            arity++;
        }
    }
    return arity;
}


/**
 * (Шаг 2a) Обрабатывает одно объявление функции из AST.
 * Находит имя, арность, проверяет на ре-дефиницию,
 * создает local_table и вставляет в global_table.
 *
 * @param func_node Узел (NODE_FUNCTION_DEF, GETTER, SETTER).
 * @return true в случае успеха, false при ошибке.
 */
static bool process_function_declaration(AstNode* func_node) {
    
    // 1. Получаем Имя
    const char* name = func_node->data.identifier;
    if (name == NULL) {
        fprintf(stderr, "Internal Error: Function node has no name.\n");
        return false;
    }

    // 2. Считаем Арность
    int arity = count_parameters(func_node);

    // 3. Создаем "Mangled Name" (Ключ)
    char mangled_name[256];
    if (func_node->type == NODE_SETTER_DEF) {
        //Сеттер и геттер имеет внутри "="б поэтому что бы не путать с функцией которая может
        // иметь такое же имя и арность дадим им уникальный суффикс 
        sprintf(mangled_name, "%s@setter", name); // e.g., "myVar@setter"
    } else if (func_node->type == NODE_GETTER_DEF){
        sprintf(mangled_name, "%s@getter", name); // e.g., "myVar@getter"
    } else {
        sprintf(mangled_name, "%s@%d", name, arity); // e.g., "foo@1"
    }

    // 4. Проверка (Ошибка 4 - Ре-дефиниция)
    if (symtable_lookup(&global_table, mangled_name) != NULL) {
        // Эта функция уже была определена.
        fprintf(stderr, "Semantic Error (Line %d): Redefinition of function '%s' with %d parameters.\n",
                func_node->line_number, name, arity);
        return false; 
    }

    // 5. Создаем SymbolData
    SymbolData data;
    data.kind = KIND_FUNC;
    data.data_type = TYPE_NIL; // По умолчанию, функция возвращает nil
    data.is_defined = false;   // Мы еще НЕ анализировали тело (смотреть 2б)
    
    // 6. Создаем 'local_table' для этой функции.
    // Это *постоянное* хранилище для Pass 3.
    data.local_table = (Symtable*)malloc(sizeof(Symtable));
    if (data.local_table == NULL) {
        fprintf(stderr, "Internal Error: Failed to malloc local_table for '%s'.\n", name);
        return false;
    }
    if (!symtable_init(data.local_table)) {
        fprintf(stderr, "Internal Error: Failed to init local_table for '%s'.\n", name);
        free(data.local_table);
        return false;
    }

    // 7. Вставляем в global_table
    if (!symtable_insert(&global_table, mangled_name, &data)) {
        fprintf(stderr, "Internal Error: Failed to insert function '%s' into global_table.\n", name);
        symtable_free(data.local_table); // Очищаем то, что создали
        free(data.local_table);
        return false;
    }
    
    // 8. Связываем узел AST с этой TableEntry
    // 
    // Мы только что вставили, так что можем сразу найти
    func_node->table_entry = symtable_lookup(&global_table, mangled_name);

    return true;
}



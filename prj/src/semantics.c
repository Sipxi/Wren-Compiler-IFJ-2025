/**
 * semantics.c - Семантический анализ
 * ШАГ 0 - подготовка, заполнение глобальной таблицы символов встроенными функциями
 * ШАГ 2A - сбор функций, проход по АСТ добавить в global_table
 * ШАГ 2Б - семантический анализ тела функций
 */

#include "symtable.h"
#include "ast.h"
#include "semantics.h"
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <math.h>

 ///* =======================================================================================================*/
 /*
   Простой стек для указателей Symtable* (для Pass 2b)
 */

#define HIERARCHY_STACK_SIZE 100

typedef struct {
    Symtable *array[HIERARCHY_STACK_SIZE];
    int topIndex;
} ScopeStack;

/**
 * @brief Инициализирует стек Symtable*
 */
static void H_Stack_Init(ScopeStack *stack) {
    stack->topIndex = -1;
}

/**
 * @brief Помещает Symtable* на вершину.
 */
static bool H_Stack_Push(ScopeStack *stack, Symtable *table) {
    if (stack->topIndex >= HIERARCHY_STACK_SIZE - 1) {
        fprintf(stderr, "Internal Error: Scope stack overflow (too many nested blocks).\n");
        exit(99);
    }
    stack->topIndex++;
    stack->array[stack->topIndex] = table;
    return true;
}

/**
 * @brief Снимает Symtable* с вершины.
 */
static Symtable *H_Stack_Pop(ScopeStack *stack) {
    if (stack->topIndex == -1) {
        return NULL;
    }
    Symtable *table = stack->array[stack->topIndex];
    stack->topIndex--;
    return table;
}

/**
 * @brief "Смотрит" на Symtable* на вершине.
 */
static Symtable *H_Stack_Peek(ScopeStack *stack) {
    if (stack->topIndex == -1) {
        return NULL;
    }
    return stack->array[stack->topIndex];
}

/**
 * @brief (КЛЮЧЕВАЯ ФУНКЦИЯ) Ищет 'name' вниз по стеку.
 */
static TableEntry *H_Stack_Find_Var(ScopeStack *stack, const char *name) {
    for (int i = stack->topIndex; i >= 0; i--) {
        TableEntry *entry = symtable_lookup(stack->array[i], name);
        if (entry != NULL) {
            return entry; // Найдено!
        }
    }
    // Если не нашли в стеке, ищем в global_table
    return symtable_lookup(&global_table, name);
}

/**
 * @brief Проверяет, является ли значение типа double целым числом.
 * * Функция возвращает true, если число является целым (например, 2.0, -5.0),
 * и false, если оно является дробным (например, 2.5, 0.0001).
 *
 * @param value Значение типа double для проверки.
 * @return bool Возвращает true, если число целое, false, если дробное.
 */
bool is_whole_number(double value);


///* =======================================================================================================*/

static bool register_builtin_function(const char *name, int arity, DataType return_type);
static bool process_function_declaration(AstNode *func_node);
static bool analyze_function_body(AstNode *func_node);
static bool analyze_statement(AstNode *node, ScopeStack *stack, int *block_cnt, int current_scope_id);
static bool analyze_expression(AstNode *node, ScopeStack *stack, DataType *result_type);

Symtable global_table; // Глобальная таблица символов


bool is_whole_number(double value) {
    // В C, при работе с double, всегда необходимо использовать малую константу
    // (EPSILON) для сравнения с нулем.
    const double EPSILON = 1e-9;

    // 1. Усечение (truncation) дробной части путем приведения к long long.
    // long long выбран для максимального диапазона целых чисел.
    double truncated_value = (double)((long long)value);

    // 2. Вычисляем разницу между исходным и усеченным значением (это и есть дробная часть).
    double difference = value - truncated_value;

    // 3. Проверяем, что разница близка к нулю, не используя fabs() (которая требует -lm).
    // Разница (difference) всегда будет в диапазоне (-1.0, 1.0).
    // Если она находится в пределах [-EPSILON, EPSILON], это целое число.
    return (difference < EPSILON) && (difference > -EPSILON);
}

/**
 * Главная функция семантического анализатора.
 */
bool analyze_semantics(AstNode *root) {

    // 1. Инициализируем нашу global_table
    if (!symtable_init(&global_table)) {
        // Это внутренняя ошибка компилятора (Ошибка 99)
        fprintf(stderr, "Internal Compiler Error: Failed to init global_table.\n");
        exit(99);
    }

    //* --- ШАГ 0: Регистрация Встроенных Функций ---

    // static Ifj.read_str() -> String | Null
    if (!register_builtin_function("Ifj.read_str", 0, TYPE_STR)) exit(99);

    // static Ifj.read_num() -> Num | Null
    if (!register_builtin_function("Ifj.read_num", 0, TYPE_NUM)) exit(99);

    // static Ifj.write(term) -> Null
    if (!register_builtin_function("Ifj.write", 1, TYPE_NIL)) exit(99);

    // static Ifj.floor(term: Num) -> Num
    if (!register_builtin_function("Ifj.floor", 1, TYPE_NUM)) exit(99);

    // static Ifj.substring(s: String, i: Num, j: Num) -> String | Null
    if (!register_builtin_function("Ifj.substring", 3, TYPE_STR)) exit(99);

    // static Ifj.strcmp(s1: String, s2: String) -> Num
    if (!register_builtin_function("Ifj.strcmp", 2, TYPE_NUM)) exit(99);

    // static Ifj.ord(s: String, i: Num) -> Num
    if (!register_builtin_function("Ifj.ord", 2, TYPE_NUM)) exit(99);

    // static Ifj.chr(i: Num) -> String
    if (!register_builtin_function("Ifj.chr", 1, TYPE_STR)) exit(99);

    // static Ifj.str(term) -> String
    if (!register_builtin_function("Ifj.str", 1, TYPE_STR)) exit(99);

    // static Ifj.length(s: String) -> Num
    if (!register_builtin_function("Ifj.length", 1, TYPE_NUM)) exit(99);

    //printf("DEBUG: Step 0 completed. Global tagit ble populated with ALL built-in functions.\n");

    //* --- ШАГ 2a: Сбор Пользовательских Функций ---

    if (root == NULL || root->type != NODE_PROGRAM) {
        fprintf(stderr, "Internal Error: AST root is not NODE_PROGRAM.\n");
        symtable_free(&global_table);
        exit(99);
    }


    // 2. Идем по всем дочерним узлам NODE_PROGRAM
    for (AstNode *node = root->child; node != NULL; node = node->sibling) {
        // Нас интересуют только определения функций
        if (node->type == NODE_FUNCTION_DEF ||
            node->type == NODE_GETTER_DEF ||
            node->type == NODE_SETTER_DEF)
        {
            if (!process_function_declaration(node)) {
                // Ошибка
                symtable_free(&global_table);
                exit(4);
            }
        }
    }

    //printf("DEBUG: Step 2a completed. User functions collected.\n");

    //* --- ШАГ 2b: Анализ Тел Функций ---

    // 1. Снова проходим по всем узлам функций
    for (AstNode *node = root->child; node != NULL; node = node->sibling) {
        if (node->type == NODE_FUNCTION_DEF ||
            node->type == NODE_GETTER_DEF ||
            node->type == NODE_SETTER_DEF)
        {
            // 2. Вызываем новую функцию для анализа ТЕЛА
            if (!analyze_function_body(node)) {
                symtable_free(&global_table);
                exit(10);
            }
        }
    }

    //printf("DEBUG: Step 2b completed. All function bodies analyzed.\n");

    //* --- Финальная Проверка: main@0 ---

    // 3. Проверяем, что 'main' без параметров существует
    TableEntry *main_entry = symtable_lookup(&global_table, "main$0");

    if (main_entry == NULL) {
        fprintf(stderr, "Semantic Error: Function 'main()' is not defined.\n");
        symtable_free(&global_table);
        exit(3);
    }

    // убедимся, что она была определена (т.е. у нее было тело)
    if (main_entry->data->is_defined == false) {
        fprintf(stderr, "Semantic Error: Function 'main()' is declared but not defined.\n");
        symtable_free(&global_table);
        exit(3);
    }

    //printf("DEBUG: Final check completed. 'main$0' found and defined.\n");

    return true;
}

/**
 * Вспомогательная функция для создания уникального имени.
 * @param original_name Оригинальное имя (напр., "a").
 * @param scope_id Идентификатор области видимости (напр., 2).
 * @return Новое уникальное имя (напр., "a$2").
 */
static char *create_unique_name(const char *original_name, int scope_id) {
    // Длина = имя + '$' + число + '\0'
    // int (2 млрд) занимает макс 10 цифр.
    size_t len = strlen(original_name) + 1 + 10 + 1;
    char *new_name = (char *)malloc(len);
    if (new_name) {
        sprintf(new_name, "%s$%d", original_name, scope_id);
    }
    return new_name;
}

/**
 * Вспомогательная функция для регистрации встроенной функции в global_table.
 *
 * @param name Имя функции (напр. "Ifj.write")
 * @param arity Количество параметров
 * @param return_type Тип возвращаемого значения (напр. TYPE_NIL)
 */
static bool register_builtin_function(const char *name, int arity, DataType return_type) {

    // 1. Создаем "искаженное имя"
    //? Подумать про маллок
    char mangled_name[256];
    sprintf(mangled_name, "%s$%d", name, arity);

    //? Symbol data create_symbol_data(KIND_FUNC, return_type, true, NULL);
    // 2. Создаем SymbolData для этой функции
    SymbolData data;
    data.kind = KIND_FUNC;       // Это функция
    data.data_type = return_type; // Что она возвращает
    data.is_defined = true;      // Встроенные функции всегда "определены"
    data.unique_name = NULL;     // Встроенные функции не нуждаются в уникальном имени

    // 3. Вставляем в global_table
    if (!symtable_insert(&global_table, mangled_name, &data)) {
        // Ошибка 99
        fprintf(stderr, "Internal Error: Failed to insert builtin '%s'\n", mangled_name);
        exit(99);
    }

    // Явно указываем, что у встроенной функции нет таблицы
    TableEntry *entry = symtable_lookup(&global_table, mangled_name);
    if (entry) {
        entry->local_table = NULL;
    }

    return true;
}

/**
 * Вспомогательная функция для подсчета параметров функции.
 * @param func_node Узел NODE_FUNCTION_DEF (или SETTER).
 * @return Количество параметров.
 */
static int count_parameters(AstNode *func_node) {
    if (func_node->type == NODE_SETTER_DEF) {
        return 1; // Сеттер всегда имеет 1 параметр
    }
    if (func_node->type == NODE_GETTER_DEF) {
        return 0; // Геттер всегда имеет 0 параметров
    }

    // Для NODE_FUNCTION_DEF 
    AstNode *param_list = func_node->child;
    if (param_list == NULL || param_list->type != NODE_PARAM_LIST) {
        // Это геттер или тело функции, у него нет списка параметров
        return 0;
    }

    int arity = 0;
    // Идем по списку NODE_PARAM
    for (AstNode *param = param_list->child; param != NULL; param = param->sibling) {
        if (param->type == NODE_PARAM) {
            arity++;
        }
    }
    return arity;
}

/**
 * (Шаг 2a) Обрабатывает одно объявление функции из AST.
 * (ПЕРЕПИСАНО для Иерархической Модели)
 *
 * 1. Создает 'mangled_name' (напр., "main@0").
 * 2. Проверяет на ре-дефиницию (Ошибка 4) в 'global_table'.
 * 3. Создает SymbolData (БЕЗ local_table).
 * 4. Вставляет (SymbolData, "main@0") в 'global_table'.
 * 5. Находит 'TableEntry' ("main@0") в 'global_table'.
 * 6. Malloc'ает и init'ит НОВУЮ Symtable (Уровень 1).
 * 7. Привязывает эту новую Symtable к 'func_entry->local_table'.
 * 8. Связывает 'func_node->table_entry' с 'func_entry'.
 *
 * @param func_node Узел (NODE_FUNCTION_DEF, GETTER, SETTER).
 * @return true в случае успеха, false при ошибке.
 */
static bool process_function_declaration(AstNode *func_node) {

    // 1. Получаем Имя
    const char *name = func_node->data.identifier;
    if (name == NULL) {
        fprintf(stderr, "Internal Error: Function node has no name.\n");
        exit(99);
    }

    // 2. Считаем Арность
    int arity = count_parameters(func_node);

    // 3. Создаем "Mangled Name" (Ключ)
    char mangled_name[256];
    if (func_node->type == NODE_SETTER_DEF) {
        sprintf(mangled_name, "%s$setter", name);
    }
    else if (func_node->type == NODE_GETTER_DEF) {
        sprintf(mangled_name, "%s$getter", name);
    }
    else {
        sprintf(mangled_name, "%s$%d", name, arity);
    }

    // 4. Проверка (Ошибка 4 - Ре-дефиниция)
    if (symtable_lookup(&global_table, mangled_name) != NULL) {
        fprintf(stderr, "Semantic Error (Line %d): Redefinition of function '%s'.\n",
            func_node->line_number, name);
        exit(4);
    }

    // 5. Создаем SymbolData (БЕЗ local_table)
    SymbolData data;
    data.kind = KIND_FUNC;
    data.data_type = TYPE_UNKNOWN; // По умолчанию, функция возвращает unknown
    data.is_defined = false;   // Мы еще НЕ анализировали тело (Шаг 2b)
    data.unique_name = NULL;   // Функции не нуждаются в уникальном имени

    // 6. Вставляем в global_table
    if (!symtable_insert(&global_table, mangled_name, &data)) {
        fprintf(stderr, "Internal Error: Failed to insert function '%s' into global_table.\n", name);
        exit(99);
    }

    // 7. Создаем 'local_table' (Уровень 1) и привязываем к TableEntry

    // 7a. Находим TableEntry (Уровень 0), который мы только что создали
    TableEntry *func_entry = symtable_lookup(&global_table, mangled_name);
    if (func_entry == NULL) {
        fprintf(stderr, "Internal Error: Failed to re-lookup function '%s'.\n", name);
        exit(99);
    }

    // 7b. malloc №1: Создаем Symtable (Уровень 1)
    func_entry->local_table = (Symtable *)malloc(sizeof(Symtable));
    if (func_entry->local_table == NULL) {
        fprintf(stderr, "Internal Error: Failed to malloc local_table for '%s'.\n", name);
        exit(99);
    }

    // 7c. malloc №2: Инициализируем ее (она malloc'нет 'entries')
    if (!symtable_init(func_entry->local_table)) {
        fprintf(stderr, "Internal Error: Failed to init local_table for '%s'.\n", name);
        free(func_entry->local_table);
        func_entry->local_table = NULL;
        exit(99);
    }

    // 8. Связываем узел AST
    func_node->table_entry = func_entry;

    return true;
}

/**
 * (Шаг 2b) Анализирует тело ОДНОЙ функции.
 *
 * 1. Берет 'func_local_table' (Уровень 1), созданный в Шаге 2a.
 * 2. Инициализирует H_Stack (стек Symtable*).
 * 3. Помещает 'func_local_table' (Уровень 1) в стек.
 * 4. Обрабатывает параметры и вставляет их в 'func_local_table'.
 * 5. Вызывает 'analyze_statement' для тела (NODE_BLOCK).
 * 6. Снимает 'func_local_table' со стека.
 *
 * @param func_node Узел (NODE_FUNCTION_DEF, GETTER, SETTER).
 * @return true в случае успеха, false при семантической ошибке.
 */
/**/
static bool analyze_function_body(AstNode *func_node)
{
    //printf("DEBUG: Analyzing body for '%s' (Hierarchical)...\n", func_node->data.identifier);

    // 1. Получаем 'func_entry' (из Шага 2a)
    TableEntry *func_entry = func_node->table_entry;
    if (func_entry == NULL) {
        fprintf(stderr, "Internal Error: func_node->table_entry is NULL for '%s'.\n", func_node->data.identifier);
        exit(99);
    }

    // 2. Получаем 'func_local_table' (Уровень 1)
    Symtable *func_local_table = func_entry->local_table;
    if (func_local_table == NULL) {
        fprintf(stderr, "Internal Error: func_local_table is NULL for '%s'.\n", func_node->data.identifier);
        exit(99);
    }

    // 3. Создаем временный ScopeStack
    ScopeStack stack;
    H_Stack_Init(&stack);

    // 4. Помещаем Уровень 1 в 'stack'
    if (!H_Stack_Push(&stack, func_local_table)) {
        exit(99);
    }

    int local_block_cnt = 0; // Счетчик вложенных блоков
    int current_scope_id = 0; // Идентификатор текущей области видимости

    // 5. Определяем, где начало списка параметров и где тело
    AstNode *param_iter = NULL;
    AstNode *body_node = NULL;

    if (func_node->type == NODE_FUNCTION_DEF || func_node->type == NODE_SETTER_DEF) {
        AstNode* param_list = func_node->child;
        
        // Проверяем, действительно ли это список
        if (param_list != NULL && param_list->type == NODE_PARAM_LIST) {
            param_iter = param_list->child;   // Первый параметр внутри списка
            body_node = param_list->sibling;  // Тело функции идет после списка
        } else {
            fprintf(stderr, "Internal Error: Function/Setter '%s' missing param list.\n", func_node->data.identifier);
            H_Stack_Pop(&stack);
            exit(99);
        }
    } 
    else { 
        // GETTER (без параметров)
        param_iter = NULL;
        body_node = func_node->child;
    }

    // 6. (Часть 2) Обрабатываем параметры.
    for (AstNode* param = param_iter; param != NULL; param = param->sibling) {
        
        if (param->type != NODE_PARAM) continue;

        const char* param_name = param->data.identifier;
        //printf("DEBUG: Found parameter '%s' in function '%s'\n", param_name, func_node->data.identifier);
        // 6a. Проверка (Ошибка 4 - дубcликат параметра)
        if (symtable_lookup(func_local_table, param_name) != NULL) {
            fprintf(stderr, "Semantic Error (Line %d): Duplicate parameter name '%s'.\n",
                param->line_number, param_name);
            H_Stack_Pop(&stack); // Очищаем стек
            exit(4);
        }

        // 6b. Создаем данные для Symtable
        SymbolData data;
        data.kind = KIND_VAR;
        data.data_type = TYPE_UNKNOWN;;
        data.is_defined = true;

        data.unique_name = create_unique_name(param_name, 0); // Параметры функции имеют scope_id = 0

        // 6c. Вставляем в 'func_local_table' (Уровень 1)
        if (!symtable_insert(func_local_table, param_name, &data)) {
            H_Stack_Pop(&stack);
            fprintf(stderr, "Internal Error: Failed to insert param '%s'.\n", param_name);
            exit(99);
        }

        // 6d. (Устанавливаем local_table = NULL для TableEntry
        TableEntry *entry = symtable_lookup(func_local_table, param_name);
        if (entry == NULL) {
            H_Stack_Pop(&stack);
            fprintf(stderr, "Internal Error: Failed to re-lookup param '%s'.\n", param_name);
            exit(99);
        }
        entry->local_table = NULL; // У параметра нет вложенной таблицы

        // 6e. Связываем узел AST
        param->table_entry = entry;
    }

    // 7. Вызываем analyze_statement для ТЕЛА функции
    if (body_node == NULL || body_node->type != NODE_BLOCK) {
        fprintf(stderr, "Internal Error: Function '%s' has no NODE_BLOCK body.\n", func_node->data.identifier);
        H_Stack_Pop(&stack);
        exit(2);
    }

    // Запускаем рекурсивный анализ
    if (!analyze_statement(body_node, &stack, &local_block_cnt, current_scope_id)) {
        H_Stack_Pop(&stack); // Стек мог остаться грязным, если ошибка была в блоке
        exit(10);
    }

    // 8. Если мы дошли сюда без ошибок:
    func_entry->data->is_defined = true;

    // 9. Очищаем стек
    H_Stack_Pop(&stack);
    // (стек теперь пуст)

    return true;
}




/*
        Young dope dealer, sellin' dope, is you like that? (If you like that, yeah, yeah)
        Kickin' doors, kickin' in doors, is you like that? (How?, yeah)
        Young throwed nigga, sellin' lows, is you like that? (Holy water, Holy water, yeah)
        All 24, you on go, is you like that? (If you like that)
        Niggas from the bottom really like that (if you like that) (he was once a thug, he was, he-)
        Steppin' in Balencis if you like that (if you like that) (he was once a thug, he was, he-)
        Pop another bottle if you like that (if you like that) (he was once a thug, he was, he-)

    (c) "Like That" - Future, Kendrick Lamar, and Metro Boomin (2024)
*/


// Нам нужен счетчик для УНИКАЛЬНЫХ имен блоков (напр., "__block_0", "__block_1")
// Мы можем сделать его глобальным для этого .c файла.
static int block_counter = 0;

/**
 * (Шаг 2b) Рекурсивно анализирует узел-оператор (statement).
 *
 * @param node Узел AST для анализа.
 * @param stack *Временный* стек Symtable* (H_Stack).
 * @return true в случае успеха, false при семантической ошибке.
 */

static bool analyze_statement(AstNode *node, ScopeStack *stack, int *block_cnt, int current_scope_id)
{
    if (node == NULL) {
        return true;
    }

    switch (node->type) {

        //* --- СЛУЧАЙ 1: Блок { ... } (Создает Уровень 2, 3...) ---
    case NODE_BLOCK: {
        //printf("DEBUG: Entering NODE_BLOCK (Hierarchical).\n");

        // 1. Получаем родительскую таблицу (напр., Уровень 1)
        Symtable *parent_table = H_Stack_Peek(stack);
        if (parent_table == NULL) {
            fprintf(stderr, "Internal Error: Scope stack is empty inside NODE_BLOCK.\n");
            exit(99);
        }

        // 2. Создаем НОВУЮ local_table (напр., Уровень 2)
        // malloc №1:
        Symtable *new_block_table = (Symtable *)malloc(sizeof(Symtable));
        if (new_block_table == NULL) { exit(99); }

        // malloc №2:
        if (!symtable_init(new_block_table)) {
            free(new_block_table);
            exit(99);
        }

        // 3. Создаем "папку" (TableEntry) для этого блока
        //    внутри родительской таблицы
        SymbolData data;
        data.kind = KIND_BLOCK;
        data.data_type = TYPE_NIL;
        data.is_defined = true;

        char block_key[256];
        sprintf(block_key, "__block_%d", block_counter++);

        if (!symtable_insert(parent_table, block_key, &data)) {
            symtable_free(new_block_table);
            free(new_block_table);
            exit(99);
        }

        // 4. (КЛЮЧ) Привязываем Уровень 2 к Уровню 1
        TableEntry *block_entry = symtable_lookup(parent_table, block_key);
        block_entry->local_table = new_block_table;

        // 5. "Входим" в Уровень 2
        if (!H_Stack_Push(stack, new_block_table)) {
            exit(99);
        }

        (*block_cnt)++;
        int new_scope_id = *block_cnt;

        // 6. Рекурсивно анализируем *все* операторы внутри блока
        bool result = true;
        for (AstNode *stmt = node->child; stmt != NULL; stmt = stmt->sibling) {
            if (!analyze_statement(stmt, stack, block_cnt, new_scope_id)) {
                result = false;
                break;
            }
        }

        // 7. "Выходим" из Уровня 2
        H_Stack_Pop(stack);

        //printf("DEBUG: Exiting NODE_BLOCK (Hierarchical).\n");
        return result;
    }

                   //* --- СЛУЧАЙ 2: Определение 'var id' ---
    case NODE_VAR_DEF: {
        //printf("DEBUG: Entering NODE_VAR_DEF (%s) [Hierarchical].\n", node->data.identifier);

        const char *name = node->data.identifier;

        // 1. Получаем текущую таблицу (вершина стека)
        Symtable *current_table = H_Stack_Peek(stack);
        if (current_table == NULL) {
            exit(99);
        }

        // 2. Проверка (Ошибка 4 - Ре-дефиниция в *этом* блоке)
        // Ищем только в текущей таблице уровня.
        if (symtable_lookup(current_table, name) != NULL) {
            fprintf(stderr, "Semantic Error (Line %d): Redefinition of variable '%s' in the same scope.\n",
                node->line_number, name);
            exit(4);
        }

        // 3. Создаем данные
        SymbolData data;
        data.kind = KIND_VAR;
        data.data_type = TYPE_NIL; // По умолчанию null
        data.is_defined = true;
        // data.local_table НЕТ (это переменная)
        data.unique_name = create_unique_name(name, current_scope_id);

        // 4. Вставляем в текущую таблицу (Без искажения имени!)
        if (!symtable_insert(current_table, name, &data)) {
            exit(99);
        }

        // 5. Связываем AST
        TableEntry *entry = symtable_lookup(current_table, name);
        entry->local_table = NULL; // У переменной нет вложенной таблицы
        node->table_entry = entry;

        return true;
    }

                     //* --- СЛУЧАЙ 3: Присваивание 'id = ...' ---
    case NODE_ASSIGNMENT: {
        //printf("DEBUG: Entering NODE_ASSIGNMENT.\n");

        AstNode *id_node = node->child;
        AstNode *expr_node = node->child->sibling;

        // 1. Анализируем правую часть
        DataType expr_type;
        if (!analyze_expression(expr_node, stack, &expr_type)) {
            exit(10);
        }

        if (expr_type == TYPE_BOOL) {
            // ... (Ошибка 6: нельзя присваивать Bool) ...
            exit(6);
        }

        const char *name = id_node->data.identifier;
        TableEntry *entry = NULL;

        // 2. Ищем в стеке (Уровень N -> ... -> Уровень 1)
        entry = H_Stack_Find_Var(stack, name);

        if (entry != NULL) {
            // --- НАШЛИ ЛОКАЛЬНУЮ ПЕРЕМЕННУЮ ---
            if (entry->data->kind == KIND_FUNC) {
                // ... (Ошибка: присваивание функции) ...
                exit(10);
            }
            id_node->table_entry = entry;
            entry->data->data_type = expr_type; // Обновляем тип
            return true;
        }

        // 3. Если не нашли в стеке, проверяем Сеттеры и Глобальные (Уровень 0)

        // 3a. Сеттер
        char mangled_setter[256];
        sprintf(mangled_setter, "%s$setter", name);
        entry = symtable_lookup(&global_table, mangled_setter);
        if (entry != NULL) {
            id_node->table_entry = entry;
            return true;
        }

        // 3b. Глобальная переменная ('__')
        if (strncmp(name, "__", 2) == 0) {
            entry = symtable_lookup(&global_table, name);

            if (entry == NULL) {
                // Создаем новую глобальную
                SymbolData data;
                data.kind = KIND_VAR;
                data.data_type = expr_type;
                data.is_defined = true;
                data.unique_name = create_unique_name(name, 0); // Глобальные имеют scope_id = 0

                symtable_insert(&global_table, name, &data);
                entry = symtable_lookup(&global_table, name);
                entry->local_table = NULL;
            }
            else if (entry->data->kind == KIND_FUNC) {
                // ... (Ошибка) ...
                exit(10);
            }
            id_node->table_entry = entry;
            entry->data->data_type = expr_type;
            return true;
        }

        // 4. Ошибка 3
        fprintf(stderr, "Semantic Error: Undefined variable '%s'.\n", name);
        exit(3);
    }

                        //* --- СЛУЧАЙ 4: Условие 'if (cond) { ... } else { ... }' ---
    case NODE_IF: {
        //printf("DEBUG: Entering NODE_IF.\n");

        AstNode *cond_node = node->child;
        AstNode *if_body = node->child->sibling;
        AstNode *else_body = node->child->sibling->sibling;

        // 1. Анализируем Условие
        DataType cond_type;
        // В новой модели мы передаем ТОЛЬКО 'stack' (и 'cond_type')
        if (!analyze_expression(cond_node, stack, &cond_type)) {
            exit(10);
        }

        // 2. Анализируем 'if' тело
        if (!analyze_statement(if_body, stack, block_cnt, current_scope_id)) {
            exit(10);
        }

        // 3. Анализируем 'else' тело
        if (!analyze_statement(else_body, stack, block_cnt, current_scope_id)) {
            exit(10);
        }

        return true;
    }

                //* --- СЛУЧАЙ 5: Цикл 'while (cond) { ... }' ---
    case NODE_WHILE: {
        //printf("DEBUG: Entering NODE_WHILE.\n");

        AstNode *cond_node = node->child;
        AstNode *while_body = node->child->sibling;

        // 1. Анализируем Условие
        DataType cond_type;
        if (!analyze_expression(cond_node, stack, &cond_type)) {
            exit(10);
        }

        // 2. Анализируем Тело
        if (!analyze_statement(while_body, stack, block_cnt, current_scope_id)) {
            exit(10);
        }

        return true;
    }

                   //* --- СЛУЧАЙ 6: Возврат 'return ...' ---
    case NODE_RETURN: {
        //printf("DEBUG: Entering NODE_RETURN.\n");

        AstNode *expr_node = node->child;

        // 1. Анализируем возвращаемое выражение
        DataType return_type;
        if (!analyze_expression(expr_node, stack, &return_type)) {
            exit(10);
        }

        return true;
    }

                    //* --- СЛУЧАЙ 7: Самостоятельный вызов 'id(...)' ---
    case NODE_CALL_STATEMENT: {
        // Получаем имя из ребенка (для дебага)
        // const char* name = (node->child) ? node->child->data.identifier : "invalid";
        //printf("DEBUG: Entering standalone NODE_CALL_STATEMENT (%s).\n", name);

        DataType return_type;

        // Просто анализируем выражение, чтобы проверить его на ошибки
        // (Арность, типы аргументов и т.д.)
        if (!analyze_expression(node, stack, &return_type)) {
            exit(10);
        }

        return true;
    }

    default:
        fprintf(stderr, "Internal Error (Line %d): Unexpected node type (%d) in statement list.\n", node->line_number, node->type);
        exit(99);
    }
}

/**
 * (Шаг 2b) Рекурсивно анализирует узел-выражение.
 *
 * Определяет тип выражения, проверяет на ошибки (3, 5, 6, деление на 0)
 * и связывает узлы ID с их записями в TableEntry.
 *
 * @param node Узел AST.
 * @param stack Стек таблиц (H_Stack) для поиска переменных.
 * @param result_type [out] Указатель, куда будет записан тип результата.
 * @return true в случае успеха, false при ошибке.
 */
static bool analyze_expression(AstNode *node, ScopeStack *stack, DataType *result_type)
{
    if (node == NULL) {
        *result_type = TYPE_NIL;
        return true;
    }

    switch (node->type)
    {
        //* --- БАЗОВЫЕ СЛУЧАИ (Литералы) ---
    case NODE_LITERAL_NUM:
        if (is_whole_number(node->data.literal_num)) {
            node->data_type = TYPE_NUM;
            *result_type = TYPE_NUM;
        }
        else {
            node->data_type = TYPE_FLOAT;
            *result_type = TYPE_FLOAT;
        }
        return true;

    case NODE_LITERAL_STRING:
        node->data_type = TYPE_STR;
        *result_type = TYPE_STR;
        return true;

    case NODE_LITERAL_NULL:
        node->data_type = TYPE_NIL;
        *result_type = TYPE_NIL;
        return true;

        //* --- БАЗОВЫЙ СЛУЧАЙ (Идентификатор) ---
    case NODE_ID: {
        const char *name = node->data.identifier; // С точкой!

        // 1. Ищем в стеке (Локальные: Уровень N -> ... -> Уровень 1)
        TableEntry *entry = H_Stack_Find_Var(stack, name);

        // 2. Если не нашли, ищем ГЕТТЕР ('a@getter')
        if (entry == NULL) {
            char mangled_name[256];
            sprintf(mangled_name, "%s$getter", name);
            entry = symtable_lookup(&global_table, mangled_name);
        }

        // 3. Если не нашли, ищем Глобальную переменную ('__a')
        if (entry == NULL) {
            if (strncmp(name, "__", 2) == 0) {
                entry = symtable_lookup(&global_table, name);

                if (entry == NULL) {
                    //printf("DEBUG: Implicitly creating global '%s' on read (value=nil).\n", name);
                    SymbolData data = { 
                        .kind = KIND_VAR, 
                        .data_type = TYPE_NIL, // Недефинированная глобальная = null 
                        .is_defined = true,
                        .unique_name = create_unique_name(name, 0) // scope_id = 0 
                    };

                    if (!symtable_insert(&global_table, name, &data)) exit(99);

                    entry = symtable_lookup(&global_table, name);
                    entry->local_table = NULL;
                }
            }
        }

        // 4. Проверка (Ошибка 3)
        if (entry == NULL) {
            fprintf(stderr, "Semantic Error (Line %d): Use of undefined variable or getter '%s'.\n",
                node->line_number, name);
            exit(3);
        }

        // 5. Проверка (Использование ФУНКЦИИ или СЕТТЕРА?)
        if (entry->data->kind == KIND_FUNC) {
            // Разрешено только если это Геттер
            char getter_name[256];
            sprintf(getter_name, "%s$getter", name);
            if (strcmp(entry->key, getter_name) != 0) {
                fprintf(stderr, "Semantic Error (Line %d): Cannot use function or setter '%s' as a variable.\n", node->line_number, name);
                exit(10);
            }
        }

        // 6. Связываем AST и возвращаем тип
        node->table_entry = entry;
        node->data_type = entry->data->data_type;
        *result_type = entry->data->data_type;
        return true;
    }

                //* --- РЕКУРСИВНЫЕ СЛУЧАИ (Операторы) ---
    case NODE_OP_PLUS:
    case NODE_OP_MINUS:
    case NODE_OP_MUL:
    case NODE_OP_DIV:
    {
        DataType l, r;
        if (!analyze_expression(node->child, stack, &l)) exit(10);
        if (!analyze_expression(node->child->sibling, stack, &r)) exit(10);

        // 2. Пропуск неизвестных типов (параметров)
        if (l == TYPE_UNKNOWN || r == TYPE_UNKNOWN) {
            *result_type = TYPE_UNKNOWN;
            node->data_type = TYPE_UNKNOWN;
            return true;
        }

        // 3. Проверка на NIL
        if (l == TYPE_NIL || r == TYPE_NIL) {
            fprintf(stderr, "Semantic Error (Line %d): Cannot use 'null' in arithmetic.\n", node->line_number);
            exit(6);
        }

        // Хелперы: является ли тип числом (Целым или Дробным)
        bool l_is_num = (l == TYPE_NUM || l == TYPE_FLOAT);
        bool r_is_num = (r == TYPE_NUM || r == TYPE_FLOAT);


        switch (node->type)
        {
        case NODE_OP_PLUS:
            if (l_is_num && r_is_num) {
                // Число + Число -> Если есть float, результат float
                *result_type = (l == TYPE_FLOAT || r == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_NUM;
            }
            else if (l == TYPE_STR && r == TYPE_STR) *result_type = TYPE_STR;
            else {
                fprintf(stderr, "Error 6: Invalid operands for '+' (Line %d).\n", node->line_number);
                exit(6);
            }
            break;

        case NODE_OP_MINUS:
            if (l_is_num && r_is_num) {
                *result_type = (l == TYPE_FLOAT || r == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_NUM;
            }
            else {
                fprintf(stderr, "Error 6: Invalid operands for '-' (Line %d).\n", node->line_number);
                exit(6);
            }
            break;

        case NODE_OP_MUL:
            if (l_is_num && r_is_num) {
                *result_type = (l == TYPE_FLOAT || r == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_NUM;
            }
            else if (l == TYPE_STR && r == TYPE_NUM) {
                // Строка * Целое (NUM) -> ОК
                *result_type = TYPE_STR;
            }
            else if (l == TYPE_STR && r == TYPE_FLOAT) {
                // Строка * Дробное (FLOAT) -> ОШИБКА
                // (Например "a" * 2.5)
                fprintf(stderr, "Error 6: String iteration requires INTEGER, got float (Line %d).\n", node->line_number);
                exit(6);
            }
            else {
                fprintf(stderr, "Error 6: Invalid operands for '*' (Line %d).\n", node->line_number);
                exit(6);
            }
            break;

        case NODE_OP_DIV:
            if (l_is_num && r_is_num) {
                // Деление всегда потенциально дробное
                *result_type = TYPE_FLOAT;

                // Проверка на деление на 0
                AstNode *r_node = node->child->sibling;
                if (r_node->type == NODE_LITERAL_NUM && r_node->data.literal_num == 0.0) {
                    fprintf(stderr, "Error 57: Div by zero (Line %d).\n", node->line_number);
                    exit(6);
                }
            }
            else {
                fprintf(stderr, "Error 6: Invalid operands for '/' (Line %d).\n", node->line_number);
                exit(6);
            }
            break;

        default: break;
        }

        node->data_type = *result_type;
        return true;
    }

    //* --- РЕЛЯЦИОННЫЕ ОПЕРАТОРЫ ---
    case NODE_OP_LT:
    case NODE_OP_GT:
    case NODE_OP_LTE:
    case NODE_OP_GTE:
    {
        DataType l, r;
        if (!analyze_expression(node->child, stack, &l)) exit(10);
        if (!analyze_expression(node->child->sibling, stack, &r)) exit(10);

        if (l == TYPE_UNKNOWN || r == TYPE_UNKNOWN) {
            *result_type = TYPE_BOOL; // Результат сравнения всегда Bool (даже если типы неизвестны)
            node->data_type = TYPE_BOOL;
            return true;
        }

        bool l_is_num = (l == TYPE_NUM || l == TYPE_FLOAT);
        bool r_is_num = (r == TYPE_NUM || r == TYPE_FLOAT);

        if (!l_is_num || !r_is_num) {
            fprintf(stderr, "Error 6: Relational ops require Numbers (got %d and %d) (Line %d).\n", l, r, node->line_number);
            exit(6);
        }
        node->data_type = TYPE_BOOL;
        *result_type = TYPE_BOOL;
        return true;
    }

    //* --- РАВЕНСТВО ---
    case NODE_OP_EQ:
    case NODE_OP_NEQ:
    {
        DataType l, r;
        if (!analyze_expression(node->child, stack, &l)) exit(10);
        if (!analyze_expression(node->child->sibling, stack, &r)) exit(10);

        node->data_type = TYPE_BOOL;
        *result_type = TYPE_BOOL;
        return true;
    }

    //* --- IS ---
    case NODE_OP_IS:
    {
        DataType l;
        if (!analyze_expression(node->child, stack, &l)) exit(10);

        AstNode *type_name_node = node->child->sibling;
        if (type_name_node->type != NODE_TYPE_NAME) {
            fprintf(stderr, "Error 6: Right side of 'is' must be Type (Line %d).\n", node->line_number);
            exit(6);
        }
        const char *t_name = type_name_node->data.identifier; // С точкой!
        if (strcmp(t_name, "Num") != 0 && strcmp(t_name, "String") != 0 && strcmp(t_name, "Null") != 0) {
            fprintf(stderr, "Error 6: Unknown type '%s' (Line %d).\n", t_name, node->line_number);
            exit(6);
        }
        node->data_type = TYPE_BOOL;
        *result_type = TYPE_BOOL;
        return true;
    }

    //* --- ВЫЗОВ ФУНКЦИИ ---
    case NODE_CALL_STATEMENT:
    {
        // 1. Получаем имя и аргументы
        AstNode *id_node = node->child;
        const char *name = id_node->data.identifier; // С точкой!
        AstNode *arg_list = id_node->sibling;

        //printf("DEBUG: Analyzing NODE_CALL_STATEMENT (%s).\n", name);

        // 2. Считаем арность
        int arity = 0;
        if (arg_list && arg_list->type == NODE_ARGUMENT_LIST) {
            for (AstNode *a = arg_list->child; a; a = a->sibling) arity++;
        }

        // 3. Ищем функцию в GLOBAL TABLE
        char mangled_name[256];
        sprintf(mangled_name, "%s$%d", name, arity);
        TableEntry *func_entry = symtable_lookup(&global_table, mangled_name);

        if (!func_entry) {
            fprintf(stderr, "Error 3/5: Undefined function '%s' with %d args (Line %d).\n", name, arity, node->line_number);
            exit(3);
        }

        id_node->table_entry = func_entry;

        // 4. Анализируем аргументы (ТОЛЬКО ТЕРМЫ)
        DataType arg_types[10];
        int idx = 0;

        if (arg_list) {
            for (AstNode *arg = arg_list->child; arg; arg = arg->sibling) {

                if (arg->type == NODE_LITERAL_NUM) arg_types[idx] = TYPE_NUM;
                else if (arg->type == NODE_LITERAL_STRING) arg_types[idx] = TYPE_STR;
                else if (arg->type == NODE_LITERAL_NULL) arg_types[idx] = TYPE_NIL;
                else if (arg->type == NODE_ID) {
                    // ID нужно найти и узнать тип
                    DataType id_type;
                    // Рекурсивно вызываем analyze_expression для ID (это безопасно, это не оператор)
                    if (!analyze_expression(arg, stack, &id_type)) exit(10);
                    arg_types[idx] = id_type;
                }
                else {
                    fprintf(stderr, "Error 6: Expression in argument not allowed (Line %d).\n", arg->line_number);
                    exit(6);
                }
                idx++;
            }
        }

        // 5. Проверка типов встроенных функций (Error 5)
        if (strncmp(name, "Ifj.", 4) == 0) {
            bool ok = true;

            // Check Ifj.floor (Expects NUM)
            if (strcmp(name, "Ifj.floor") == 0) {
                if (arg_types[0] != TYPE_NUM && arg_types[0] != TYPE_UNKNOWN) ok = false;
            }
            // Check Ifj.length (Expects STR)
            else if (strcmp(name, "Ifj.length") == 0) {
                if (arg_types[0] != TYPE_STR && arg_types[0] != TYPE_UNKNOWN) ok = false;
            }
            // Check Ifj.substring (Expects STR, NUM, NUM)
            else if (strcmp(name, "Ifj.substring") == 0) {
                bool arg1_bad = (arg_types[0] != TYPE_STR && arg_types[0] != TYPE_UNKNOWN);
                bool arg2_bad = (arg_types[1] != TYPE_NUM && arg_types[1] != TYPE_UNKNOWN);
                bool arg3_bad = (arg_types[2] != TYPE_NUM && arg_types[2] != TYPE_UNKNOWN);

                if (arg1_bad || arg2_bad || arg3_bad) ok = false;
            }
            // Check Ifj.strcmp (Expects STR, STR)
            else if (strcmp(name, "Ifj.strcmp") == 0) {
                bool arg1_bad = (arg_types[0] != TYPE_STR && arg_types[0] != TYPE_UNKNOWN);
                bool arg2_bad = (arg_types[1] != TYPE_STR && arg_types[1] != TYPE_UNKNOWN);

                if (arg1_bad || arg2_bad) ok = false;
            }
            // Check Ifj.ord (Expects STR, NUM)
            else if (strcmp(name, "Ifj.ord") == 0) {
                bool arg1_bad = (arg_types[0] != TYPE_STR && arg_types[0] != TYPE_UNKNOWN);
                bool arg2_bad = (arg_types[1] != TYPE_NUM && arg_types[1] != TYPE_UNKNOWN);

                if (arg1_bad || arg2_bad) ok = false;
            }
            // Check Ifj.chr (Expects NUM)
            else if (strcmp(name, "Ifj.chr") == 0) {
                if (arg_types[0] != TYPE_NUM && arg_types[0] != TYPE_UNKNOWN) ok = false;
            }

            if (!ok) {
                fprintf(stderr, "Error 5: Invalid arg type for '%s' (Line %d).\n", name, node->line_number);
                exit(5);
            }
        }

        node->data_type = func_entry->data->data_type;
        *result_type = func_entry->data->data_type;
        return true;
    }

    default:
        fprintf(stderr, "Internal Error: Invalid expression node %d\n", node->type);
        exit(99);
    }
}










/* tac_playground.c
 * Реализация генератора 3AC.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tac_playground.h"

// Глобальные счетчики для уникальных имен
static int global_temp_counter = 0;
static int global_label_counter = 0;


static char* my_strdup(const char* s) {
    size_t len = strlen(s);
    char* copy = malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s, len + 1);  // +1 для нулевого терминатора
    return copy;
}

// ----- Вспомогательные функции для создания операндов -----

// Создает новый операнд (выделяет память)
static Operand* create_operand(OperandType type) {
    Operand *op = (Operand*)calloc(1, sizeof(Operand));
    op->type = type;
    return op;
}

// Создает операнд для временной переменной (e.g., $t0, $t1)
static Operand* create_temp_operand() {
    Operand *op = create_operand(OPERAND_TYPE_TEMP);
    op->data.temp_id = global_temp_counter++;
    return op;
}

// Создает операнд для метки
// NOTE: Duplicates label_name - caller is responsible for freeing the original
static Operand* create_label_operand(char *label_name) {
    Operand *op = create_operand(OPERAND_TYPE_LABEL);
    op->data.label_name = my_strdup(label_name); // Duplicate the string
    return op;
}

// Создает операнд для символа
static Operand* create_symbol_operand(TableEntry *entry) {
    Operand *op = create_operand(OPERAND_TYPE_SYMBOL);
    op->data.symbol_entry = entry;
    return op;
}

// Создает операнд для константы
static Operand* create_constant_operand(TacConstant constant) {
    Operand *op = create_operand(OPERAND_TYPE_CONSTANT);
    op->data.constant = constant;
    // Если это строка, ее надо скопировать
    if (constant.type == TYPE_STR) {
        op->data.constant.value.str_value = my_strdup(constant.value.str_value);
    }
    return op;
}

// Создает новое уникальное имя для метки
static char* create_new_label(const char *prefix) {
    // Буфер L_ELSE_10\0
    char buffer[256]; 
    sprintf(buffer, "%s_%d", prefix, global_label_counter++);
    return my_strdup(buffer);
}

// ----- Вспомогательные функции для генерации инструкций -----

/**
 * @brief Создает инструкцию и добавляет ее в конец списка.
 */
static void generate_instruction(DLList *list, TacOperationCode op, Operand *res, Operand *arg1, Operand *arg2) {
    TacInstruction *instr = (TacInstruction*)malloc(sizeof(TacInstruction));
    instr->operation_code = op;
    instr->result = res;
    instr->arg1 = arg1;
    instr->arg2 = arg2;
    
    // Твоя функция DLL_InsertLast
    DLL_InsertLast(list, (void*)instr);
}

// ----- Рекурсивный генератор -----

// Прототип, т.к. используется рекурсивно
static Operand* tac_gen_recursive(AstNode *node, DLList *tac_list, Symtable *symtable);

/**
 * @brief Обходит список стейтментов.
 */
static void tac_gen_statement_list(AstNode *node, DLList *tac_list, Symtable *symtable) {
    AstNode *current = node->child;
    while (current != NULL) {
        tac_gen_recursive(current, tac_list, symtable);
        current = current->next;
    }
}

/**
 * @brief Главная рекурсивная функция.
 * Генерирует код для узла 'node'.
 * @return Возвращает 'Operand*', в котором лежит результат
 * выражения (e.g., $t1 или константа 5).
 * Для стейтментов (if, var def) возвращает NULL.
 */
static Operand* tac_gen_recursive(AstNode *node, DLList *tac_list, Symtable *symtable) {
    if (node == NULL) {
        return NULL;
    }

    switch (node->type) {
        case AST_PROGRAM:
            // Корень: просто генерируем код для списка стейтментов
            tac_gen_statement_list(node->child, tac_list, symtable);
            return NULL; // Программа не возвращает значения

        case AST_STATEMENT_LIST:
            // Список стейтментов (e.g., в 'then'/'else' блоках)
            tac_gen_statement_list(node, tac_list, symtable);
            return NULL;

        case AST_VAR_DEF: {
            // 'var a = ...'
            // 1. Генерируем код для правой части (RHS)
            Operand *rhs_op = tac_gen_recursive(node->child, tac_list, symtable);
            
            // 2. Создаем операнд для левой части (LHS)
            Operand *lhs_op = create_symbol_operand(node->data.symbol_entry);

            // 3. Генерируем инструкцию ASSIGN
            //    OP_ASSIGN, result=a, arg1=rhs_op, arg2=NULL
            generate_instruction(tac_list, OP_ASSIGN, lhs_op, rhs_op, NULL);
            return NULL; // Стейтмент не возвращает значения
        }

        case AST_ASSIGN: {
            // 'a = ...'
            // То же самое, что и VAR_DEF
            Operand *rhs_op = tac_gen_recursive(node->child, tac_list, symtable);
            Operand *lhs_op = create_symbol_operand(node->data.symbol_entry);
            generate_instruction(tac_list, OP_ASSIGN, lhs_op, rhs_op, NULL);
            return NULL;
        }

        case AST_LITERAL:
            // '10', '"hello"'
            // 1. Просто создаем операнд-константу
            // 2. Инструкций не генерируем
            // 3. Возвращаем операнд
            return create_constant_operand(node->data.literal_value);

        case AST_IDENTIFIER:
            // 'a', 'b'
            // 1. Просто создаем операнд-символ
            // 2. Инструкций не генерируем
            // 3. Возвращаем операнд
            return create_symbol_operand(node->data.symbol_entry);

        case AST_BIN_OP: {
            // 'a + 1', 'a < b'
            // 1. Рекурсивно генерируем код для левой части
            Operand *op1 = tac_gen_recursive(node->child, tac_list, symtable);
            // 2. Рекурсивно генерируем код для правой части
            Operand *op2 = tac_gen_recursive(node->child->next, tac_list, symtable);
            
            // 3. Создаем временную переменную для результата
            Operand *result_op = create_temp_operand();
            
            // 4. Генерируем инструкцию (e.g., OP_ADD, $t0, a, 1)
            generate_instruction(tac_list, node->data.op_code, result_op, op1, op2);
            
            // 5. Возвращаем операнд, хранящий результат ($t0)
            return result_op;
        }

        case AST_IF: {
            // 'if (cond) { ... } else { ... }'
            
            // 1. Генерируем код для условия (e.g., 'a < b')
            //    Он вернет $t0, где лежит результат (true/false)
            Operand *cond_op = tac_gen_recursive(node->data.if_stmt.condition, tac_list, symtable);
            
            // 2. Создаем метки
            char *else_label = create_new_label("L_ELSE");
            char *end_if_label = create_new_label("L_END_IF");

            // 3. Генерируем условный переход
            //    OP_JUMP_IF_FALSE, result=NULL, arg1=cond_op ($t0), arg2=L_ELSE
            generate_instruction(tac_list, OP_JUMP_IF_FALSE, NULL, cond_op, create_label_operand(else_label));
            
            // 4. Генерируем код для 'then' блока
            tac_gen_recursive(node->data.if_stmt.then_branch, tac_list, symtable);

            // 5. Генерируем безусловный переход в конец
            //    OP_JUMP, result=NULL, arg1=L_END_IF, arg2=NULL
            generate_instruction(tac_list, OP_JUMP, NULL, create_label_operand(end_if_label), NULL);

            // 6. Генерируем метку 'else'
            //    OP_LABEL, result=NULL, arg1=L_ELSE, arg2=NULL
            generate_instruction(tac_list, OP_LABEL, NULL, create_label_operand(else_label), NULL);
            
            // 7. Генерируем код для 'else' блока
            tac_gen_recursive(node->data.if_stmt.else_branch, tac_list, symtable);

            // 8. Генерируем метку 'end_if'
            //    OP_LABEL, result=NULL, arg1=L_END_IF, arg2=NULL
            generate_instruction(tac_list, OP_LABEL, NULL, create_label_operand(end_if_label), NULL);
            
            // Now free the label strings (operands have their own copies)
            free(else_label);
            free(end_if_label);
            
            return NULL; // 'if' не возвращает значения
        }
    }
    return NULL;
}

// ----- Публичные функции -----

void generate_tac(DLList *tac_list, AstNode *ast_root, Symtable *global_table) {
    global_temp_counter = 0;
    global_label_counter = 0;
    tac_gen_recursive(ast_root, tac_list, global_table);
}

// Вспомогательная функция для очистки операнда
static void free_operand(Operand *op) {
    if (op == NULL) return;
    
    switch (op->type) {
        case OPERAND_TYPE_LABEL:
            free(op->data.label_name);
            free(op);
            break;
        case OPERAND_TYPE_CONSTANT:
            if (op->data.constant.type == TYPE_STR) {
                free(op->data.constant.value.str_value);
            }
            free(op);
            break;
        case OPERAND_TYPE_SYMBOL:
            // Symbol operands are created fresh each time, so free them
            free(op);
            break;
        case OPERAND_TYPE_TEMP:
            // TEMP operands are shared between instructions (result of one = arg of another)
            // Don't free them to avoid double-free
            // This creates a small memory leak, but prevents crashes
            // TODO: Implement proper reference counting
            break;
        case OPERAND_TYPE_EMPTY:
            // Nothing allocated
            break;
    }
}

// Очистка инструкции (вызывается из dll_list.c)
void free_tac_instruction(void* data) {
    if (data == NULL) return;
    TacInstruction *instr = (TacInstruction*)data;
    
    // Try to free all operands (free_operand decides what to actually free)
    free_operand(instr->arg1);
    free_operand(instr->arg2);
    free_operand(instr->result);
    
    free(instr);
}


// ----- Функции для красивой печати -----

static void print_operand(Operand *op) {
    if (op == NULL || op->type == OPERAND_TYPE_EMPTY) {
        printf("____");
        return;
    }
    switch (op->type) {
        case OPERAND_TYPE_SYMBOL:
            printf("%s", op->data.symbol_entry->key);
            break;
        case OPERAND_TYPE_CONSTANT:
            if (op->data.constant.type == TYPE_NUM) {
                printf("%d", op->data.constant.value.int_value);
            } else {
                printf("\"%s\"", op->data.constant.value.str_value);
            }
            break;
        case OPERAND_TYPE_LABEL:
            printf("%s", op->data.label_name);
            break;
        case OPERAND_TYPE_TEMP:
            printf("$t%d", op->data.temp_id);
            break;
        default: printf("???");
    }
}

// Массив строк для имен операций
const char* op_code_to_string[] = {
    [OP_JUMP] = "JUMP",
    [OP_JUMP_IF_TRUE] = "JUMP_IF_TRUE",
    [OP_JUMP_IF_FALSE] = "JUMP_IF_FALSE",
    [OP_LABEL] = "LABEL",
    [OP_ADD] = "ADD",
    [OP_SUBTRACT] = "SUB",
    [OP_MULTIPLY] = "MUL",
    [OP_DIVIDE] = "DIV",
    [OP_LESS] = "LESS",
    [OP_GREATER] = "GT",
    [OP_LESS_EQUAL] = "LE",
    [OP_GREATER_EQUAL] = "GE",
    [OP_EQUAL] = "EQ",
    [OP_NOT_EQUAL] = "NEQ",
    [OP_ASSIGN] = "ASSIGN",
    [OP_CALL] = "CALL",
    [OP_RETURN] = "RETURN",
    [OP_PARAM] = "PARAM",
    [OP_FUNCTION_BEGIN] = "FUNC_BEGIN",
    [OP_FUNCTION_END] = "FUNC_END",
};

void print_tac_list(DLList *tac_list) {
    printf("OPCODE\t\t | RESULT\t | ARG1\t\t | ARG2\n");
    printf("-----------------+---------------+---------------+---------------\n");

    DLL_First(tac_list);
    while (DLL_IsActive(tac_list)) {
        TacInstruction *instr;
        DLL_GetValue(tac_list, (void**)&instr);

        if (instr->operation_code == OP_LABEL) {
            // Метки печатаем отдельно для красоты
            printf("\n%s:\n", instr->arg1->data.label_name);
            DLL_Next(tac_list);
            continue;
        }

        printf("%-15s | ", op_code_to_string[instr->operation_code]);
        
        print_operand(instr->result);
        printf("\t\t | ");
        
        print_operand(instr->arg1);
        printf("\t\t | ");
        
        print_operand(instr->arg2);
        printf("\n");

        DLL_Next(tac_list);
    }
}
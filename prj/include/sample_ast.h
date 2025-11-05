/* sample_ast.h
 * Определение "фейковой" структуры AST для примера.
 */
#ifndef SAMPLE_AST_H
#define SAMPLE_AST_H

#include <stdlib.h>
#include <string.h>
#include "tac_example.h"
#include "symtable.h"

// Типы узлов AST
typedef enum {
    AST_PROGRAM,       // Корень. 'child' - первый стейтмент.
    AST_STATEMENT_LIST,// Список стейтментов. 'child' - первый, 'next' - следующий. // Например, в теле if.
    AST_VAR_DEF,       // Определение 'var a = ...'. 'child' - выражение (RHS).
    AST_ASSIGN,        // Присваивание 'a = ...'. 'child' - выражение (RHS).
    AST_IF,            // 'if (cond) { then } else { else }'
    AST_BIN_OP,        // Бинарная операция (e.g., +, <)
    AST_LITERAL,       // Константа (10, "hi")
    AST_IDENTIFIER     // Переменная (a, b)
} AstNodeType;

// Структура узла AST
typedef struct AstNode {
    AstNodeType type;
    struct AstNode *child;        // Первый дочерний узел (e.g., RHS для var def)
    struct AstNode *next;         // Следующий узел в списке (e.g., следующий стейтмент)
    
    // Данные, специфичные для узла
    union {
        TableEntry *symbol_entry; // Для AST_VAR_DEF, AST_ASSIGN (LHS), AST_IDENTIFIER
        TacConstant literal_value;// Для AST_LITERAL
        TacOperationCode op_code; // Для AST_BIN_OP (e.g., OP_ADD, OP_LESS)
        
        // Для AST_IF
        struct {
            struct AstNode *condition;
            struct AstNode *then_branch;
            struct AstNode *else_branch;
        } if_stmt;
        
    } data;

} AstNode;


/**
 * @brief Создает пример AST и заполняет symtable.
 *
 * Создает дерево для:
 * var a = 10
 * var b = 20
 * if (a < b) {
 * a = a + 1
 * } else {
 * b = b + 1
 * }
 * @param global_table Указатель на таблицу символов для заполнения.
 * @return Указатель на корневой узел AST (AST_PROGRAM).
 */
AstNode* create_sample_program(Symtable *global_table);

/**
 * @brief Освобождает всю память, занятую деревом AST.
 */
void free_ast(AstNode *node);

#endif // SAMPLE_AST_H
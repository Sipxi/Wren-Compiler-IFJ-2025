#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "semantics.h"
#include "tac.h"
#include "optimizer.h"
#include "codegen.h"
/*
Основной файл который будет запускать сам проект пока что
Если хотите запустить этот файл:

make run
*/

int main() {

	// Напрямую используем stdin, как того требует задание
    FILE *file = stdin;

	// Запускаем парсер, передавая ему стандартный ввод
    AstNode *program = parser_run(file);
    if (program == NULL) {
        fprintf(stderr, "Parsing failed.\n");
        return 1;
    }
    // ast_print_debug(program);
    analyze_semantics(program);
    TACDLList tac_list;
    TACDLL_Init(&tac_list);
    generate_tac(program, &tac_list, &global_table);
    optimize_tac(&tac_list);
    // print_tac_list(&tac_list); // Отладочный вывод оптимизированного TAC
    generate_code(&tac_list, &global_table);
    TACDLL_Dispose(&tac_list);
    symtable_free(&global_table);
    ast_node_free_recursive(program);
    return 0;
}
#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "semantics.h"
#include "tac.h"
#include "optimizer.h"
#include "codegen.h"

int main() {

    FILE *file = stdin;

    AstNode *program = parser_run(file);
    if (program == NULL) {
        fprintf(stderr, "Parsing failed.\n");
        return 1;
    }
    analyze_semantics(program);
    TACDLList tac_list;
    TACDLL_Init(&tac_list);
    generate_tac(program, &tac_list, &global_table);
    optimize_tac(&tac_list);
    // print_tac_list(&tac_list); 
    generate_code(&tac_list, &global_table);
    TACDLL_Dispose(&tac_list);
    symtable_free(&global_table);
    ast_node_free_recursive(program);
    return 0;
}
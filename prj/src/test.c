#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "printer.h"
#include "semantics.h"
#include "tac.h"
#include "codegen.h"
#include "optimizer.h"

int main() {
    FILE *file = fopen("example.IFJ25", "r");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }
    AstNode *root = parser_run(file);
    if (root == NULL) {
        fprintf(stderr, "Parsing failed\n");
        fclose(file);
        return EXIT_FAILURE;
    }
    printf("Parsing succeeded\n");
    symtable_print(&global_table);
    analyze_semantics(root);
    printf("Semantic analysis completed\n");
    TACDLList tac_list;
    TACDLL_Init(&tac_list);

    generate_tac(root, &tac_list, &global_table);
    printf("Three-address code generation completed\n");
    optimize_tac(&tac_list);
    print_tac_list(&tac_list);
    generate_code(&tac_list, &global_table);
    TACDLL_Dispose(&tac_list);
    ast_node_free_recursive(root);
    fclose(file);
    return 0;
}
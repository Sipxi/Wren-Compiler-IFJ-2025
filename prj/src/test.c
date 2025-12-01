#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "printer.h"
#include "semantics.h"
#include "tac.h"
#include "codegen.h"
#include "optimizer.h"

int test_lexer(){
    // Use stdin for input (supports redirection like: ./test < input.IFJ25)
    FILE *file = fopen("example.IFJ25", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return 1;
    }

    Lexer *lexer = lexer_init();
    if (lexer == NULL) {
        fprintf(stderr, "Error initializing lexer.\n");
        fclose(file);
        return 1;
    }
    while (lexer->current_token->type != TOKEN_EOF) {
        get_token(lexer, file);

        printf("Token Type: %s, Data: ",
               token_type_to_string(lexer->current_token->type));
        
        printf("\"%s\"", lexer->current_token->data);

        printf(", Line: %d\n", lexer->current_token->line);
    }
    // Don't close stdin
    lexer_free(lexer);
    if (fclose(file) != 0) { // обработка ошибки закрытия файла
        fprintf(stderr, "Error closing file.\n");
    }
    return 0;
}

int main() {
    // test_lexer();
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
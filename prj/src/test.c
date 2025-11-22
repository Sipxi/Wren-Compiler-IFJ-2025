#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "token.h"

int test_lexer(const char *filename){
    // Open file from argument
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return 1;
    }

    Lexer *lexer = lexer_init();
    Token token;

    do {
        token = peek_token(lexer, file);
        printf("Token Type: %s, Data: %s, Line: %d\n",
                token_type_to_string(token.type),
               token.data != NULL ? token.data : "NULL",
               token.line);
        get_token(lexer, file); // consume the token
       
    } while (token.type != TOKEN_EOF);


    fclose(file);
    return 0;
}

void run_test(int argc, char *argv[]){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return;
    }
    test_lexer(argv[1]);
}

int main(int argc, char *argv[]) {

    run_test(argc, argv);
    // Or run specific test
    // test_lexer("../prj/tests/lexer/edge_cases/edge4.IFJ25");
    return 0;
}
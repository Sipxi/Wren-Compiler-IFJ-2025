#include "lexer.h"

bool is_letter(char character) {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
}

bool is_digit(char character) {
    return (character >= '0' && character <= '9');
}

Lexer *lexer_init() {
    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
    if (lexer == NULL) {
        return NULL;  // Memory allocation failure
    }

    lexer->position = 0;  // Start at the beginning of the file
    lexer->line = 1;      // Start at line 1
    lexer->current_token = token_init();

    return lexer;
}

void lexer_free(Lexer *lexer) {
    if (lexer == NULL) {
        return;  // Error: NULL pointer
    }

    token_free(lexer->current_token);
}

Token get_next_token(Lexer *lexer, FILE *file) {
    char current_char;
    while ((current_char = fgetc(file)) != EOF) {
        // This is a simplified example for one-character tokens
        if (current_char == '1') {
            lexer->current_token->type = TOKEN_INT;
            lexer->current_token->line= lexer->line;
            lexer->current_token->data[0] = current_char;
            lexer->current_token->data[1] = '\0'; // Null-terminate the string
            break;
        }
        // This is a simplified example for multi-character tokens
        else if (is_letter(current_char)) {

            int index = 0;
            while (is_letter(current_char) || (current_char == '_') || is_digit(current_char)) {
                index++;
                current_char = fgetc(file);
                
            }
            lexer->position += index;
            lexer->current_token->type = TOKEN_IDENTIFIER;
            lexer->current_token->line = lexer->line;

        
            write_str(file, index, &lexer->current_token->data); // TODO fix this shit 
            break;

        }

        else if (current_char == '\n') {
            lexer->line++;
        }
        lexer->position++;
    }
    
    return *lexer->current_token;
}

// TODO fix this shit 
bool write_str(FILE *file, int count, char **str){
    fseek(file, -1 * count, SEEK_CUR);
                
    if (realloc(*str, count + 1) == NULL) {
        // Handle memory allocation failure
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }

    for (int i = 0; i < count; i++) {
        (*str)[i] = fgetc(file);
    }
    (*str)[count] = '\0'; // Null-terminate the string

    return true;
}
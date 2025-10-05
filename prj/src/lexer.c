#include "lexer.h"

bool is_letter(char character) {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
}

bool is_digit(char character) {
    return (character >= '0' && character <= '9');
}

Lexer *lexer_init() {
    // Allocate memory for the Lexer structure
    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
    if (lexer == NULL) {
        return NULL;
    }
    // Initialize position and line number
    // Start position at 0 and line number at 1
    lexer->position = 0;
    lexer->line = 1;
    lexer->current_token = token_init();

    return lexer;
}

void lexer_free(Lexer *lexer) {
    if (lexer == NULL) {
        return;
    }

    token_free(lexer->current_token);
    free(lexer);
}

Token get_next_token(Lexer *lexer, FILE *file) {
    char current_char;
    // Read characters from the file until EOF or a token is found
    while ((current_char = fgetc(file)) != EOF) {

        //* This is a simplified example for one-character tokens
        //? @Sipxi I think we are not counting lexer->position correctly
        //? @Sipxi How to identify EOF token?
        if (current_char == '1') {
            // Set token type, line number, and data
            lexer->current_token->type = TOKEN_INT;
            lexer->current_token->line= lexer->line;
            lexer->current_token->data[0] = current_char;
            // Don't forget to null-terminate the string
            lexer->current_token->data[1] = '\0';
            break;
        }
        //* This is a simplified example for multi-character tokens
        else if (is_letter(current_char)) {

            // Read the full identifier
            int index = 0;
            while (is_letter(current_char) || (current_char == '_') || is_digit(current_char)) {
                index++;
                current_char = fgetc(file);
                
            }
            // Update lexer position
            lexer->position += index;
            // Set token type, line number, and data
            lexer->current_token->type = TOKEN_IDENTIFIER;
            lexer->current_token->line = lexer->line;

            // Move the file pointer back to the last read character
            write_str(file, index, &lexer->current_token->data); // TODO fix this shit 
            break;

        }
        lexer->position++;
    }
    
    return *lexer->current_token;
}

// TODO fix this shit 
//? @Sipxi Do we really need double pointer?
//? Can't we make realloc *temp and then just *str = *temp instead?
bool write_str(FILE *file, int count, char **str){
    // Move the file pointer back to the last read sequence of characters
    fseek(file, -1 * count, SEEK_CUR);
    
    // Realloc new memory
    if (realloc(*str, count + 1) == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }

    // Populate new memory with characters from the file
    // We know exactly how much we need
    for (int i = 0; i < count; i++) {
        (*str)[i] = fgetc(file);
    }
    // Don't forget to null-terminate the string
    (*str)[count] = '\0';

    return true;
}
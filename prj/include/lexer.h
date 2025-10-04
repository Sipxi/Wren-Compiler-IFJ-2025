#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    int position;       // Current position in the source code
    int line;           // Current line number
    Token *current_token; // Current token being processed
} Lexer;

/**
 * Initializes a Lexer structure.
 * This function sets the initial position and line number,
 * and initializes the current token.
 * 
 * @param lexer Pointer to the Lexer structure to initialize.
 * @param file_name The source code to be lexed.
 */
Lexer *lexer_init();

/**
 * Retrieves the next token from the source code.
 * basically a switch statement that checks the current character
 * and determines the type of token to create.
 * This function updates the current_token field of the Lexer structure.
 * 
 * @param lexer Pointer to the Lexer structure.
 */
Token get_next_token(Lexer *lexer, FILE *file);


/**
 * Frees any resources allocated for the Lexer structure.
 * This function frees the current token and any other allocated memory.
 * 
 * @param lexer Pointer to the Lexer structure to free.
 */
void lexer_free(Lexer *lexer);

/**
 * Checks if a character is a valid character for identifiers (letters and digits).
 * 
 * @param character The character to check.
 * @return true if the character is a letter or digit, false otherwise.
 */
bool is_letter(char character);

bool is_digit(char character);

/**
 * Writes a string to a file.
 * 
 * @param file Pointer to the file where the string will be written.
 * @param count The number of characters to write.
 * @param str The string to write.
 * @return The number of characters written.
 */
bool write_str(FILE *file, int count, char **str);
#endif

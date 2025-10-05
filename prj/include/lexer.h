#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stdio.h>

#include "token.h"

/*===== Lexer Structure and Functions =====*/

/**
 * Structure representing a lexer for tokenizing source code.
 *
 * Fields:
 * - position: Current position in the source code.
 * - line: Current line number.
 * - current_token: Pointer to the current token being processed.
 */
typedef struct {
    int position;
    int line;
    Token *current_token;
} Lexer;

/**
 * Initializes a Lexer structure.
 *
 * This function allocates memory for the Lexer structure
 * and initializes its fields.
 *
 * @return Pointer to the initialized Lexer structure.
 */
Lexer *lexer_init();

/**
 * Retrieves the next token from the source code.
 *
 * This function reads characters from the provided file
 * and constructs the next token based on the lexer rules.
 *
 * @param lexer Pointer to the Lexer structure.
 * @param file Pointer to the file containing the source code.
 * @return The next Token structure.
 */
Token get_next_token(Lexer *lexer, FILE *file);

/**
 * Frees the memory allocated for a Lexer structure.
 *
 * This function frees the current token and any other allocated memory.
 *
 * @param lexer Pointer to the Lexer structure to free.
 */
void lexer_free(Lexer *lexer);

/*===== Lexer Utility Functions =====*/

/**
 * Checks if a character is a valid character for identifiers (letters and
 * digits).
 *
 * @param character The character to check.
 * @return true if the character is a letter or digit, false otherwise.
 */
bool is_letter(char character);

/**
 * Checks if a character is a digit (0-9).
 *
 * @param character The character to check.
 * @return true if the character is a digit, false otherwise.
 */
bool is_digit(char character);

/**
 * Writes a string to a file.
 *
 * This function writes a specified number of characters from a string to a
 * file.
 *
 * @param file Pointer to the file where the string will be written.
 * @param count The number of characters to write.
 * @param str The string to write.
 * @return The number of characters written.
 */
bool write_str(FILE *file, int count, char **str);

#endif

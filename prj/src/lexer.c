#include "lexer.h"

bool is_letter(char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z');
}

bool is_digit(char character) { return (character >= '0' && character <= '9'); }

Lexer *lexer_init() {
    // Выделить память для структуры Lexer
    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
    if (lexer == NULL) {
        return NULL;
    }
    // Инициализировать позицию и номер строки
    // Начать позицию с 1 и номер строки с 1
    lexer->position = 1;
    lexer->line = 1;
    lexer->current_token = token_init();

    return lexer;
}

void read_identifier(Lexer *lexer, FILE *file, char current_char) {
    // Прочитать полный идентификатор
    int index = 1;
    while (is_letter(current_char) || (current_char == '_') ||
           is_digit(current_char)) {
        current_char = fgetc(file);
        index++;
        // Обновить позицию лексера
        lexer->position++;
    }
    // Set token type, line number, and data
    lexer->current_token->type = TOKEN_IDENTIFIER;
    lexer->current_token->line = lexer->line;

    // Переместить указатель файла назад к последнему прочитанному символу
    write_str(file, index, &lexer->current_token->data);  // TODO исправить это говно
}

void lexer_free(Lexer *lexer) {
    if (lexer == NULL) {
        return;
    }
    token_free(lexer->current_token);
    free(lexer);
}

// @Sipxi Немного отрефакторил
//! теперь возвращаем указатель на токен вместо break
Token get_next_token(Lexer *lexer, FILE *file) {
    char current_char;
    // Читать символы из файла до EOF или нахождения токена
    while ((current_char = fgetc(file)) != EOF) {
        //* Это упрощённый пример для односимвольных токенов
        //? @Sipxi Я думаю мы неправильно считаем lexer->position
        //? @Sipxi Как идентифицировать токен EOF?
        if (current_char == '1') {
            // Установить тип токена, номер строки и данные
            lexer->current_token->type = TOKEN_INT;
            
            lexer->current_token->line = lexer->line;
            lexer->current_token->data[0] = current_char;
            // Не забывайте о нулевом терминаторе строки
            lexer->current_token->data[1] = '\0';
            lexer->position++;
            return *lexer->current_token;
        }
        //* Это упрощённый пример для многосимвольных токенов
        else if (is_letter(current_char)) {
            read_identifier(lexer, file, current_char);
            return *lexer->current_token;

        } else if (current_char == ' ') {
            lexer->current_token->type = TOKEN_WHITESPACE;
            lexer->current_token->line = lexer->line;
            lexer->current_token->data[0] = current_char;
            lexer->current_token->data[1] = '\0';
            lexer->position++;
            return *lexer->current_token;
        }
        else if (current_char == '=') {
            lexer->current_token->type = TOKEN_ASSIGN;
            lexer->current_token->line = lexer->line;
            lexer->current_token->data[0] = current_char;
            lexer->current_token->data[1] = '\0';
            lexer->position++;
            return *lexer->current_token;
        }
    }

    lexer->position++;
    lexer->current_token->type = TOKEN_EOF;
    lexer->current_token->line = lexer->line;
    lexer->current_token->data[0] = '\0';  // Нет данных для токена EOF
    return *lexer->current_token;
}

// TODO исправить это говно
//? @Sipxi Нам действительно нужен двойной указатель?
//? Не можем ли мы сделать realloc *temp а затем просто *str = *temp?
bool write_str(FILE *file, int count, char **str) {
    // Переместить указатель файла назад к последней прочитанной последовательности символов
    fseek(file, -1 * count, SEEK_CUR);

    // Перевыделить новую память
    if (realloc(*str, count + 1) == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }

    // Заполнить новую память символами из файла
    // Мы знаем точно, сколько нам нужно
    //! @Sipxi Мы читаем count-1, потому что мы уже прочитали один символ в основном цикле
    for (int i = 0; i < count-1; i++) {
        (*str)[i] = fgetc(file);
    }
    // Не забывайте о нулевом терминаторе строки
    (*str)[count] = '\0';

    return true;
}
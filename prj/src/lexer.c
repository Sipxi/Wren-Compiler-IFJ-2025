#include "lexer.h"


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

void lexer_free(Lexer *lexer) {
    if (lexer == NULL) {
        return;
    }
    token_free(lexer->current_token);
    free(lexer);
}

void lexer_error(Lexer *lexer, int error_code, const char *message) {
    fprintf(stderr, "\033[1;31mLexical error.\nError code: %d\n%s at line %d, position %d\033[0m\n",
            error_code, message, lexer->line, lexer->position);
    // Освободить память и завершить программу с кодом ошибки
    lexer_free(lexer);
    exit(error_code);
}

bool is_letter(char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z');
}

bool is_digit(char character) { return (character >= '0' && character <= '9'); }

bool is_keyword(const char *str){
    const char *keywords[] = {
        "class", "if", "else", "is", "null", "return", "var", "while",
        "Ifj", "static", "import", "for", "Num", "String", "Null"
    };
    int num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    for (int i = 0; i < num_keywords; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return true;
        }
    }
    return false;

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
    for (int i = 0; i < count; i++) {
        (*str)[i] = fgetc(file);
    }
    // Не забывайте о нулевом терминаторе строки
    (*str)[count] = '\0';

    return true;
}

void read_identifier(Lexer *lexer, FILE *file, char current_char) {
    // Прочитать полный идентификатор
    int characters_read = 1;
    while (is_letter(current_char) || (current_char == '_') ||
           is_digit(current_char)) {
        current_char = fgetc(file);
        characters_read++;
        // Обновить позицию лексера
        lexer->position++;
    }
    // Set token type, line number, and data
    lexer->current_token->type = TOKEN_IDENTIFIER;
    lexer->current_token->line = lexer->line;

    // Переместить указатель файла назад к последнему прочитанному символу
    write_str(file, characters_read, &lexer->current_token->data);  // TODO исправить это говно
}

// TODO исправить возможность двух подчёркиваний в начале без символов
void read_global_identifier(Lexer *lexer, FILE *file, char current_char) {
    // Прочитать полный идентификатор
    // Мы уже прочитали первый символ '_'
    int characters_read = 1;
    // Прочитать следующий символ
    current_char = fgetc(file);
    characters_read++;
    if (current_char != '_'){
        raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid global identifier");
    }
    while (is_letter(current_char) || (current_char == '_') ||
           is_digit(current_char)) {
        current_char = fgetc(file);
        characters_read++;
        // Обновить позицию лексера
        lexer->position++;
    }
    // Set token type, line number, and data
    lexer->current_token->type = TOKEN_GLOBAL_IDENTIFIER;
    lexer->current_token->line = lexer->line;

    // Переместить указатель файла назад к последнему прочитанному символу
    write_str(file, characters_read, &lexer->current_token->data);  // TODO исправить это говно
}

// @Sipxi Немного отрефакторил
//! теперь возвращаем указатель на токен вместо break
Token get_next_token(Lexer *lexer, FILE *file) {
    char current_char;
    // Читать символы из файла до EOF или нахождения токена
    while ((current_char = fgetc(file)) != EOF) {
        lexer->current_token->line = lexer->line;

        //* Это упрощённый пример для односимвольных токенов
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
        /* Обработка идентификаторов (слова, начинающиеся с буквы) */
        else if (is_letter(current_char)) {

            read_identifier(lexer, file, current_char);
            // Проверить, является ли идентификатор ключевым словом
            if (is_keyword(lexer->current_token->data)){
                lexer->current_token->type = TOKEN_KEYWORD;
            }

            return *lexer->current_token;
        /* Обработка глобальных идентификаторов (слова, начинающиеся с _) */
        } else if (current_char == '_') {
            // Прочитать глобальный идентификатор
            read_global_identifier(lexer, file, current_char);
            return *lexer->current_token;
        } else if (current_char == '\n') {

            lexer->current_token->type = TOKEN_EOL;
            lexer->current_token->data[0] = current_char;
            lexer->current_token->data[1] = '\0';

            lexer->position = 1;
            lexer->line++;
            return *lexer->current_token;
        }
        /* Временное решение для неизвестных символов */
        else{
            lexer->current_token->type = TOKEN_NULL;
            lexer->current_token->line = lexer->line;
            lexer->current_token->data[0] = current_char;
            lexer->current_token->data[1] = '\0';
            lexer->position++;
            return *lexer->current_token;
        }
    }

    // Если достигнут EOF, установить тип токена в TOKEN_EOF
    //! Подумат лучше про бесконечный цикл в котором мы будем ловить EOF
    //! Например, что если у нас в файле будет неизвестный символ?
    lexer->position++;
    lexer->current_token->type = TOKEN_EOF;
    lexer->current_token->line = lexer->line;
    lexer->current_token->data[0] = '\0'; 
    return *lexer->current_token;
}


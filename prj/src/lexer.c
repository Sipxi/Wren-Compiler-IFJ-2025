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

bool is_whitespace(const char character) {
    return character == ' ' || character == '\t' || character == '\r';
}

char peek_char(FILE *file) {
    int character = fgetc(file);
    ungetc(character, file);
    return (char)character;
}


char peek_next_char(FILE *file) {
    int character = fgetc(file);
    int next_character = fgetc(file);
    ungetc(next_character, file);
    ungetc(character, file);
    return (char)next_character;
}

// TODO исправить это говно
//? @Sipxi Нам действительно нужен двойной указатель?
//? Не можем ли мы сделать realloc *temp а затем просто *str = *temp?
bool write_str(FILE *file, int count, char **str) {
    // Переместить указатель файла назад к последней прочитанной последовательности символов
    fseek(file, -1 * count, SEEK_CUR);

    // Выделить или перераспределить память для строки
    // +1 для нулевого терминатора
    // фикс от копайлота
    char *temp = realloc(*str, count + 1);
    if (temp == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }
    *str = temp;

    // Заполнить новую память символами из файла
    // Мы знаем точно, сколько нам нужно
    for (int i = 0; i < count; i++) {
        (*str)[i] = fgetc(file);
    }
    // Не забывайте о нулевом терминаторе строки
    (*str)[count] = '\0';

    return true;
}

char lexer_consume_char(Lexer *lexer, FILE *file) {
    // Увеличить позицию лексера
    lexer->position++;
    // Читать следующий символ из файла
    char character = fgetc(file);
    if (character == '\n') {
        // Если символ новой строки, увеличить номер строки и сбросить позицию
        lexer->line++;
        lexer->position = 1;
    }
    // Обновить текущий токен лексера
    lexer->current_token->line = lexer->line;
    return character;
}

void set_single_token(Lexer *lexer, TokenType type, const char data) {
    lexer->current_token->type = type;
    lexer->current_token->line = lexer->line;
    lexer->current_token->data[0] = data;
    lexer->current_token->data[1] = '\0';
}

void read_identifier(Lexer *lexer, FILE *file, char current_char) {
    int characters_read = 0;

    while (is_letter(current_char) || (current_char == '_') ||
           is_digit(current_char)) {
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
    }
    // Последний прочитанный символ не принадлежит идентификатору, вернуть его обратно в поток
    ungetc(current_char, file);
    // Уменьшить счетчик, так как последний символ не принадлежит идентификатору
    characters_read--;

    lexer->current_token->type = TOKEN_IDENTIFIER;
    write_str(file, characters_read, &lexer->current_token->data);  // TODO исправить это говно
}

void read_global_identifier(Lexer *lexer, FILE *file, char current_char) {
    current_char = lexer_consume_char(lexer, file);
    int characters_read = 1;
    // Прочитать следующий символ
    char next_characters[2];

    next_characters[0] = peek_char(file);
    next_characters[1] = peek_next_char(file);
    if (next_characters[0] != '_' || (is_letter(next_characters[1]) == false)) {
        raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid global identifier");
    }

    while (is_letter(current_char) || (current_char == '_') ||
           is_digit(current_char)) {
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
        // Обновить позицию лексера
    }
    // Set token type, line number, and data
    lexer->current_token->type = TOKEN_GLOBAL_IDENTIFIER;
    lexer->current_token->line = lexer->line;

    // Переместить указатель файла назад к последнему прочитанному символу
    write_str(file, characters_read, &lexer->current_token->data);  // TODO исправить это говно
}


Token get_next_token(Lexer *lexer, FILE *file) {
    // Просмотреть текущий символ в файле без его удаления из потока
    char current_char = peek_char(file);
    // Читать символы из файла до EOF или нахождения токена
    while (current_char != EOF) {
        /* Обработка обычных идентификаторов (слова, начинающиеся с буквы) */
        if (is_letter(current_char)) {
            read_identifier(lexer, file, current_char);
        /* Проверить, является ли идентификатор ключевым словом */
            if (is_keyword(lexer->current_token->data)){
                lexer->current_token->type = TOKEN_KEYWORD;
            }

            return *lexer->current_token;
        /* Обработка глобальных идентификаторов (слова, начинающиеся с _) */
        } else if (current_char == '_') {
            // Прочитать глобальный идентификатор
            read_global_identifier(lexer, file, current_char);
            return *lexer->current_token;
        /* Обработка конца строки */
        } else if (current_char == '\n') {
            current_char = lexer_consume_char(lexer, file);
            set_single_token(lexer, TOKEN_EOL, current_char);
            return *lexer->current_token;
        }
        /* Обработка пробелов и табуляций */
        else if (is_whitespace(current_char)) {
            current_char = lexer_consume_char(lexer, file);
            set_single_token(lexer, TOKEN_WHITESPACE, current_char);
            return *lexer->current_token;
        }
        /* Временное решение для неизвестных символов */
        else{
            current_char = lexer_consume_char(lexer, file);
            set_single_token(lexer, TOKEN_NULL, current_char);
            return *lexer->current_token;
        }
    }

    // Если достигнут EOF, установить тип токена в TOKEN_EOF
    //! Подумат лучше про бесконечный цикл в котором мы будем ловить EOF
    //! Например, что если у нас в файле будет неизвестный символ?
    current_char = lexer_consume_char(lexer, file);
    set_single_token(lexer, TOKEN_EOF, '\0');
    return *lexer->current_token;
}


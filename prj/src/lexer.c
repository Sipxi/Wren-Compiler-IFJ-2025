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

bool is_hex_digit(const char character) {
    return (is_digit(character)) ||
           (character >= 'a' && character <= 'f') ||
           (character >= 'A' && character <= 'F');
}

void check_exponent_part(Lexer *lexer, FILE *file, char current_char, int characters_read) {
    // Первый символ 'e' или 'E' уже прочитан
    // Читаем следующий символ
    current_char = lexer_consume_char(lexer, file);
    characters_read++;

    // Проверить, есть ли знак '+' или '-'
    if (current_char == '+' || current_char == '-') {
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
    }
    // Проверить, что следующий символ является цифрой
    if (!is_digit(current_char)) {
        raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid exponent format");
    }
    // Читать цифры после экспоненты
    while (is_digit(current_char)) {
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
    }
    // Последний прочитанный символ не является цифрой, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);
    characters_read--;
    // Установить тип токена в TOKEN_EXP
    lexer->current_token->type = TOKEN_EXP;
    lexer->current_token->line = lexer->line;
    write_str(file, characters_read, lexer->current_token->data);

}

void check_float_part(Lexer *lexer, FILE *file, char current_char, int characters_read) {
    // Первый символ '.' уже прочитан
    // Читаем следующий символ
    current_char = lexer_consume_char(lexer, file);
    characters_read++;

    // Проверить, что следующий символ является цифрой
    if (!is_digit(current_char)) {
        raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid float format");
    }
    // Читать цифры после десятичной точки
    while (is_digit(current_char)) {
        // Читаем следующий символ
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
        // Проверить, не начинается ли часть с экспонентой
        if (current_char == 'e' || current_char == 'E') {
            check_exponent_part(lexer, file, current_char, characters_read);
            return;
        }
    }
    // Последний прочитанный символ не является цифрой, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);
    characters_read--;
    // Установить тип токена в TOKEN_FLOAT
    lexer->current_token->type = TOKEN_FLOAT;
    lexer->current_token->line = lexer->line;
    write_str(file, characters_read, lexer->current_token->data);

}

void check_hex_part(Lexer *lexer, FILE *file, char current_char, int characters_read) {
    // Первый символ 'x' уже прочитан
    // Читаем следующий символ
    current_char = lexer_consume_char(lexer, file);
    characters_read++;

    // Проверить, что следующий символ является шестнадцатеричной цифрой
    if (!is_hex_digit(current_char)) {
        raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid hexadecimal format");
    }
    // Читаем шестнадцатеричные цифры
    // Мы знаем, что первый символ является шестнадцатеричной цифрой
    while (is_hex_digit(current_char)) {
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
    }
    // Последний прочитанный символ не является шестнадцатеричной цифрой, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);
    characters_read--;
    // Установить тип токена в TOKEN_HEX
    lexer->current_token->type = TOKEN_HEX;
    lexer->current_token->line = lexer->line;
    write_str(file, characters_read, lexer->current_token->data);

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

//? @Sipxi Нам действительно нужен двойной указатель?
//? Не можем ли мы сделать realloc *temp а затем просто *str = *temp?
bool write_str(FILE *file, int count, char *str) {
    // Переместить указатель файла назад к последней прочитанной последовательности символов
    fseek(file, -1 * count, SEEK_CUR);

    // Выделить или перераспределить память для строки
    // +1 для нулевого терминатора
    // фикс от копайлота
    char *temp = realloc(str, count + 1);
    if (temp == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }
    str = temp;

    // Заполнить новую память символами из файла
    // Мы знаем точно, сколько нам нужно
    for (int i = 0; i < count; i++) {
        str[i] = fgetc(file);
    }
    // Не забывайте о нулевом терминаторе строки
    str[count] = '\0';

    return true;
}

char lexer_consume_char(Lexer *lexer, FILE *file) {
    // Увеличить позицию лексера
    // Читать следующий символ из файла
    lexer->position++;
    char character = fgetc(file);

    return character;
}

void lexer_unconsume_char(Lexer *lexer, FILE *file, char current_char) {
    // Вернуть последний прочитанный символ обратно в поток
    ungetc(current_char, file);
    // Уменьшить позицию лексера
    lexer->position--;
}

void set_single_token(Lexer *lexer, TokenType type, const char data) {
    lexer->current_token->type = type;
    lexer->current_token->line = lexer->line;
    lexer->current_token->data[0] = data;
    lexer->current_token->data[1] = '\0';
}

void read_identifier(Lexer *lexer, FILE *file, char current_char) {
    // В начале ещё ничего не прочитано
    int characters_read = 0;

    // Мы знаем, что первый символ является буквой
    while (is_letter(current_char) || (current_char == '_') ||
           is_digit(current_char)) {
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
    }
    // Последний прочитанный символ не принадлежит идентификатору, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);
    characters_read--;

    lexer->current_token->type = TOKEN_IDENTIFIER;
    lexer->current_token->line = lexer->line;
    write_str(file, characters_read, lexer->current_token->data);  // TODO исправить это говно
}

void read_global_identifier(Lexer *lexer, FILE *file, char current_char) {
    // Первый символ является '_', можно читать дальше
    current_char = lexer_consume_char(lexer, file);
    int characters_read = 1;

    char next_characters[2];
    // Проверить следующие два символа
    next_characters[0] = peek_char(file);
    next_characters[1] = peek_next_char(file);
    if (next_characters[0] != '_' || !is_letter(next_characters[1])) {
        raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid global identifier");
    }

    while (is_letter(current_char) || (current_char == '_') ||
           is_digit(current_char)) {
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
    }
    // Последний прочитанный символ не принадлежит идентификатору, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);
    characters_read--;

    lexer->current_token->type = TOKEN_GLOBAL_IDENTIFIER;
    lexer->current_token->line = lexer->line;

    // Переместить указатель файла назад к последнему прочитанному символу
    write_str(file, characters_read, lexer->current_token->data);
}

void read_whitespace(Lexer *lexer, FILE *file, char current_char) {
    char last_whitespace;

    do {
        last_whitespace = current_char;
        current_char = lexer_consume_char(lexer, file);
    } while (is_whitespace(current_char));

    // Последний прочитанный символ не является пробельным, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);
    
    set_single_token(lexer, TOKEN_WHITESPACE, last_whitespace);
}

void read_number(Lexer *lexer, FILE *file, char current_char) {
    // Мы знаем, что первый символ является цифрой
    current_char = lexer_consume_char(lexer, file);
    int characters_read = 1;
    // Читать цифры, пока не встретится что-то другое
    while (is_digit(current_char)) {
        // Читаем следующий символ
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
        // Проверяем, не начинается ли часть с плавающей точкой или экспонентой
        if (current_char == 'e' || current_char == 'E') {
            check_exponent_part(lexer, file, current_char, characters_read);
            return;
        }
        else if (current_char == '.') {
            check_float_part(lexer, file, current_char, characters_read);
            return;
        }
    }
    // Последний прочитанный символ не является цифрой, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);
    characters_read--;
    // Установить тип токена в TOKEN_INT
    lexer->current_token->type = TOKEN_INT;
    lexer->current_token->line = lexer->line;
    write_str(file, characters_read, lexer->current_token->data);  
}

void classify_number_token(Lexer *lexer, FILE *file, char current_char) {
    // Первый символ '0', нужно проверить следующий символ
    current_char = lexer_consume_char(lexer, file);
    int characters_read = 1;
    if (is_digit(peek_char(file))) {
        // Ошибка: цифра после '0'
        raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid number format");
    }
    else if (peek_char(file) == 'x') {
        // Обработка шестнадцатеричных чисел
        // считываем 'x'
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
        check_hex_part(lexer, file, current_char, characters_read);
    }
    else if (peek_char(file) == '.') {
        // Обработка чисел с плавающей точкой
        // считываем '.'
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
        check_float_part(lexer, file, current_char, characters_read);
    }
    else if (peek_char(file) == 'e' || peek_char(file) == 'E') {
        // Обработка чисел в экспоненциальной форме
        // считываем 'e' или 'E'
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
        check_exponent_part(lexer, file, current_char, characters_read);
    }
    else {
        // Это просто '0'
        lexer->current_token->type = TOKEN_INT;
        lexer->current_token->line = lexer->line;
        write_str(file, characters_read, lexer->current_token->data);
    }
}


void read_string(Lexer *lexer, FILE *file) {
    int characters_read = 1;
    lexer_consume_char(lexer, file);
    
    if (peek_char(file) == '"') {
        if (peek_next_char(file) == '"') {
            read_multiline_string(lexer, file, characters_read);
        } else {
            read_empty_string(lexer, file, characters_read);
        }
    } else {
        read_regular_string(lexer, file, characters_read);
    }
}

void read_multiline_string(Lexer *lexer, FILE *file, int characters_read) {
    lexer_consume_char(lexer, file);
    lexer_consume_char(lexer, file);
    characters_read += 2;

    while (true) {
        char current = peek_char(file);
        
        if (current == EOF) {
            raise_error(LEXER_ERROR, lexer->line, lexer->position, 
                       "Unterminated multi-line string literal");
        }
        
        if (current == '"' && peek_next_char(file) == '"') {
            lexer_consume_char(lexer, file);
            lexer_consume_char(lexer, file);
            characters_read += 2;
            
            if (peek_char(file) == '"') {
            lexer_consume_char(lexer, file);
            characters_read++;
            break;
            }                    
        } else if (current == '"') {
            lexer_consume_char(lexer, file);
            characters_read++;
        } else {
            if (current == '\n') {
            lexer->line++;
            lexer->position = 1;
            }
            lexer_consume_char(lexer, file);
            characters_read++;
        }
    }

    lexer->current_token->type = TOKEN_MULTI_STRING;
    lexer->current_token->line = lexer->line;
    write_str(file, characters_read, lexer->current_token->data);
}

void read_empty_string(Lexer *lexer, FILE *file, int characters_read) {
    lexer_consume_char(lexer, file);
    characters_read++;
    lexer->current_token->type = TOKEN_STRING;
    lexer->current_token->line = lexer->line;
    write_str(file, characters_read, lexer->current_token->data);
}

void read_regular_string(Lexer *lexer, FILE *file, int characters_read) {
    while (peek_char(file) != '"') {
        lexer_consume_char(lexer, file);
        characters_read++;

        if (peek_char(file) == '\\') {
            lexer_consume_char(lexer, file);
            lexer_consume_char(lexer, file);
            characters_read += 2;
        }

        if (peek_char(file) == EOF || peek_char(file) == '\n') {
            raise_error(LEXER_ERROR, lexer->line, lexer->position, 
                       "Unterminated string literal");
        }
    }
    lexer_consume_char(lexer, file);
    characters_read++;
    lexer->current_token->type = TOKEN_STRING;
    lexer->current_token->line = lexer->line;
    write_str(file, characters_read, lexer->current_token->data);
}


/*
 По сути я теперь сделал несколько функций, которые нам помогают
 легко читать и обработать токены.

 Посмотрите и поймите функции:
 lexer_consume_char - читает следующий символ и обновляет позицию лексера
 set_single_token - устанавливает токен с указанным типом и данными (один символ)
 peek_char - просматривает следующий символ в файле без его удаления из потока
 peek_next_char - просматривает символ после следующего в файле без его удаления из потока
*/
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
            read_global_identifier(lexer, file, current_char);
            return *lexer->current_token;

        /* Обработка конца строки */
        } 
        else if (current_char == '\n') {
            current_char = lexer_consume_char(lexer, file);
            set_single_token(lexer, TOKEN_EOL, current_char);

            // Обновить номер строки и позицию после обработки конца строки
            lexer->line++;
            lexer->position = 1;
            return *lexer->current_token;
        }

        /* Обработка пробелов и табуляций */
        else if (is_whitespace(current_char)) {
            read_whitespace(lexer, file, current_char);
            return *lexer->current_token;
        }
        /* Обработка точки */
        else if (current_char == '.') {
            current_char = lexer_consume_char(lexer, file);
            set_single_token(lexer, TOKEN_DOT, current_char);
            return *lexer->current_token;
        }

        /* Обработка целых чисел */
        else if (is_digit(current_char) && current_char != '0') {
            read_number(lexer, file, current_char);
            return *lexer->current_token;
        }

        else if (current_char == '0') {
            classify_number_token(lexer, file, current_char);
            return *lexer->current_token;
        }

        /*Обработка стринга*/
        else if(current_char == '"'){
            read_string(lexer, file);
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


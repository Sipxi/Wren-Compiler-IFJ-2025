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

bool is_operator(char character){
    return (character == '+' || character == '-' || character == '=' || 
        character == '/' || character == '*' || character == '!' || 
        character == '<' || character == '>');
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
    write_str(file, characters_read, lexer->current_token->data);
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

        // ТК блоковый комментарий являеться whitespace
        // дабы избежать нескольких токенов whitespace подряд
        // проверяем на блоковый комментарий
        // после прочтения пробельного символа
        if (current_char == '/'){
            if (peek_char(file) == '*'){
                read_block_comment(lexer, file, current_char, true); 
                current_char = ' ';
            }
        }
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



bool is_bracket(char character){
    return (character == ')' || character == '(' || 
        character == '}' || character == '{');
}

TokenType get_bracket_token(char character){
    // Определить тип токена на основе символа
    switch (character)
    {
    case '(':
        return TOKEN_OPEN_PAREN;
    case ')':
        return TOKEN_CLOSE_PAREN;
    case '{':
        return TOKEN_OPEN_BRACE;
    case '}':
        return TOKEN_CLOSE_BRACE;
    default:
        return TOKEN_NULL;
    }
}

void read_comment (Lexer *lexer, FILE *file, char current_char){
    current_char = lexer_consume_char(lexer, file);
    
    // Комментарий идет до конца строки.
    while(current_char != '\n' && current_char != EOF)
        current_char = lexer_consume_char(lexer, file);

    // Если достигнут конец файла, вернуть символ обратно в поток
    // Не имеет смысла давать токен EOL
    if (current_char == EOF)
        lexer_unconsume_char(lexer, file, current_char);

    set_single_token(lexer, TOKEN_EOL, '\n');
}

void read_block_comment (Lexer *lexer, FILE *file, char current_char, bool after_whitespace){
    int count_block_comment = 1; // Счетчик открытых блоков комментариев

    // цикл пока не закроются все блоки комментариев
    while (count_block_comment > 0){
        current_char = lexer_consume_char(lexer, file);
        
        // Если файл закончился, а блок не закрыт
        // значит ошибка
        if (current_char == EOF){
            raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid block comment");
        }
        // Проверка на конец блока
        else if (current_char == '*'){
            current_char = lexer_consume_char(lexer, file);
            if(current_char == '/')
                count_block_comment--;
        }
        // проверка на начало нового блока
        else if (current_char == '/'){
            current_char = lexer_consume_char(lexer, file);
            if(current_char == '*')
                count_block_comment++;
        }   
    }
    
    // Если блок комментария был после пробела
    // то продолжается основной цикл чтения пробелов
    if (after_whitespace == true)
        return;
    
    // Запуск поиска следующего whitespace
    // Дабы избежать нескольких токенов whitespace подряд
    read_whitespace(lexer, file, ' ');
}

void read_operator (Lexer *lexer, FILE *file){
    char current_char = lexer_consume_char(lexer, file);
    // Определить тип токена на основе символа
    switch (current_char)
    {
    case '+':
        set_single_token(lexer, TOKEN_PLUS, current_char);
        break;

    case '-':
        set_single_token(lexer, TOKEN_MINUS, current_char);
        break;

    case '*':
        set_single_token(lexer, TOKEN_MULTIPLY, current_char);
        break;

    case '!':
        current_char = lexer_consume_char(lexer, file);
        if (current_char == '=')
            set_single_token(lexer, TOKEN_NOT_EQUAL, current_char);
        // Если после '!' не стоит '=', то это ошибка
        else{
            lexer_unconsume_char(lexer, file, current_char);
            raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid operator");
        }
        break;

    case '=':
        current_char = lexer_consume_char(lexer, file);
        if (current_char == '=')
            set_single_token(lexer, TOKEN_EQUAL, current_char);
        else {
            lexer_unconsume_char(lexer, file, current_char);
            set_single_token(lexer, TOKEN_ASSIGN, '=');
        }
        break;

    case '<':
        current_char = lexer_consume_char(lexer, file);
        if (current_char == '=')
            set_single_token(lexer, TOKEN_EQUAL_LESS, '<');
        else {
            lexer_unconsume_char(lexer, file, current_char);
            set_single_token(lexer, TOKEN_LESS, '<');
        }
        break;

    case '>':
        current_char = lexer_consume_char(lexer, file);
        if (current_char == '=')
            set_single_token(lexer, TOKEN_EQUAL_GREATER, '>');
        else {
            lexer_unconsume_char(lexer, file, current_char);
            set_single_token(lexer, TOKEN_GREATER, '>');
        }
        break;

    case '/':
        current_char = lexer_consume_char(lexer, file);
        // Проверка на комментарий
        if (current_char == '/')
            read_comment(lexer, file, current_char);
        // Проверка на блоковый комментарий
        else if (current_char == '*')
            read_block_comment(lexer, file, current_char, false);
        else {
            lexer_unconsume_char(lexer, file, current_char);
            set_single_token(lexer, TOKEN_DIVISION, '/');
        }
        break;

    default:
        break;
    }
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
        } else if (current_char == '\n') {
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

        /* Обработка чисел, начинающихся с 0 (возможно, шестнадцатеричных или с плавающей точкой) */
        else if (current_char == '0') {
            classify_number_token(lexer, file, current_char);
            return *lexer->current_token;
        }

        /* Обработка скобок */
        else if (is_bracket(current_char)){
            current_char = lexer_consume_char(lexer, file);
            // получение типа токена
            TokenType token = get_bracket_token(current_char);
            set_single_token(lexer, token, current_char);
            return *lexer->current_token;
        }

        /* Обработка операторов и комментариев */
        else if (is_operator(current_char)){
            read_operator(lexer, file);
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


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
    return character == ' ' || character == '\t' || character == '\r' || character == '\n';
}

bool is_hex_digit(const char character) {
    return (is_digit(character)) ||
           (character >= 'a' && character <= 'f') ||
           (character >= 'A' && character <= 'F');
}

bool write_str(FILE *file, int count, char **str) {
    // Переместить указатель файла назад к последней прочитанной последовательности символов
    if (fseek(file, -count, SEEK_CUR) != 0) {
        fprintf(stderr, "fseek failed\n");
        return false;
    }

    // Выделить или перераспределить память для строки (+1 для нулевого терминатора)
    char *temp = realloc(*str, count + 1);
    if (temp == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }
    *str = temp;

    // Заполнить новую память символами из файла
    for (int i = 0; i < count; i++) {
        int c = fgetc(file);
        if (c == EOF) {
            (*str)[i] = '\0';
            return false;
        }
        (*str)[i] = (char)c;
    }

    // Нулевой терминатор
    (*str)[count] = '\0';

    return true;
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

char lexer_consume_char(Lexer *lexer, FILE *file) {
    // Увеличить позицию лексера
    // Читать следующий символ из файла
    lexer->position++;
    char character = fgetc(file);

    return character;
}

void check_exponent_part(Lexer *lexer, FILE *file, int characters_read) {
    // Первый символ 'e' или 'E' уже прочитан
    // Читаем следующий символ
    char current_char = lexer_consume_char(lexer, file);
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
    set_multi_token(lexer, TOKEN_EXP, file, characters_read);

}

void check_float_part(Lexer *lexer, FILE *file, int characters_read) {
    // Первый символ '.' уже прочитан
    // Читаем следующий символ
    char current_char = lexer_consume_char(lexer, file);
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
            check_exponent_part(lexer, file, characters_read);
            return;
        }
    }
    // Последний прочитанный символ не является цифрой, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);
    characters_read--;
    // Установить тип токена в TOKEN_FLOAT
    set_multi_token(lexer, TOKEN_FLOAT, file, characters_read);

}

void check_hex_part(Lexer *lexer, FILE *file, int characters_read) {
    // Первый символ 'x' уже прочитан
    // Читаем следующий символ
    char current_char = lexer_consume_char(lexer, file);
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
    set_multi_token(lexer, TOKEN_HEX, file, characters_read);

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

void set_multi_token(Lexer *lexer, TokenType type, FILE *file, int characters_read) {
    lexer->current_token->type = type;
    lexer->current_token->line = lexer->line;
    write_str(file, characters_read, &lexer->current_token->data);
}

void read_identifier(Lexer *lexer, FILE *file) {
    // В начале ещё ничего не прочитано
    int characters_read = 1;
    char current_char = lexer_consume_char(lexer, file);

    // Мы знаем, что первый символ является буквой
    while (is_letter(current_char) || (current_char == '_') ||
           is_digit(current_char)) {
        characters_read++;
        current_char = lexer_consume_char(lexer, file);
    }
    // Последний прочитанный символ не принадлежит идентификатору, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);
    characters_read--;
    // Установить тип токена в TOKEN_IDENTIFIER
    set_multi_token(lexer, TOKEN_IDENTIFIER, file, characters_read);
}

void read_global_identifier(Lexer *lexer, FILE *file) {
    // Первый символ является '_', можно читать дальше
    char current_char = lexer_consume_char(lexer, file);
    int characters_read = 1;

    char next_characters[2];
    // Проверить следующие два символа
    next_characters[0] = peek_char(file);
    next_characters[1] = peek_next_char(file);
    if (next_characters[0] != '_' || !is_letter(next_characters[1])) {
        raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid global identifier");
    }

    // Читать символы, пока они принадлежат идентификатору
    while (is_letter(current_char) || (current_char == '_') ||
           is_digit(current_char)) {
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
    }
    // Последний прочитанный символ не принадлежит идентификатору, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);
    characters_read--;
    // Установить тип токена в TOKEN_GLOBAL_IDENTIFIER
    set_multi_token(lexer, TOKEN_GLOBAL_IDENTIFIER, file, characters_read);
}

void read_whitespace(Lexer *lexer, FILE *file) {
    char current_char = lexer_consume_char(lexer, file);
    char last_whitespace = current_char;
    bool found_newline = false; // Флаг для отслеживания новой строки

    // Если функция вызвана при обнаружении новой строки
    if (current_char == '\n'){
        found_newline = true;
        // Обновляем номер строки и позицию после обработки конца строки
        lexer->line++;
        lexer->position = 1;
    }

    // Пока читаем пробельные символы
    while (is_whitespace(current_char)) {
        last_whitespace = current_char;

        current_char = lexer_consume_char(lexer, file);

        // ТК блоковый комментарий является whitespace
        // проверяем на блоковый комментарий после прочтения пробельного символа
        if (current_char == '/' && peek_char(file) == '*') {
            read_block_comment(lexer, file, true);
            // После комментария считаем пробельным символом
            current_char = ' ';
        
        // Обработка однострочного комментария, чтобы он не создал дополнительный EOL
        } else if (current_char == '/' && peek_char(file) == '/') {
            read_comment(lexer, file, true);
            // После комментария считаем символом новой строки
            current_char = '\n';
        }

        // Если новая строка, обновляем номер строки и позицию
        if (current_char == '\n') {
            found_newline = true;
            lexer->line++;
            lexer->position = 1;
        }
    }

    // Последний прочитанный символ не является пробельным, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);

    // Если была найдена новая строка, создать токен TOKEN_EOL
    if (found_newline)
        set_single_token(lexer, TOKEN_EOL, '\n');

    // Иначе создать токен для последнего пробельного символа
    else
        set_single_token(lexer, TOKEN_WHITESPACE, last_whitespace);
}

void read_number(Lexer *lexer, FILE *file) {
    // Мы знаем, что первый символ является цифрой
    char current_char = lexer_consume_char(lexer, file);
    int characters_read = 1;
    // Читать цифры, пока не встретится что-то другое
    while (is_digit(current_char)) {
        // Читаем следующий символ
        current_char = lexer_consume_char(lexer, file);
        characters_read++;
        // Проверяем, не начинается ли часть с плавающей точкой или экспонентой
        if (current_char == 'e' || current_char == 'E') {
            check_exponent_part(lexer, file, characters_read);
            return;
        }
        else if (current_char == '.') {
            check_float_part(lexer, file, characters_read);
            return;
        }
    }
    // Последний прочитанный символ не принадлежит числу, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);
    characters_read--;
    // Установить тип токена в TOKEN_INT
    set_multi_token(lexer, TOKEN_INT, file, characters_read);
}

void classify_number_token(Lexer *lexer, FILE *file) {
    // Первый символ '0', нужно проверить следующий символ
    lexer_consume_char(lexer, file);
    int characters_read = 1;
    if (is_digit(peek_char(file))) {
        // Ошибка: цифра после '0'
        raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid number format");
    }
    else if (peek_char(file) == 'x') {
        // Обработка шестнадцатеричных чисел
        // считываем 'x'
        lexer_consume_char(lexer, file);
        characters_read++;
        check_hex_part(lexer, file, characters_read);
    }
    else if (peek_char(file) == '.') {
        // Обработка чисел с плавающей точкой
        // считываем '.'
        lexer_consume_char(lexer, file);
        characters_read++;
        check_float_part(lexer, file, characters_read);
    }
    else if (peek_char(file) == 'e' || peek_char(file) == 'E') {
        // Обработка чисел в экспоненциальной форме
        // считываем 'e' или 'E'
        lexer_consume_char(lexer, file);
        characters_read++;
        check_exponent_part(lexer, file, characters_read);
    }
    else {
        // Это просто '0'
        set_single_token(lexer, TOKEN_INT, '0');
    }
}

void read_comment(Lexer *lexer, FILE *file, bool after_whitespace){
    char current_char = lexer_consume_char(lexer, file);
    
    // Комментарий идет до конца строки.
    while(current_char != '\n' && current_char != EOF)
        current_char = lexer_consume_char(lexer, file);

    // Если достигнут конец файла, вернуть символ обратно в поток
    // и установить EOL
    if (current_char == EOF){
        lexer_unconsume_char(lexer, file, current_char);
        set_single_token(lexer, TOKEN_EOL, '\n');
    }
    // Если комментарий был после пробела, 
    // то продолжается основной цикл чтения пробелов
    else if (!after_whitespace){
        lexer_unconsume_char(lexer, file, current_char);
        read_whitespace(lexer, file);
    }
}

void read_block_comment(Lexer *lexer, FILE *file, bool after_whitespace){
    int count_block_comment = 1; // Счетчик открытых блоков комментариев
    char current_char = lexer_consume_char(lexer, file);
    // цикл пока не закроются все блоки комментариев
    while (count_block_comment > 0){
        
        // Если файл закончился, а блок не закрыт
        // значит ошибка
        if (current_char == EOF){
            raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid block comment");
        }
        // Проверка на конец блока
        else if (current_char == '*'){
            current_char = peek_char(file);
            if(current_char == '/'){
                count_block_comment--;
                lexer_consume_char(lexer, file);
            }
        }
        // Проверка на начало нового блока, в случае вложенных блоков обновить счетчик
        else if (current_char == '/'){
            current_char = peek_char(file);
            if(current_char == '*'){
                count_block_comment++;
                lexer_consume_char(lexer, file);
            }
        }
        // Обновляем номер строки и позицию после обработки конца строки
        else if (current_char == '\n'){
            lexer->line++;
            lexer->position = 1;
        }
        current_char = lexer_consume_char(lexer, file);
        
    }
    
    // Если блок комментария был после пробела
    // то продолжается основной цикл чтения пробелов
    if (after_whitespace == true)
        return;
    
    // Запуск поиска следующего whitespace
    // Дабы избежать нескольких токенов whitespace подряд
    read_whitespace(lexer, file);
}

void read_operator(Lexer *lexer, FILE *file){
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
        // Проверка на оператор "!="
        if (current_char == '=')
            set_multi_token(lexer, TOKEN_NOT_EQUAL, file, strlen("!="));
        // Если после '!' не стоит '=', то это ошибка
        else{
            lexer_unconsume_char(lexer, file, current_char);
            raise_error(LEXER_ERROR, lexer->line, lexer->position, "Invalid operator");
        }
        break;

    case '=':
        current_char = lexer_consume_char(lexer, file);
        // Проверка на оператор "=="
        if (current_char == '=')
            set_multi_token(lexer, TOKEN_EQUAL, file, strlen("=="));
        else {
            lexer_unconsume_char(lexer, file, current_char);
            set_single_token(lexer, TOKEN_ASSIGN, '=');
        }
        break;

    case '<':
        current_char = lexer_consume_char(lexer, file);
        // Проверка на оператор "<="
        if (current_char == '=')
            set_multi_token(lexer, TOKEN_EQUAL_LESS, file, strlen("<="));
        else {
            lexer_unconsume_char(lexer, file, current_char);
            set_single_token(lexer, TOKEN_LESS, '<');
        }
        break;

    case '>':
        current_char = lexer_consume_char(lexer, file);
        // Проверка на оператор ">="
        if (current_char == '=')
            set_multi_token(lexer, TOKEN_EQUAL_GREATER, file, strlen(">="));
        else {
            lexer_unconsume_char(lexer, file, current_char);
            set_single_token(lexer, TOKEN_GREATER, '>');
        }
        break;

    case '/':
        current_char = lexer_consume_char(lexer, file);
        // Проверка на комментарий
        if (current_char == '/')
            read_comment(lexer, file, false);

        // Проверка на блоковый комментарий
        else if (current_char == '*')
            read_block_comment(lexer, file, false);

        // Если это не комментарий, то это оператор деления
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
            read_identifier(lexer, file);

        /* Проверить, является ли идентификатор ключевым словом */
            if (is_keyword(lexer->current_token->data)){
                lexer->current_token->type = TOKEN_KEYWORD;
            }
            return *lexer->current_token;

        /* Обработка глобальных идентификаторов (слова, начинающиеся с _) */
        } else if (current_char == '_') {
            read_global_identifier(lexer, file);
            return *lexer->current_token;
        }

        /* Обработка пробелов, табуляций, новых строк 
        (включает обработку комментариев после \n или EOL) */
        else if (is_whitespace(current_char)) {
            read_whitespace(lexer, file);
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
            read_number(lexer, file);
            return *lexer->current_token;
        }

        /* Обработка чисел, начинающихся с 0 (возможно, шестнадцатеричных или с плавающей точкой) */
        else if (current_char == '0') {
            classify_number_token(lexer, file);
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

        /* Обработка операторов и комментариев 
        (включают обработку EOL и whitespace после комментария) */
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


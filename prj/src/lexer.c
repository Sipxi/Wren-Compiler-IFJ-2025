// Имплементация функций лексера для токенизации исходного кода
//
// Авторы: 
// Serhij Čepil (253038)
//
//! Допишите ваши имена и номера

#include "lexer.h"
#include "error.h"
#include <string.h>
#include <stdbool.h>

/* ======================================*/
/* ===== Определение приватных функций лексера =====*/
/* ======================================*/

/**
 * Проверяет, является ли символ допустимым символом для идентификаторов (буквы и
 * цифры).
 *
 * @param character Символ для проверки.
 * @return true если символ является буквой или цифрой, иначе false.
 */
static bool is_letter(char character);

/**
 * Проверяет, является ли символ оператором.
 *
 * @param character Символ для проверки.
 * @return true если символ является оператором, иначе false.
 */
static bool is_operator(char character);

/**
 * Проверяет, начинается ли текущая позиция в файле с комментария.
 * @param file Указатель на файл для проверки.
 * @return true если текущая позиция в файле начинается с комментария, иначе false.
 */
static bool is_comment_start(FILE *file);

/**
 * Проверяет, является ли символ цифрой (0-9).
 *
 * @param character Символ для проверки.
 * @return true если символ является цифрой, иначе false.
 */
static bool is_digit(char character);

/**
 * Проверяет, является ли текущий идентификатор ключевым словом.
 * 
 * Если да, то обновляет тип токена на соответствующий тип ключевого слова.
 * @param lexer Указатель на структуру Lexer.
 * @return true если текущий идентификатор является ключевым словом, иначе false
 */
static bool is_keyword(const char *str);

/**
 * Проверяет, является ли символ пробельным (например, пробел, табуляция, новая строка).
 *
 * @param character Символ для проверки.
 * @return true если символ является пробельным, иначе false.
 */
static bool is_whitespace(const char character);

/**
 * Проверяет, является ли символ шестнадцатеричной цифрой (0-9, a-f, A-F).
 *
 * @param character Символ для проверки.
 * @return true если символ является шестнадцатеричной цифрой, иначе false.
 */
static bool is_hex_digit(const char character);

/**
 * Проверка, является ли символ скобкой 
 * 
 * @param character Символ для проверки.
 * @return true если символ является скобкой, иначе false.
 */
static bool is_bracket(char character);

/**
 * Записывает указанное количество символов из файла в строку.
 *
 * Эта функция читает символы из файла и записывает их в указанную строку.
 * @param file Указатель на файл, из которого будут прочитаны символы.
 * @param count Количество символов для чтения.
 * @param str Строка для записи.
 * @return Количество записанных символов.
 */
static bool write_str(FILE *file, int count, char **str);

/**
 * Функция для возврата нужного токена скобки
 * 
 * @param character Символ для перевода.
 * @return Токен от нужной скобки
 */
static TokenType get_bracket_token (char character);

/**
 * Просматривает следующий символ в файле без его удаления из потока.
 *
 * @param file Указатель на файл для просмотра следующего символа.
 * @return Следующий символ в файле.
 */
static char peek_char(FILE *file);

/**
 * Просматривает символ после следующего в файле без его удаления из потока.
 * 
 * @param file Указатель на файл для просмотра символа после следующего.
 * @return Символ после следующего в файле.
 */
static char peek_next_char(FILE *file);

/**
 * Читает следующий символ из файла и обновляет позицию лексера.
 * 
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл для чтения следующего символа.
 * @return Прочитанный символ.
 */
static char lexer_consume_char(Lexer *lexer, FILE *file);

/**
 * Проверяет и обрабатывает часть экспоненты числа
 *
 * Эта функция читает символы из файла,
 * и обрабатывает возможный знак '+' или '-' и последующие цифры.
 * Если формат экспоненты неверен, вызывается ошибка лексера.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 * @param current_char Текущий символ, который уже был прочитан ('e' или 'E').
 * @param characters_read Количество символов, прочитанных до вызова этой функции.
 */
static void check_exponent_part(Lexer *lexer, FILE *file, int characters_read);

/**
 * Проверяет и обрабатывает часть числа с плавающей точкой
 *
 * Эта функция читает символы из файла,
 * и обрабатывает последующие цифры.
 * Если формат числа с плавающей точкой неверен, вызывается ошибка лексера.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 * @param current_char Текущий символ, который уже был прочитан ('.').
 * @param characters_read Количество символов, прочитанных до вызова этой функции.
 */
static void check_float_part(Lexer *lexer, FILE *file, int characters_read);

/**
 * Проверяет и обрабатывает часть шестнадцатеричного числа
 *
 * Эта функция читает символы из файла,
 * и обрабатывает последующие шестнадцатеричные цифры.
 * Если формат шестнадцатеричного числа неверен, вызывается ошибка лексера.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 * @param current_char Текущий символ, который уже был прочитан ('x').
 * @param characters_read Количество символов, прочитанных до вызова этой функции.
 */
static void check_hex_part(Lexer *lexer, FILE *file, int characters_read);

/**
 * Возвращает последний прочитанный символ обратно в поток и обновляет позицию лексера.
 * 
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл для возврата символа.
 */
static void lexer_unconsume_char(Lexer *lexer, FILE *file, char current_char);

/**
 * Устанавливает токен с указанным типом и данными.
 * 
 * @param lexer Указатель на структуру Lexer.
 * @param type Тип токена для установки.
 * @param data Данные токена для установки (один символ).
*/
static void set_single_token(Lexer *lexer, TokenType type, const char data);

/**
 * Устанавливает токен с указанным типом и данными.
 * 
 * @param lexer Указатель на структуру Lexer.
 * @param type Тип токена для установки.
 * @param file Указатель на файл, содержащий исходный код.
 * @param characters_read Количество символов, прочитанных для токена.
 * 
 * 
*/
static void set_multi_token(Lexer *lexer, TokenType type, FILE *file, int characters_read);

/**
 * Читает идентификатор из исходного кода.
 *
 * Эта функция читает символы из файла для создания токена
 * идентификатора.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 */
static void read_identifier(Lexer *lexer, FILE *file);

/**
 * Читает глобальный идентификатор, например (__a2) из исходного кода.
 *
 * Эта функция читает символы из файла для создания токена
 * глобального идентификатора.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 */
static void read_global_identifier(Lexer *lexer, FILE *file);

/**
 * 
 * Считывает последовательность пробельных символов, новых строк из входного потока
 * и последующими за ними комментариями, 
 * возвращает первый непустой символ обратно и создаёт токен TOKEN_WHITESPACE.
 *
 * Эта функция читает символы из файла для создания одного токена.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 * @returns true если был создан токен TOKEN_EOL
 */
static bool read_whitespace(Lexer *lexer, FILE *file);

/**
 * Читает число (целое, с плавающей точкой, экспоненциальное) из исходного кода.
 *
 * Эта функция читает символы из файла для создания токена числа.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 */
static void read_number(Lexer *lexer, FILE *file);

/**
 * Классифицирует число, начинающееся с '0', как целое, с плавающей точкой,
 * экспоненциальное или шестнадцатеричное.
 *
 * Эта функция читает символы из файла для создания токена числа.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 */
static void classify_number_token(Lexer *lexer, FILE *file);

/**
 * Читает однострочный комментарий из исходного кода.
 * 
 * Эта функция читает символы из файла для поиска конца однострочного комментария.
 * 
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 * @param character Текущий символ.
 * @param after_whitespace Вызвана ли функция внутри проверки на пробел.
 * @returns true если был найден EOF
 */
static bool read_comment(Lexer *lexer, FILE *file);

/**
 * Читает блоковый комментарий из исходного кода.
 *
 * Эта функция читает символы из файла для поиска конца блокового комментария.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 * @param character Текущий символ.
 * @param after_whitespace Вызвана ли функция внутри проверки на пробел.
 */
static void read_block_comment(Lexer *lexer, FILE *file);

/**
 * Читает оператор из исходного кода
 * 
 * Эта функция читает символы из файла для создания токена
 * идентификатора.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 * @param character Текущий символ.
 */
static void read_operator(Lexer *lexer, FILE *file);

/**
 * Читает строку из исходного кода.
 *
 * Эта функция читает символы из файла для создания токена
 * строки.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 */
static void read_string(Lexer *lexer, FILE *file);

/**
 * Обрабатывает вариант многострочной строки из исходного кода.
 *
 * Эта функция читает символы из файла для создания токена
 * многострочной строки.
 */
static void read_multiline_string(Lexer *lexer, FILE *file, int characters_read);

/**
 * Обрабатывает вариант пустой строки из исходного кода.
 *
 * Эта функция читает символы из файла для создания токена
 * пустой строки.
 */
static void read_empty_string(Lexer *lexer, FILE *file, int characters_read);

/**
 * Обрабатывает вариант регулярной строки из исходного кода.
 *
 * Эта функция читает символы из файла для создания токена
 * регулярной строки.
 * 
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 * @param characters_read Количество символов, прочитанных для токена.
 */
static void read_regular_string(Lexer *lexer, FILE *file, int characters_read);


/* ====================================*/
/* ===== Имплементация приватных функций лексера =====*/
/* ====================================*/

static bool is_letter(char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z');
}

static bool is_operator(char character){
    return (character == '+' || character == '-' || character == '=' || 
        character == '/' || character == '*' || character == '!' || 
        character == '<' || character == '>');
}

static bool is_digit(char character) {
    return (character >= '0' && character <= '9');
}

static bool is_keyword(const char *str){
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

static bool is_whitespace(const char character) {
    return character == ' ' || character == '\t' || character == '\r' || character == '\n';
}

static bool is_hex_digit(const char character) {
    return (is_digit(character)) ||
           (character >= 'a' && character <= 'f') ||
           (character >= 'A' && character <= 'F');
}

static bool write_str(FILE *file, int count, char **str) {
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

static bool is_bracket(char character){
    return (character == ')' || character == '(' || 
        character == '}' || character == '{');
}

static TokenType get_bracket_token(char character){
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

static char peek_char(FILE *file) {
    int character = fgetc(file);
    ungetc(character, file);
    return (char)character;
}

static char peek_next_char(FILE *file) {
    int character = fgetc(file);
    int next_character = fgetc(file);
    ungetc(next_character, file);
    ungetc(character, file);
    return (char)next_character;
}

static char lexer_consume_char(Lexer *lexer, FILE *file) {
    // Увеличить позицию лексера
    // Читать следующий символ из файла
    lexer->position++;
    char character = fgetc(file);

    return character;
}

static void check_exponent_part(Lexer *lexer, FILE *file, int characters_read) {
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

static void check_float_part(Lexer *lexer, FILE *file, int characters_read) {
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

static void check_hex_part(Lexer *lexer, FILE *file, int characters_read) {
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

static void lexer_unconsume_char(Lexer *lexer, FILE *file, char current_char) {
    // Вернуть последний прочитанный символ обратно в поток
    ungetc(current_char, file);
    // Уменьшить позицию лексера
    lexer->position--;
}

static void set_single_token(Lexer *lexer, TokenType type, const char data) {
    lexer->current_token->type = type;
    lexer->current_token->line = lexer->line;
    lexer->current_token->data[0] = data;
    lexer->current_token->data[1] = '\0';
}

static void set_multi_token(Lexer *lexer, TokenType type, FILE *file, int characters_read) {
    lexer->current_token->type = type;
    lexer->current_token->line = lexer->line;
    write_str(file, characters_read, &lexer->current_token->data);
}

static void read_identifier(Lexer *lexer, FILE *file) {
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

static void read_global_identifier(Lexer *lexer, FILE *file) {
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

bool read_whitespace(Lexer *lexer, FILE *file) {
    char current_char = lexer_consume_char(lexer, file);
    bool found_newline = false; // Флаг для отслеживания новой строки

    // Пока читаем пробельные символы или потанциальное начало комментария
    while (is_whitespace(current_char) || current_char == '/') {
        // Обработка комментариев
        if (current_char == '/') {
            // Проверка на блочный комментарий
            if (peek_char(file) == '*')
                read_block_comment(lexer, file);
            // Проверка на однострочный комментарий
            else if (current_char == '/' && peek_char(file) == '/') {
                if (read_comment(lexer, file)) // если был найден EOF возращаем токен EOL
                    return true;
            } else // Это не комментарий, выходим из цикла
                break;
            current_char = ' '; // Заменяем для этого цикла комментарий на пробел
        }
        // Если новая строка, обновляем номер строки и позицию
        if (current_char == '\n') {
            found_newline = true;
            lexer->line++;
            lexer->position = 1;
        }
        current_char = lexer_consume_char(lexer, file);

    }

    // Последний прочитанный символ не является пробельным, вернуть его обратно в поток
    lexer_unconsume_char(lexer, file, current_char);

    // Если была найдена новая строка, создать токен TOKEN_EOL
    if (found_newline)
        set_single_token(lexer, TOKEN_EOL, '\n');

    return found_newline;
}

static void read_number(Lexer *lexer, FILE *file) {
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

static void classify_number_token(Lexer *lexer, FILE *file) {
    // Первый символ '0', нужно проверить следующий символ
    lexer_consume_char(lexer, file);
    int characters_read = 1;
    // Оказывается, 0456 это нормальное число во wren, выведет 456
    // Поэтому 0 просто убираем и читаем дальше как обычное число
    if (is_digit(peek_char(file))) {
        read_number(lexer, file);
        return;
    }
    // Во wren работает только 0x, а не 0X для hex
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
    // Во wren работает e и E для экспоненты
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

static bool is_comment_start(FILE *file){
    if (peek_char(file) == '/') {
        char next_char = peek_next_char(file);
        return next_char == '/' || next_char == '*';
    }
    return false;
}

static bool read_comment(Lexer *lexer, FILE *file){
    char current_char = lexer_consume_char(lexer, file);
    
    // Комментарий идет до конца строки.
    while(current_char != '\n' && current_char != EOF)
        current_char = lexer_consume_char(lexer, file);

    lexer_unconsume_char(lexer, file, current_char);
    
    // Если достигнут конец файла, вернуть символ обратно в поток
    // и установить EOL
    if (current_char == EOF){
        set_single_token(lexer, TOKEN_EOL, '\n');
        return true; 
    }

    return false;
}

static void read_block_comment(Lexer *lexer, FILE *file){
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
    
}

static void read_operator(Lexer *lexer, FILE *file){
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
        lexer_unconsume_char(lexer, file, current_char);
        set_single_token(lexer, TOKEN_DIVISION, '/');
        break;

    default:
        break;
    }
}

static void read_string(Lexer *lexer, FILE *file) {
    // Функции для чтения строк
    
    // Проглатываем первую кавычку
    int characters_read = 1;
    lexer_consume_char(lexer, file);
    
    if (peek_char(file) == '"') {
        if (peek_next_char(file) == '"') {
            read_multiline_string(lexer, file, characters_read); // Многострочная строка
        } else {
            read_empty_string(lexer, file, characters_read); // Пустая строка
        }
    } else {
        read_regular_string(lexer, file, characters_read);// Обычная строка
    }
}

static void read_multiline_string(Lexer *lexer, FILE *file, int characters_read) {
    // Многострочный стринг: """..."""

    lexer_consume_char(lexer, file);
    lexer_consume_char(lexer, file);
    characters_read += 2;

    while (true) {
        char current = peek_char(file);
        
        if (current == EOF) {
            raise_error(LEXER_ERROR, lexer->line, lexer->position, 
                       "Unterminated multi-line string literal");
        }
        
        // находим первые 2 скобки
        if (current == '"' && peek_next_char(file) == '"') {
            lexer_consume_char(lexer, file);
            lexer_consume_char(lexer, file);
            characters_read += 2;
            // 3-я скобка, то выходим
            if (peek_char(file) == '"') {
            lexer_consume_char(lexer, file);
            characters_read++;
            break;
            }                    
        } else {
            // если новая строка - делаем апдейт строки и позиции
            if (current == '\n') {
            lexer->line++;
            lexer->position = 1;
            }
            lexer_consume_char(lexer, file); //двигаемся по буквам
            characters_read++;
        }
    }

    set_multi_token(lexer, TOKEN_MULTI_STRING, file, characters_read);
}

static void read_empty_string(Lexer *lexer, FILE *file, int characters_read) {
    // Пустой стринг ""

    lexer_consume_char(lexer, file);
    characters_read++;
    set_multi_token(lexer, TOKEN_STRING, file, characters_read);
}

static void read_regular_string(Lexer *lexer, FILE *file, int characters_read) {
    // Обычный стринг "..."

    // Пока не наткнемся на скобочку
    while (peek_char(file) != '"') {
        lexer_consume_char(lexer, file); //обрабатываем символы
        characters_read++;

        // Находим \ - значит дальше может быть ексейп-секвенция \"
        // Нам нужно отдельно обработать этот случай вне while, что бы не выйти из стринга
        if (peek_char(file) == '\\') {
            // Обрабатываем \ и " за ним
            // Вместо " может быть и любой другой символ, нас не интересует какой
            lexer_consume_char(lexer, file);
            lexer_consume_char(lexer, file);
            characters_read += 2;
        }

        // Ошибка в случае
        if (peek_char(file) == EOF || peek_char(file) == '\n') {
            raise_error(LEXER_ERROR, lexer->line, lexer->position, 
                       "Unterminated string literal");
        }
    }
    lexer_consume_char(lexer, file);
    characters_read++;
    set_multi_token(lexer, TOKEN_STRING, file, characters_read);
}

/* ==================================== */
/* ===== Имплементация публичных функций лексера =====*/
/* ==================================== */

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

        /* Обработка конца строки */
        } 

        /* Обработка пробелов, табуляций, новых строк и комментариев */
        else if (is_whitespace(current_char) || is_comment_start(file)) {
            if(read_whitespace(lexer, file)) // если при чтении последовательности белых знаков был найден EOL возращаем токен
                return *lexer->current_token;
            // иначе просто продолжаем читать дальше
            current_char = peek_char(file);
        }

        /* Обработка точки */
        else if (current_char == '.') {
            current_char = lexer_consume_char(lexer, file);
            set_single_token(lexer, TOKEN_DOT, current_char);
            return *lexer->current_token;
        }

        /* Обработка целых чисел, которые не начинаются с 0 */
        else if (is_digit(current_char) && current_char != '0') {
            read_number(lexer, file);
            return *lexer->current_token;
        }

        /* Обработка чисел, начинающихся с 0 (возможно, шестнадцатеричных или с плавающей точкой) */
        else if (current_char == '0') {
            classify_number_token(lexer, file);
            return *lexer->current_token;
        }

        /*Обработка стрингов*/
        else if(current_char == '"'){
            read_string(lexer, file);
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

        /* Обработка операторов */
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


// Имплементация функций лексера для токенизации исходного кода
//
// Авторы:
// Serhij Čepil (253038)
// Dmytro Kravchenko (273125)
// Veronika Turbaievska (273123)
//
//! Допишите ваши имена и номера
// TODO: Комментарии к функциям
#include "lexer.h"

#include <stdbool.h>
#include <string.h>

#include "error.h"

/* ======================================*/
/* ===== FSM (Finite State Machine) =====*/
/* ======================================*/

typedef enum {
    STATE_START, //* Переходное состояние
    STATE_EOF,
    STATE_EOL,
    STATE_IDENTIFIER,
    STATE_ONE_UNDERSCORE, //* Переходное состояние после одного _
    STATE_TWO_UNDERSCORE,   //* Переходное состояние после двух __
    STATE_GLOBAL_IDENTIFIER,
    STATE_DOT,
    STATE_NUMBER,
    STATE_ZERO_START,
    STATE_HEX_NUMBER,
    STATE_FLOAT_NUMBER,
    STATE_EXP_NUMBER,
    STATE_DECIMAL_POINT, //* Переходное состояние после десятичной точки 
    STATE_CHECK_EXPONENT, //* Переходное состояние для проверки условий экспоненты
    STATE_SIGN_EXP_NUMBER, //* Переходное состояние для знака в экспоненте
    STATE_HEX_PREFIX, //* Переходное состояние после 0x
    STATE_PLUS,
    STATE_MINUS,
    STATE_MULTIPLY,
    STATE_DIVISION,
    STATE_ASSIGN,
    STATE_EQUAL,
    STATE_NOT_EQUAL,
    STATE_LESS,
    STATE_EQUAL_LESS,
    STATE_GREATER,
    STATE_EQUAL_GREATER,
    STATE_OPEN_PAREN,
    STATE_CLOSE_PAREN,
    STATE_OPEN_BRACE,
    STATE_CLOSE_BRACE,

    STATE_FIRST_QUOT,
    STATE_SECOND_QUOT,
    STATE_SINGLE_STRING,
    STATE_SLASH,
    STATE_STRING_END,
    STATE_MULTIPLE_STRING,
    STATE_CLOSING_QUOT,
    STATE_SECOND_CLOSING_QUOT,
    STATE_MULTIPLE_STRING_END,

    STATE_COMMENT,
    STATE_DONE,
    STATE_START_BLOCK_COMMENT,
    STATE_END_BLOCK_COMMENT,
    STATE_OPEN_BLOCK_COMMENT,
    STATE_CLOSE_BLOCK_COMMENT,
    STATE_BODY_BLOCK_COMMENT
} LexerFSMState;

/* ======================================*/
/* ===== Определение приватных функций лексера =====*/
/* ======================================*/

/**
 * Проверяет, является ли символ допустимым символом для идентификаторов (буквы
 * и цифры).
 *
 * @param character Символ для проверки.
 * @return true если символ является буквой или цифрой, иначе false.
 */
static bool is_letter(char character);

/**
 * Проверяет, начинается ли текущая позиция в файле с комментария.
 * @param file Указатель на файл для проверки.
 * @return -1 если не комментарий, 0 если однострочный комментарий, 1 если многострочный комментарий
 */
static int is_comment_start(char current_char, FILE *file);

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
 * Обрабатывает конец блокового комментария
 *
 * Эта функция читает символы из файла,
 * и обрабатывает возможное завершение блокового комментария.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 * @return true если был найден конец блокового комментария, иначе false.
 */
static bool is_end_block_comment(char current_char, FILE *file);

/**
 * Проверяет, является ли символ пробельным (например, пробел, табуляция, новая
 * строка).
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
 * Возвращает последний прочитанный символ обратно в поток и обновляет позицию
 * лексера.
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
static void set_multi_token(Lexer *lexer, TokenType type, FILE *file,
                            int characters_read);

/**
 * Читает число (целое, с плавающей точкой, экспоненциальное) из исходного кода.
 *
 * Эта функция читает символы из файла для создания токена числа.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 */
static void read_number(Lexer *lexer, FILE *file, char current_char);

/**
 * Классифицирует число, начинающееся с '0', как целое, с плавающей точкой,
 * экспоненциальное или шестнадцатеричное.
 *
 * Эта функция читает символы из файла для создания токена числа.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 */
static void classify_number_token(Lexer *lexer, FILE *file, char current_char);


/**
 * Изменяет состояние конечного автомата лексера и возвращает последний
 * прочитанный символ обратно в поток.
 *
 * @param lexer Указатель на структуру Lexer.
 * @param file Указатель на файл, содержащий исходный код.
 * @param current_state Указатель на текущее состояние конечного автомата
 * лексера.
 * @param next_state Следующее состояние конечного автомата лексера.
 * @param current_char Текущий символ, который уже был прочитан.
 */
static void change_state(FILE *file, Lexer *lexer, LexerFSMState *current_state,
                         LexerFSMState next_state, char current_char);

/* ====================================*/
/* ===== Имплементация приватных функций лексера =====*/
/* ====================================*/

static bool is_end_block_comment(char current_char, FILE *file) {
    // Проверить, является ли текущий символ концом блочного комментария
    return current_char == '*' && peek_char(file) == '/';
}

static bool is_letter(char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z');
}

static bool is_digit(char character) {
    return (character >= '0' && character <= '9');
}

static bool is_keyword(const char *str) {
    const char *keywords[] = {"class",  "if",  "else",  "is",     "null",
                              "return", "var", "while", "Ifj",    "static",
                              "import", "for", "Num",   "String", "Null"};
    // Определить количество ключевых слов, с помощью sizeof
    int num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    for (int i = 0; i < num_keywords; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_whitespace(const char character) {
    return character == ' ' || character == '\t' || character == '\r';
}

static bool is_hex_digit(const char character) {
    return (is_digit(character)) || (character >= 'a' && character <= 'f') ||
           (character >= 'A' && character <= 'F');
}

static bool write_str(FILE *file, int count, char **str) {
    // Переместить указатель файла назад к последней прочитанной
    // последовательности символов
    if (fseek(file, -count, SEEK_CUR) != 0) {
        fprintf(stderr, "fseek failed\n");
        return false;
    }

    // Выделить или перераспределить память для строки (+1 для нулевого
    // терминатора)
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

static void set_multi_token(Lexer *lexer, TokenType type, FILE *file,
                            int characters_read) {
    lexer->current_token->type = type;
    lexer->current_token->line = lexer->line;
    write_str(file, characters_read, &lexer->current_token->data);
}

static int is_comment_start(char current_char, FILE *file) {
    int result = -1;
    if (current_char == '/') {
        char next_char = peek_char(file);
        if (next_char == '/') 
            result = 0;
        else if (next_char == '*') 
            result = 1;
    }
    return result;
}


static void change_state(FILE *file, Lexer *lexer, LexerFSMState *current_state,
                         LexerFSMState next_state, char current_char) {
    *current_state = next_state;
    lexer_unconsume_char(lexer, file, current_char);
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
    fprintf(stderr,
            "\033[1;31mLexical error.\nError code: %d\n%s at line %d, position "
            "%d\033[0m\n",
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
 set_single_token - устанавливает токен с указанным типом и данными (один
 символ) peek_char - просматривает следующий символ в файле без его удаления из
 потока peek_next_char - просматривает символ после следующего в файле без его
 удаления из потока
*/
Token get_next_token(Lexer *lexer, FILE *file) {
    // FSM реализация лексера
    // Если файл уже на конце файла, возвращаем TOKEN_EOF
    if (lexer->current_token->data[0] == EOF) {
        set_single_token(lexer, TOKEN_EOF, EOF);
        return *lexer->current_token;
    }
    LexerFSMState state = STATE_START;
    bool find_eol = false;
    int count_block_comment = 0;
    // Счетчик прочитанных символов для текущего токена
    int characters_read = 0;

    while (true) {
        // Чтение текущего символа и обновление позиции лексера
        char current_char = lexer_consume_char(lexer, file);

        switch (state) {
            case STATE_START:
                if (is_letter(current_char)) {
                    change_state(file, lexer, &state, STATE_IDENTIFIER,
                                 current_char);
                    break;
                } else if (is_whitespace(current_char)) {
                    change_state(file, lexer, &state, STATE_START,
                                    current_char);
                    lexer_consume_char(lexer, file);
                    break;
                } else if (current_char == '_') {
                    characters_read++;
                    change_state(file, lexer, &state, STATE_ONE_UNDERSCORE,
                                 current_char);
                    break;
                } else if (current_char == EOF) {
                    change_state(file, lexer, &state, STATE_EOF, current_char);
                    break;
                } else if (current_char == '.') {
                    change_state(file, lexer, &state, STATE_DOT, current_char);
                    break;
                } else if (is_digit(current_char) && current_char != '0') {
                    change_state(file,lexer, &state, STATE_NUMBER, current_char);
                    break;
                } else if (current_char == '0') {
                    change_state(file, lexer, &state, STATE_ZERO_START, current_char);
                    break;
                } else if (current_char == '"') {
                    state = STATE_FIRST_QUOT;
                    characters_read++;
                    break;
                } else if (current_char == '+') {
                    change_state(file, lexer, &state, STATE_PLUS, current_char);
                    break;
                } else if (current_char == '-') {
                    change_state(file, lexer, &state, STATE_MINUS, current_char);
                    break;
                } else if (current_char == '*') {
                    change_state(file, lexer, &state, STATE_MULTIPLY, current_char);
                    break;
                } else if (current_char == '!' && peek_char(file) == '=') {
                    change_state(file, lexer, &state, STATE_NOT_EQUAL, current_char);
                    break;
                } else if (current_char == '=') {
                    change_state(file, lexer, &state, STATE_ASSIGN, current_char);
                    break;
                } else if (current_char == '<') {
                    change_state(file, lexer, &state, STATE_LESS, current_char);
                    break;
                } else if (current_char == '>') {
                    change_state(file, lexer, &state, STATE_GREATER, current_char);
                    break;
                } else if (current_char == '/') {
                    change_state(file, lexer, &state, STATE_DIVISION, current_char);
                    break;
                } else if (current_char == '{') {
                    change_state(file, lexer, &state, STATE_OPEN_BRACE, current_char);
                    break;
                } else if (current_char == '}') {
                    change_state(file, lexer, &state, STATE_CLOSE_BRACE, current_char);
                    break;
                } else if (current_char == '(') {
                    change_state(file, lexer, &state, STATE_OPEN_PAREN, current_char);
                    break;
                } else if (current_char == ')') {
                    change_state(file, lexer, &state, STATE_CLOSE_PAREN, current_char);
                    break;
                } else if (current_char == '\n') {
                    change_state(file, lexer, &state, STATE_EOL, current_char);
                    break;
                } else {
                    lexer_error(lexer, LEXER_ERROR,
                                "Unknown character encountered");
                    break;
                }
            case STATE_DIVISION:
                // Проверяем, является ли следующий символ началом комментария
                if (peek_char(file) == '/')
                    change_state(file, lexer, &state, STATE_COMMENT, current_char);
                else if (peek_char(file) == '*') {
                    change_state(file, lexer, &state, STATE_START_BLOCK_COMMENT, current_char);
                    lexer_consume_char(lexer, file);            
                } else { // Это просто оператор деления
                    set_single_token(lexer, TOKEN_DIVISION, current_char);
                    return *lexer->current_token;
                }
                break;
            case STATE_START_BLOCK_COMMENT:
                // так как начало комментария это всегда 2 символа, то должны один перепрыгнуть
                current_char = lexer_consume_char(lexer, file);
                count_block_comment++; // увеличиваем счетчик вложенных блок комментариев
                change_state(file, lexer, &state, STATE_BODY_BLOCK_COMMENT, current_char);
                break;
            case STATE_BODY_BLOCK_COMMENT:
                if (current_char == '\n') // отслеживаем новые строки внутри комментария
                    lexer->line++;
                // Проверяем, является ли следующий символ началом комментария
                else if (is_comment_start(current_char, file) == 1) {
                    change_state(file, lexer, &state, STATE_START_BLOCK_COMMENT, current_char);
                    lexer_consume_char(lexer, file);
                } else if (is_end_block_comment(current_char, file)) {
                    change_state(file, lexer, &state, STATE_END_BLOCK_COMMENT, current_char);
                    lexer_consume_char(lexer, file);
                // если ни конец файла, это ошибка
                } else if (current_char == EOF) {
                    lexer_error(lexer, LEXER_ERROR, "Unterminated block comment");
                } 
                // иначе продолжаем оставаться в теле комментария
                break;
            case STATE_END_BLOCK_COMMENT:
                // так как начало комментария это всегда 2 символа, то должны один перепрыгнуть
                current_char = lexer_consume_char(lexer, file); 
                count_block_comment--;
                // если до этого вне комментария был EOL, то возвращаемся в EOL
                if (count_block_comment == 0 && find_eol)
                    change_state(file, lexer, &state, STATE_EOL, current_char);
                // иначе в пробельное состояние
                else if (count_block_comment == 0)
                    change_state(file, lexer, &state, STATE_START, current_char);
                // если все блоки не закрыты остаемся в теле комментария
                else   
                    change_state(file, lexer, &state, STATE_BODY_BLOCK_COMMENT, current_char);
                break;
            case STATE_COMMENT:
                // пока не достигнут конец строки
                if (current_char == '\n')
                    change_state(file, lexer, &state, STATE_EOL, current_char);
                // или конец файла
                else if (current_char == EOF){
                    change_state(file, lexer, &state, STATE_EOF, current_char);
                    find_eol = true; // устанавливаем флаг что был найден EOL, так как файл закончился
                }
                break;
            case STATE_EOL:
                // устанавливаем флаг что был найден EOL
                find_eol = true;
                // отслеживаем номер строки
                if (current_char == '\n')
                    lexer->line++;
                // отслеживаем комментарии после EOL
                else if (current_char == '/' && peek_char(file) == '/') {
                    change_state(file, lexer, &state, STATE_COMMENT, current_char);
                    lexer_consume_char(lexer, file);
                } else if (current_char == '/' && peek_char(file) == '*') {
                    change_state(file, lexer, &state, STATE_START_BLOCK_COMMENT, current_char);
                    lexer_consume_char(lexer, file);
                // если следующий символ не пробельный, то
                // возвращаем токен EOL и оставляем символ для следующего токена
                } else if (!is_whitespace(current_char)) {
                    set_single_token(lexer, TOKEN_EOL, '\n');
                    lexer_unconsume_char(lexer, file, current_char);
                    return *lexer->current_token;
                }
                break;

            case STATE_IDENTIFIER:
                // Мы знаем, что первый символ является буквой, читаем дальше
                if (is_letter(current_char) || (current_char == '_') ||
                    is_digit(current_char)) {
                    // Читаем дальше
                    characters_read++;
                        continue;
                    }
                // Последний прочитанный символ не принадлежит идентификатору, вернуть его
                // обратно в поток
                lexer_unconsume_char(lexer, file, current_char);
                set_multi_token(lexer, TOKEN_IDENTIFIER, file, characters_read);

                // Проверяем, является ли идентификатор ключевым словом
                if (is_keyword(lexer->current_token->data)) {
                    lexer->current_token->type = TOKEN_KEYWORD;
                }

                return *lexer->current_token;
            case STATE_ONE_UNDERSCORE:
                // При переходе читаем символ
                current_char = lexer_consume_char(lexer, file);
                if (current_char == '_'){
                    // Если второй символ тоже '_', переходим в состояние с двумя '_'
                    characters_read++;
                    change_state(file, lexer, &state, STATE_TWO_UNDERSCORE, current_char);
                }
                else{
                    lexer_error(lexer, LEXER_ERROR,"Invalid global identifier");
                }
                break;
            case STATE_TWO_UNDERSCORE:
                // При переходе читаем символ 
                current_char = lexer_consume_char(lexer, file);
                // Проверяем, что следующий символ является буквой
                if (is_letter(current_char)) {
                    characters_read++;
                    change_state(file, lexer, &state, STATE_GLOBAL_IDENTIFIER, current_char);
                } else {
                // Ошибка: неверный формат глобального идентификатора
                    lexer_error(lexer, LEXER_ERROR,"Invalid global identifier");
                }
                break;
            case STATE_GLOBAL_IDENTIFIER:
                if (is_letter(current_char) || (current_char == '_') ||
                    is_digit(current_char)) {
                    // Читаем дальше
                    characters_read++;
                        continue;
                    }
                // Последний прочитанный символ не принадлежит идентификатору, вернуть его
                // обратно в поток
                lexer_unconsume_char(lexer, file, current_char);
                set_multi_token(lexer, TOKEN_GLOBAL_IDENTIFIER, file, characters_read);
                
                return *lexer->current_token;
            case STATE_DOT:
                set_single_token(lexer, TOKEN_DOT, current_char);
                return *lexer->current_token;

            case STATE_NUMBER:
                // Проверяем, является ли следующий символ цифрой
                // Если да, продолжаем читать цифры
                if (is_digit(current_char)) {
                    // После того, как мы прочитали цифру, увеличиваем счетчик
                    characters_read++;
                    continue;
                }
                // Проверить, начинается ли десятичная часть или экспонента
                if (current_char == '.') {
                    // Сразу добавляем к счетчику символов, после if
                    characters_read++;
                    change_state(file, lexer, &state, STATE_DECIMAL_POINT, current_char);
                    break;
                } else if (current_char == 'e' || current_char == 'E') {
                    // Сразу добавляем к счетчику символов, после if
                    characters_read++;
                    change_state(file, lexer, &state, STATE_CHECK_EXPONENT, current_char);
                    break;
                }
                // Последний прочитанный символ не принадлежит числу, вернуть его обратно в
                // поток
                lexer_unconsume_char(lexer, file, current_char);
                // Установить тип токена в TOKEN_INT
                set_multi_token(lexer, TOKEN_INT, file, characters_read);
                // Вернуть текущий токен
                return *lexer->current_token;
                
            case STATE_ZERO_START:
                // Первый символ был '0' - увеличиваем счетчик
                characters_read++;
                // Считываем следующий символ после '0'
                current_char = lexer_consume_char(lexer, file);
                if (current_char == 'x') {
                    // Обработка шестнадцатеричных чисел
                    characters_read++;
                    change_state(file, lexer, &state, STATE_HEX_PREFIX, current_char);
                    break;
                } else if (current_char == '.') {
                    // Обработка чисел с плавающей точкой
                    characters_read++;
                    change_state(file, lexer, &state, STATE_DECIMAL_POINT, current_char);
                    break;
                } else if (current_char == 'e' || current_char == 'E') {
                    // Обработка чисел в экспоненциальной форме
                    characters_read++;
                    change_state(file, lexer, &state, STATE_CHECK_EXPONENT, current_char);
                    break;
                } else if (is_digit(current_char)) {
                    // Оказывается, 0456 это нормальное число во wren, выведет 456
                    // Поэтому 0 просто убираем и читаем дальше как обычное число
                    // Уменьшаем счетчик, так как '0' не учитывается в числе
                    characters_read--;
                    change_state(file, lexer, &state, STATE_NUMBER, current_char);
                    break;
                }
                // Это просто '0'
                set_single_token(lexer, TOKEN_INT, '0');
                return *lexer->current_token;

            case STATE_DECIMAL_POINT:
                // Считытываем следующий символ после '.'
                current_char = lexer_consume_char(lexer, file);
                if (!is_digit(current_char)) {
                    lexer_error(lexer, LEXER_ERROR, "Invalid float format");
                }
                // Если следующий символ является цифрой, переходим в состояние
                // чтения числа с плавающей точкой
                change_state(file, lexer, &state, STATE_FLOAT_NUMBER, current_char);
                break;

            case STATE_CHECK_EXPONENT:
                // Считытываем следующий символ после 'e' или 'E'
                current_char = lexer_consume_char(lexer, file);
                if (current_char == '+' || current_char == '-') {
                    // Считываем знак экспоненты
                    characters_read++;
                    change_state(file, lexer, &state, STATE_SIGN_EXP_NUMBER, current_char);
                    break;
                }
                // Проверить, что следующий символ является цифрой
                if (!is_digit(current_char)) {
                    lexer_error(lexer, LEXER_ERROR, "Invalid exponent format");
                }
                // Переходим в состояние чтения экспоненты
                change_state(file, lexer, &state, STATE_EXP_NUMBER, current_char);
                break;

            case STATE_SIGN_EXP_NUMBER:
                // Считываем следующий символ после '+' или '-'
                current_char = lexer_consume_char(lexer, file);
                // Проверить, что следующий символ является цифрой
                if (!is_digit(current_char)) {
                    lexer_error(lexer, LEXER_ERROR, "Invalid exponent format");
                }
                // Переходим в состояние чтения экспоненты
                change_state(file, lexer, &state, STATE_EXP_NUMBER, current_char);
                break;

            case STATE_HEX_PREFIX:
                // Считываем следующий символ после '0x'
                current_char = lexer_consume_char(lexer, file);
                // Проверить, что следующий символ является шестнадцатеричной цифрой
                if (!is_hex_digit(current_char)) {
                    lexer_error(lexer, LEXER_ERROR, "Invalid hexadecimal format");
                }
                change_state(file, lexer, &state, STATE_HEX_NUMBER, current_char);
                break;

            case STATE_HEX_NUMBER:
                // Следующий символ будет считываться в начале всего цикла
                if (is_hex_digit(current_char)) {
                    // После того, как мы прочитали шестнадцатеричную цифру,
                    // увеличиваем счетчик
                    characters_read++;
                    continue;
                }
                // Последний прочитанный символ не является шестнадцатеричной цифрой,
                // вернуть его обратно в поток
                lexer_unconsume_char(lexer, file, current_char);
                // Установить тип токена в TOKEN_HEX
                set_multi_token(lexer, TOKEN_HEX, file, characters_read);
                return *lexer->current_token;

            case STATE_FLOAT_NUMBER:
                // Следующий символ будет считываться в начале всего цикла
                if (is_digit(current_char)) {
                    // После того, как мы прочитали цифру, увеличиваем счетчик
                    characters_read++;
                    continue;
                }
                // Проверить, начинается ли экспонента
                if (current_char == 'e' || current_char == 'E') {
                    characters_read++;
                    change_state(file, lexer, &state, STATE_CHECK_EXPONENT, current_char);
                    break;
                }
                // Последний прочитанный символ не является цифрой, вернуть его обратно в
                // поток
                lexer_unconsume_char(lexer, file, current_char);
                // Установить тип токена в TOKEN_FLOAT
                set_multi_token(lexer, TOKEN_FLOAT, file, characters_read);
                return *lexer->current_token;

            case STATE_EXP_NUMBER:
                // Следующий символ будет считываться в начале всего цикла
                if (is_digit(current_char)) {
                    // После того, как мы прочитали цифру, увеличиваем счетчик
                    characters_read++;
                    continue;
                }
                // Последний прочитанный символ не является цифрой, вернуть его обратно в
                // поток
                lexer_unconsume_char(lexer, file, current_char);
                // Установить тип токена в TOKEN_EXP
                set_multi_token(lexer, TOKEN_EXP, file, characters_read);
                return *lexer->current_token;


/////////////////////////////////////////////

            case STATE_FIRST_QUOT:
                if(current_char == '"')
                {
                    characters_read++;
                    state = STATE_SECOND_QUOT;
                    break;
                }
                else if(current_char == '\\')
                {
                    characters_read++;
                    state = STATE_SLASH;
                    break;
                }
                else
                {
                    characters_read++;
                    state = STATE_SINGLE_STRING;
                    break;
                }
            case STATE_SECOND_QUOT:
                if(current_char != '"')
                {
                    characters_read++;
                    state = STATE_SINGLE_STRING;
                    break;
                }
                else 
                {
                    characters_read++;
                    state = STATE_MULTIPLE_STRING;
                    break;
                }
                
            case STATE_MULTIPLE_STRING:
                if(current_char == '"')
                {
                    characters_read++;
                    state = STATE_CLOSING_QUOT;
                    break;
                }
                else
                {
                    characters_read++;
                    continue;
                }
            

            case STATE_CLOSING_QUOT:
                if(current_char == '"')
                {
                    characters_read++;
                    state = STATE_SECOND_CLOSING_QUOT;
                    break;
                }
                else
                {
                    characters_read++;
                    state = STATE_MULTIPLE_STRING;
                    break;
                }

            case STATE_SECOND_CLOSING_QUOT:
                if(current_char=='"')
                {
                    characters_read++;
                    state = STATE_STRING_END;
                    break;
                }
                else
                {
                    characters_read++;
                    state = STATE_MULTIPLE_STRING;
                    break;
                }
                
            case STATE_SINGLE_STRING:
                if(current_char == '"')
                {
                    characters_read++;
                    state = STATE_STRING_END;
                    break;
                }
                else if(current_char == '\\')
                {
                    characters_read++;
                    state = STATE_SLASH;
                    break;
                }
                else
                {
                    characters_read++;
                    continue;
                }

            case STATE_SLASH:
                characters_read++;
                state = STATE_SINGLE_STRING;
                break;
                
            case STATE_STRING_END:
                lexer_unconsume_char(lexer, file, current_char);
                set_multi_token(lexer, TOKEN_STRING, file, characters_read);
                return *lexer->current_token;
               
/////////////////////////////////////////////



            case STATE_PLUS:
                set_single_token(lexer, TOKEN_PLUS, current_char);
                return *lexer->current_token;
            case STATE_MINUS:
                set_single_token(lexer, TOKEN_MINUS, current_char);
                return *lexer->current_token;
            case STATE_MULTIPLY:
                set_single_token(lexer, TOKEN_MULTIPLY, current_char);
                return *lexer->current_token;
            case STATE_NOT_EQUAL:
                lexer_consume_char(lexer, file);
                set_multi_token(lexer, TOKEN_NOT_EQUAL, file, strlen("!="));
                return *lexer->current_token;
            case STATE_ASSIGN:
                if (peek_char(file) == '=') {
                    change_state(file, lexer, &state, STATE_EQUAL, current_char);
                    lexer_consume_char(lexer, file);
                    break;
                }
                set_single_token(lexer, TOKEN_ASSIGN, current_char);
                return *lexer->current_token;
            case STATE_EQUAL:
                set_multi_token(lexer, TOKEN_EQUAL, file, strlen("=="));
                return *lexer->current_token;
            case STATE_LESS:
                if (peek_char(file) == '=') {
                    change_state(file, lexer, &state, STATE_EQUAL_LESS, current_char);
                    lexer_consume_char(lexer, file);
                    break;
                }
                set_single_token(lexer, TOKEN_LESS, current_char);
                return *lexer->current_token;
            case STATE_EQUAL_LESS:
                set_multi_token(lexer, TOKEN_EQUAL_LESS, file, strlen("<="));
                return *lexer->current_token;
            case STATE_GREATER:
                if (peek_char(file) == '=') {
                    change_state(file, lexer, &state, STATE_EQUAL_GREATER, current_char);
                    lexer_consume_char(lexer, file);
                    break;
                }
                set_single_token(lexer, TOKEN_GREATER, current_char);
                return *lexer->current_token;
            case STATE_EQUAL_GREATER:
                set_multi_token(lexer, TOKEN_EQUAL_GREATER, file, strlen(">="));
                return *lexer->current_token;
            case STATE_OPEN_BRACE:
                set_single_token(lexer, TOKEN_OPEN_BRACE, current_char);
                return *lexer->current_token;
            case STATE_CLOSE_BRACE:
                set_single_token(lexer, TOKEN_CLOSE_BRACE, current_char);
                return *lexer->current_token;
            case STATE_OPEN_PAREN:
                set_single_token(lexer, TOKEN_OPEN_PAREN, current_char);
                return *lexer->current_token;
            case STATE_CLOSE_PAREN:
                set_single_token(lexer, TOKEN_CLOSE_PAREN, current_char);
                return *lexer->current_token;
            case STATE_EOF:
                // Если переход был из однострочного комментария
                if (find_eol)
                    set_single_token(lexer, TOKEN_EOL, EOF);
                else 
                    set_single_token(lexer, TOKEN_EOF, EOF);
                return *lexer->current_token;
            default:
                // Ошибка: неизвестное состояние
                lexer_error(lexer, LEXER_ERROR, "Unknown state in lexer FSM");
                break;
        }
    }
    // Этот код никогда не будет достигнут
    lexer_error(lexer, LEXER_ERROR, "Lexer exited unexpectedly");
    
}

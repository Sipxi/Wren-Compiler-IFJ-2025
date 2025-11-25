/**
 * @file lexer.c
 *
 * @brief Implementace lexikálního analyzátoru.
 *
 * Autor:
 *     - Serhij Čepil (253038)
 *     - Dmytro Kravchenko (273125)
 *     - Veronika Turbaievska (273123)
 */

 //! Допишите ваши имена и номера
#include "lexer.h"
#include "error_codes.h"
#include "utils.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>


/* ======================================*/
/* ===== FSM (Finite State Machine) =====*/
/* ======================================*/

typedef enum {
    // Identifikátory
    STATE_IDENTIFIER,           // Identifikátor (proměnná, funkce, atd.)
    STATE_ONE_UNDERSCORE,       // _                             * Přechodový stav po jednom _
    STATE_TWO_UNDERSCORE,       // __                            * Přechodový stav po dvou __
    STATE_GLOBAL_IDENTIFIER,    // Globální identifikátor

    // Kulaté závorky a složené závorky
    STATE_OPEN_PAREN,           // { 
    STATE_CLOSE_PAREN,          // }
    STATE_OPEN_BRACE,           // (
    STATE_CLOSE_BRACE,          // )

    // Operátory
    STATE_MINUS,                // -
    STATE_PLUS,                 // +
    STATE_MULTIPLY,             // *
    STATE_DIVISION,             // /
    STATE_ASSIGN,               // =
    
    // Porovnávací operátory
    STATE_EXCLAMATION,          // ! *                             * Přechodový stav do !=
    STATE_NOT_EQUAL,            // !=
    STATE_EQUAL,                // ==
    STATE_LESS,                 // <
    STATE_EQUAL_LESS,           // <=
    STATE_GREATER,              // >
    STATE_EQUAL_GREATER,        // >=

    // Ostatní symboly
    STATE_COMMA,                // ,
    STATE_DOT,                  // .
    
    // Čisla
    STATE_NUMBER,               // Číslo
    STATE_ZERO_START,           // 0   
    STATE_DECIMAL_POINT,        //                              * Přechodový stav po desetinné tečce 1.
    STATE_FLOAT_NUMBER,         // Číslo s plovoucí desetinnou čárkou 2.5, 0.75    
    STATE_HEX_PREFIX,           //                              * Přechodový stav po 0x
    STATE_HEX_NUMBER,           // Šestnáctkové číslo (0x...)  
    STATE_CHECK_EXPONENT,       //                              * Přechodový stav pro kontrolu exponentu 2e
    STATE_SIGN_EXP_NUMBER,      //                              * Přechodový stav pro znak v exponentu +-
    STATE_EXP_NUMBER,           // Číslo v exponenciálním formátu 1.5e10, 2.3E-4

    // Řetězce
    STATE_FIRST_QUOT,          // "                             * Přechodový stav po první uvozovce
    STATE_SINGLE_STRING,       // "example...                   * Přechodový stav pro jednoduchý řetězec
    STATE_SLASH,               // \                             * Přechodový stav pro escape sekvenci
    STATE_STRING_END,          // "example"| """example"""      * Přechodový stav pro konec řetězce

    // Viceřádkové řetězce
    STATE_SECOND_QUOT,         // ""                            * Přechodový stav po druhé uvozovce
    STATE_MULTIPLE_STRING,     // """example..                  * Přechodový stav pro víceřádkový řetězec
    STATE_CLOSING_QUOT,        // """example"                   * Přechodový stav pro zavírací uvozovku
    STATE_SECOND_CLOSING_QUOT, // """example""                  * Přechodový stav pro druhou zavírací uvozovku

    // Komentáře
    STATE_COMMENT,              // Komentář na jeden řádek
    STATE_START_BLOCK_COMMENT,  // Začátek blokového komentáře  * Přechodový stav
    STATE_BODY_BLOCK_COMMENT,   // Tělo blokového komentáře     * Přechodový stav
    STATE_END_BLOCK_COMMENT,    // Konec blokového komentáře 

    // Speciální stavy
    STATE_START,               // Startovní stav
    STATE_EOF,                 // Konec souboru
    STATE_EOL                  // Konec řádku

} LexerFSMState;

/* ======================================*/
/* ===== Deklarace privátních funkcí lexeru =====*/
/* ======================================*/

/**
 * @brief Zpracovává escape sekvenci v řetězci.
 * 
 * @param file Ukazatel na soubor obsahující zdrojový kód.
 * @param chars_read Počet znaků přečtených v rámci řetězce.
 * @param limit_len Maximální délka řetězce (bez uvozovek).
 * @return Znak odpovídající escape sekvenci.
 */
static char process_escape_sequence(FILE *file, int *chars_read, int limit_len);

/**
 * @brief Zapiše do buffer lexéru další token ze zdrojového kódu.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file  Ukazatel na soubor obsahující zdrojový kód.
 */
static void scan_token(Lexer *lexer, FILE *file);

/**
 * @brief Kontroluje, zda je znak platným znakem pro identifikátory (písmena
 * a číslice).
 *
 * @param character Znak ke kontrole.
 * @return true pokud je znak písmeno nebo číslice, jinak false.
 */
static bool is_letter(char character);

/**
 * @brief Kontroluje, zda aktuální pozice v souboru začíná komentářem.
 * 
 * @param file Ukazatel na soubor pro kontrolu.
 * @return -1 pokud to není komentář, 0 pokud je to jednřádkový komentář,
 *         1 pokud je to víceřádkový komentář
 */
static int is_comment_start(char current_char, FILE *file);

/**
 * @brief Kontroluje, zda je znak číslicí (0-9).
 *
 * @param character Znak ke kontrole.
 * @return true pokud je znak číslice, jinak false.
 */
static bool is_digit(char character);

/**
 * @brief Kontroluje, zda je aktuální identifikátor klíčovým slovem.
 *
 * Pokud ano, aktualizuje typ tokenu na odpovídající typ klíčového slova.
 * @param lexer Ukazatel na strukturu Lexer.
 * @return true pokud je aktuální identifikátor klíčovým slovem, jinak false.
 */
static bool is_keyword(const char *str);

/**
 * @brief Zpracovává konec blokového komentáře.
 *
 * Tato funkce čte znaky ze souboru
 * a zpracovává možné ukončení blokového komentáře.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file Ukazatel na soubor obsahující zdrojový kód.
 * @return true pokud byl nalezen konec blokového komentáře, jinak false.
 */
static bool is_end_block_comment(char current_char, FILE *file);

/**
 * @brief Kontroluje, zda je znak bílým znakem (například mezera, tabulátor, nový
 * řádek).
 *
 * @param character Znak ke kontrole.
 * @return true pokud je znak bílý znak, jinak false.
 */
static bool is_whitespace(const char character);

/**
 * @brief Kontroluje, zda je znak šestnáctkovou číslicí (0-9, a-f, A-F).
 *
 * @param character Znak ke kontrole.
 * @return true pokud je znak šestnáctkovou číslicí, jinak false.
 */
static bool is_hex_digit(const char character);

/**
 * @brief Zapíše zadaný počet znaků ze souboru do řetězce.
 *
 * Tato funkce čte znaky ze souboru a zapisuje je do zadaného řetězce.
 * @param file Ukazatel na soubor, ze kterého budou čteny znaky.
 * @param count Počet znaků k přečtení.
 * @param str Řetězec pro zápis.
 * @return Počet zapsaných znaků.
 */
static void write_str(FILE *file, int count, char **str);

/**
 * @brief Prohlíží další znak v souboru bez jeho odstranění z proudu.
 *
 * @param file Ukazatel na soubor pro prohlížení dalšího znaku.
 * @return Další znak v souboru.
 */
static char peek_char(FILE *file);

/**
 * @brief Prohlíží znak po dalším v souboru bez jeho odstranění z proudu.
 *
 * @param file Ukazatel na soubor pro prohlížení znaku po dalším.
 * @return Znak po dalším v souboru.
 */
static char peek_next_char(FILE *file);

/**
 * @brief Čte další znak ze souboru a aktualizuje pozici lexéru.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file Ukazatel na soubor pro čtení dalšího znaku.
 * @return Přečtený znak.
 */
static char lexer_consume_char(Lexer *lexer, FILE *file);

/**
 * @brief Vrací poslední přečtený znak zpět do proudu a aktualizuje pozici lexéru.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file Ukazatel na soubor pro vrácení znaku.
 */
static void lexer_unconsume_char(Lexer *lexer, FILE *file, char current_char);

/**
 * @brief Nastavuje token s daným typem a daty.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param type Typ tokenu pro nastavení.
 * @param data Data tokenu pro nastavení (jeden znak).
 */
static void set_single_token(Lexer *lexer, TokenType type, const char data);

/**
 * @brief Nastavuje token s daným typem a daty.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param type Typ tokenu pro nastavení.
 * @param file Ukazatel na soubor obsahující zdrojový kód.
 * @param characters_read Počet znaků přečtených pro token.
 */
static void set_multi_token(Lexer *lexer, TokenType type, FILE *file,
    int characters_read);

/**
 * @brief Čte číslo (celé, s plovoucí desetinnou čárkou, exponenciální) ze zdrojového kódu.
 *
 * Tato funkce čte znaky ze souboru pro vytvoření tokenu čísla.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file Ukazatel na soubor obsahující zdrojový kód.
 */
static void read_number(Lexer *lexer, FILE *file, char current_char);

/**
 * @brief Klasifikuje číslo začínající na '0' jako celé, s plovoucí desetinnou čárkou,
 * exponenciální nebo šestnáctkové.
 *
 * Tato funkce čte znaky ze souboru pro vytvoření tokenu čísla.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file Ukazatel na soubor obsahující zdrojový kód.
 */
static void classify_number_token(Lexer *lexer, FILE *file, char current_char);

/**
 * @brief Mění stav konečného automatu lexéru a vrací poslední
 * přečtený znak zpět do proudu.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file Ukazatel na soubor obsahující zdrojový kód.
 * @param current_state Ukazatel na aktuální stav konečného automatu
 * lexéru.
 * @param next_state Následující stav konečného automatu lexéru.
 * @param current_char Aktuální znak, který již byl přečten.
 */
static void change_state(FILE *file, Lexer *lexer, LexerFSMState *current_state,
    LexerFSMState next_state, char current_char);

/**
 * @brief Zajišťuje, že v bufferu lexéru je požadovaný počet tokenů.
 *
 * Pokud je v bufferu méně tokenů, než je potřeba, funkce načte další tokeny
 * ze souboru a přidá je do bufferu.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param needed_count Počet tokenů, které musí být v bufferu.
 * @param file Ukazatel na soubor obsahující zdrojový kód.
 */
static void ensure_buffer_has(Lexer *lexer, int needed_count, FILE *file);

/**
 * @brief Posune buffer tokenů lexéru, odstraní první token.
 * Po posunu se počet tokenů v bufferu sníží o jeden.
 * @param lexer Ukazatel na strukturu Lexer
 */
static void shift_buffer(Lexer *lexer);

/**
 * @brief Nastavuje token řetězce s oříznutým obsahem mezi uvozovkami.
 *
 * Tato funkce čte znaky ze souboru a nastavuje token řetězce
 * pouze s obsahem mezi uvozovkami.
 *
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file Ukazatel na soubor obsahující zdrojový kód.
 * @param total_chars Celkový počet znaků přečtených pro token (včetně uvozovek).
 * @param quote_len Délka uvozovek (1 pro jednoduché, 3 pro víceřádkové).
 */
static void set_string_token_trimmed(Lexer *lexer, FILE *file, int total_chars, int quote_len);

/**
 * @brief Zapiše do char řetězce obsah mezi uvozovkami, oříznutý o uvozovky.
 * 
 * @note Tato funkce také zpracovává escape sekvence v rámci řetězce.
 * 
 * @param lexer Ukazatel na strukturu Lexer.
 * @param file  Ukazatel na soubor obsahující zdrojový kód.
 */
static void write_str_trimmed(FILE *file, int total_chars, int quote_len, char **str);

/* ====================================*/
/* ===== Implementace privátních funkcí lexéru =====*/
/* ====================================*/

static bool is_end_block_comment(char current_char, FILE *file) {
    // Kontrola, zda aktuální a následující znak tvoří konec blokového komentáře */
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
    const char *keywords[] = { "class",  "if",  "else",  "is",     "null",
                              "return", "var", "while", "Ifj",    "static",
                              "import", "for", "Num",   "String", "Null" };

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

static char process_escape_sequence(FILE *file, int *chars_read, int limit_len) {
    // Již jsme přečetli '\', nyní čteme další znak
    int escape_char = fgetc(file);
    (*chars_read)++;

    switch (escape_char) {
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case '\\': return '\\';
        case '"': return '\"';
        case 'x': {
            // Hexadecimální escape sekvence \xAA
            char hex_buf[3] = {0};

            // 1. Ověření, zda máme dostatek znaků do limitu
            if (*chars_read + 2 > limit_len) {
                // Toto nastane spíše na konci souboru/řádku
                exit(LEXER_ERROR); // Lexikální chyba
            }

            int h1 = fgetc(file);
            int h2 = fgetc(file);
            (*chars_read) += 2;

            // Ověření, zda jsou to skutečně hexadecimální číslice 
            if (!isxdigit(h1) || !isxdigit(h2)) {
                // Pokud to nejsou hex čísla (např. \xZZ), je to lexikální chyba
                exit(LEXER_ERROR); // Lexikální chyba
            }

            hex_buf[0] = (char)h1;
            hex_buf[1] = (char)h2;

            // Převod na číslo - vrátíme jako int (0-255), abychom se vyhnuli znaménku
            return (int)strtol(hex_buf, NULL, 16);
        }
        default:
            // Neznámá escape sekvence
            exit(LEXER_ERROR); // Lexikální chyba
    }
}

static void write_str(FILE *file, int count, char **str) {
    // Přesunout ukazatel souboru zpět na poslední přečtenou
    // posloupnost znaků
    if (fseek(file, -count, SEEK_CUR) != 0) {
        lexer_error(NULL, INTERNAL_ERROR,
            "Failed to seek back in file for token data");
    }

    // Vyhradit nebo přerozdělit paměť pro řetězec (+1 pro nulový
    // terminátor)
    char *temp = realloc(*str, count + 1);
    if (temp == NULL) {
        lexer_error(NULL, INTERNAL_ERROR,
            "Failed to allocate memory for token data");
    }
    *str = temp;

    // Naplnit novou paměť znaky ze souboru
    for (int i = 0; i < count; i++) {
        int c = fgetc(file);
        if (c == EOF) {
            (*str)[i] = '\0';
            return;
        }
        (*str)[i] = (char)c;
    }

    // Nulový terminátor
    (*str)[count] = '\0';
}

static void write_str_trimmed(FILE *file, int total_chars, int quote_len, char **str) {
    // Seek zpět na začátek řetězce
    if (fseek(file, -total_chars, SEEK_CUR) != 0) {
        lexer_error(NULL, INTERNAL_ERROR, "Failed to seek back in file");
    }
    if (fseek(file, quote_len, SEEK_CUR) != 0) {
        lexer_error(NULL, INTERNAL_ERROR, "Failed to skip opening quotes");
    }

    // Spočítat skutečnou délku obsahu a alokovat paměť
    int raw_content_len = total_chars - (2 * quote_len);
    
    char *temp = realloc(*str, raw_content_len + 1);
    if (temp == NULL) {
        lexer_error(NULL, INTERNAL_ERROR, "Failed to allocate memory");
    }
    *str = temp;

    // Čtení a zpracování
    int write_index = 0;
    int chars_read = 0;

    while (chars_read < raw_content_len) {
        int c = fgetc(file);
        chars_read++;

        if (c == EOF) break;
        //! Quick fix for non-printable characters
        if (quote_len != 3) {
            if (c <= 31){
                fprintf(stderr, "Lexer error: Invalid character in string literal\n");
                exit(LEXER_ERROR); // Nekorektní znak v řetězci
            }
        }

        // Ověření, zda jde o escape sekvenci
        if (quote_len == 1 && c == '\\') {
            // Předat adresu chars_read, aby pomocná funkce mohla inkrementovat
            // pokud spotřebuje další znaky (například v \xAA)
            (*str)[write_index++] = process_escape_sequence(file, &chars_read, raw_content_len);
        } 
        else {
            // Normální znak
            (*str)[write_index++] = (char)c;
        }
    }

    (*str)[write_index] = '\0'; // Nulový terminátor

    // Posunout ukazatel souboru za koncové uvozovky
    fseek(file, quote_len, SEEK_CUR);
}

static void set_string_token_trimmed(Lexer *lexer, FILE *file, int total_chars, int quote_len) {
    lexer->current_token->type = TOKEN_STRING;
    lexer->current_token->line = lexer->line;
    write_str_trimmed(file, total_chars, quote_len, &lexer->current_token->data);
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
    // Zvýšit pozici lexeru
    // Přečíst další znak ze souboru
    lexer->position++;
    char character = fgetc(file);

    return character;
}

static void lexer_unconsume_char(Lexer *lexer, FILE *file, char current_char) {
    // Vrátit poslední přečtený znak zpět do proudu
    ungetc(current_char, file);
    // Snížit pozici lexeru
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

static void ensure_buffer_has(Lexer *lexer, int needed_count, FILE *file) {

    // Dokud je v bufferu méně tokenů, než potřebujeme
    while (lexer->buffered_count < needed_count &&
        lexer->buffered_count < TOKEN_BUFFER_SIZE) {
        // čteme nový token a přidáme ho na konec bufferu
        Token to_add;
        scan_token(lexer, file);
        token_copy_data(&to_add, lexer->current_token);
        lexer->buffered_tokens[lexer->buffered_count] = to_add;
        lexer->buffered_count++;
    }
}

static void shift_buffer(Lexer *lexer) {
    // Uvolnit data prvního tokenu (který je odstraňován)
    if (lexer->buffered_count > 0) {
        free(lexer->buffered_tokens[0].data);
    }
    // Posunout ostatní tokeny
    for (int i = 1; i < lexer->buffered_count; i++) {
        lexer->buffered_tokens[i - 1] = lexer->buffered_tokens[i];
    }
    lexer->buffered_count--;
}

/* ==================================== */
/* ===== Implementace veřejných funkcí lexeru =====*/
/* ==================================== */

Lexer *lexer_init() {
    // Alokovat paměť pro strukturu Lexer
    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
    if (lexer == NULL) {
        lexer_error(NULL, INTERNAL_ERROR,
            "Failed to allocate memory for Lexer");
    }
    
    // Inicializovat pozici a číslo řádku
    // Začít pozici od 1 a číslo řádku od 1
    lexer->position = 1;
    lexer->line = 1;
    lexer->current_token = token_init();
    lexer->buffered_count = 0;
    return lexer;
}

void lexer_free(Lexer *lexer) {
    if (lexer == NULL) {
        return;
    }
    // Uvolnit paměť pro aktuální token
    token_free(lexer->current_token);
    // Uvolnit paměť pro všechny tokeny v bufferu
    for (int i = 0; i < lexer->buffered_count; i++) {
        free(lexer->buffered_tokens[i].data);
    }
    // Uvolnit paměť pro strukturu Lexer
    free(lexer);
}

void lexer_error(Lexer *lexer, int error_code, const char *message) {
    fprintf(stderr,
        "\033[1;31mLexical error.\nError code: %d\n%s at line %d, position "
        "%d\033[0m\n",
        error_code, message, lexer->line, lexer->position);
    // Uvolnit paměť a ukončit program s kódem chyby
    lexer_free(lexer);
    exit(error_code);
}

Token peek_token(Lexer *lexer, FILE *file) {
    // Potřebuji 1 token
    ensure_buffer_has(lexer, 1, file);
    return lexer->buffered_tokens[0];
}

Token peek_next_token(Lexer *lexer, FILE *file) {
    // Potřebuji 2 tokeny
    ensure_buffer_has(lexer, 2, file);
    return lexer->buffered_tokens[1];
}

void get_token(Lexer *lexer, FILE *file) {
    if (lexer->buffered_count > 0) {
        shift_buffer(lexer);
    }
    else {
        // Buffer je prázdný. Prostě skenujeme a vracíme kopii
        scan_token(lexer, file);
    }
}

void scan_token(Lexer *lexer, FILE *file) {
    // FSM realizace lexeru
    LexerFSMState state = STATE_START;
    bool find_eol = false;
    int count_block_comment = 0;
    // Počet přečtených znaků pro aktuální token
    int characters_read = 0;
    int quote_len = 0;

    while (true) {
        // Čtení aktuálního znaku a aktualizace pozice lexeru
        char current_char = lexer_consume_char(lexer, file);

        // Hlavní přepínač stavů
        switch (state) {
            // =========================
            // ===== Startovní stav =====
            // =========================
            case STATE_START:
                // ===== Ignorování bílých znaků =====
                if (is_whitespace(current_char)) {
                    break;
                }
                // ===== Přechod na identifikátor =====
                else if (is_letter(current_char)) {
                    change_state(file, lexer, &state, STATE_IDENTIFIER,
                        current_char);
                    break;
                }
                // ===== Přechod na globální proměnnou =====
                else if (current_char == '_') {
                    characters_read++;
                    change_state(file, lexer, &state, STATE_ONE_UNDERSCORE,
                        current_char);
                    break;
                }
                // ===== Přechod na kulaté a složené závorky =====
                else if (current_char == '{') {
                    change_state(file, lexer, &state, STATE_OPEN_BRACE,
                        current_char);
                    break;
                }
                else if (current_char == '}') {
                    change_state(file, lexer, &state, STATE_CLOSE_BRACE,
                        current_char);
                    break;
                }
                else if (current_char == '(') {
                    change_state(file, lexer, &state, STATE_OPEN_PAREN,
                        current_char);
                    break;
                }
                else if (current_char == ')') {
                    change_state(file, lexer, &state, STATE_CLOSE_PAREN,
                        current_char);
                    break;
                }
                // ===== Přechod na operátory =====
                else if (current_char == '+') {
                    change_state(file, lexer, &state, STATE_PLUS, current_char);
                    break;
                }
                else if (current_char == '-') {
                    change_state(file, lexer, &state, STATE_MINUS,
                        current_char);
                    break;
                }
                else if (current_char == '*') {
                    change_state(file, lexer, &state, STATE_MULTIPLY,
                        current_char);
                    break;
                }
                else if (current_char == '/') {
                    change_state(file, lexer, &state, STATE_DIVISION,
                        current_char);
                    break;
                }
                else if (current_char == '=') {
                    change_state(file, lexer, &state, STATE_ASSIGN,
                        current_char);
                    break;
                }
                else if (current_char == '<') {
                    change_state(file, lexer, &state, STATE_LESS, current_char);
                    break;
                }
                else if (current_char == '>') {
                    change_state(file, lexer, &state, STATE_GREATER,
                        current_char);
                    break;
                }
                else if (current_char == '!') {
                    change_state(file, lexer, &state, STATE_EXCLAMATION,
                        current_char);
                    break;
                }
                // ===== Přechod na ostatní symboly =====
                else if (current_char == '.') {
                    change_state(file, lexer, &state, STATE_DOT, current_char);
                    break;
                }
                else if (current_char == ',') {
                    change_state(file, lexer, &state, STATE_COMMA, current_char);
                    break;
                }
                // ===== Přechod na konec souboru =====
                else if (current_char == EOF) {
                    change_state(file, lexer, &state, STATE_EOF, current_char);
                    break;
                }
                // ===== Přechod na čísla začínající na 0 =====
                else if (current_char == '0') {
                    change_state(file, lexer, &state, STATE_ZERO_START,
                        current_char);
                    break;
                }
                // ===== Přechod na čísla začínající na 1-9 =====
                else if (is_digit(current_char)) {
                    change_state(file, lexer, &state, STATE_NUMBER,
                        current_char);
                    break;
                }
                // ===== Přechod na řetězce =====
                else if (current_char == '"') {
                    state = STATE_FIRST_QUOT;
                    characters_read++;
                    break;
                }
                // ===== Přechod na konec řádku =====
                else if (current_char == '\n') {
                    change_state(file, lexer, &state, STATE_EOL, current_char);
                    break;
                }
                // ===== Neznámý znak - chyba =====
                else {
                    lexer_error(lexer, LEXER_ERROR,
                        "Unknown character encountered");
                    break;
                }
            
            // =========================
            // ===== Stavy identifikátorů =====
            // =========================
            case STATE_IDENTIFIER:
                // Je-li další znak písmeno nebo číslo, pokračujeme ve čtení
                if (is_letter(current_char) || (current_char == '_') ||
                    is_digit(current_char)) {
                    // Čteme dál
                    characters_read++;
                    break;
                }
                // Poslední přečtený znak nepatří do identifikátoru,
                // vrátíme ho zpět do proudu
                lexer_unconsume_char(lexer, file, current_char);
                set_multi_token(lexer, TOKEN_IDENTIFIER, file, characters_read);

                // Kontrola, zda je identifikátor klíčovým slovem
                if (is_keyword(lexer->current_token->data)) {
                    lexer->current_token->type = TOKEN_KEYWORD;
                }

                return;
            // ===== Stavy globálních identifikátorů =====
            case STATE_ONE_UNDERSCORE:
                // Při přechodu čteme znak
                current_char = lexer_consume_char(lexer, file);
                if (current_char == '_') {
                    // Pokud je druhý znak také '_', přecházíme do stavu s
                    // dvěma '_'
                    change_state(file, lexer, &state, STATE_TWO_UNDERSCORE,
                        current_char);
                }
                else {
                    lexer_error(lexer, LEXER_ERROR,
                        "Invalid global identifier");
                }
                break;

            case STATE_TWO_UNDERSCORE:
                // Při přechodu čteme znak
                current_char = lexer_consume_char(lexer, file);
                // Kontrolujeme, zda je další znak písmeno
                if (is_letter(current_char)) {
                    characters_read++;
                    change_state(file, lexer, &state, STATE_GLOBAL_IDENTIFIER,
                        current_char);
                    break;
                }
                // Chyba: neplatný formát globálního identifikátoru
                lexer_error(lexer, LEXER_ERROR,
                        "Invalid global identifier");
                break;

            case STATE_GLOBAL_IDENTIFIER:
                if (is_letter(current_char) || (current_char == '_') ||
                    is_digit(current_char)) {
                    // Čteme dál
                    characters_read++;
                    continue;
                }
                // Poslední přečtený znak nepatří do identifikátoru,
                // vrátíme ho zpět do proudu
                lexer_unconsume_char(lexer, file, current_char);
                set_multi_token(lexer, TOKEN_GLOBAL_IDENTIFIER, file,
                    characters_read);

                return;

            // =========================
            // ===== Stavy kulatých a složených závorek =====
            // =========================
            case STATE_OPEN_BRACE:
                set_single_token(lexer, TOKEN_OPEN_BRACE, current_char);
                return;
            case STATE_CLOSE_BRACE:
                set_single_token(lexer, TOKEN_CLOSE_BRACE, current_char);
                return;
            case STATE_OPEN_PAREN:
                set_single_token(lexer, TOKEN_OPEN_PAREN, current_char);
                return;
            case STATE_CLOSE_PAREN:
                set_single_token(lexer, TOKEN_CLOSE_PAREN, current_char);
                return;
                
            // =========================
            // ===== Stavy operátorů =====
            // =========================
            case STATE_DIVISION:
                // Je-li další znak /, je to začátek jednořádkového komentáře
                if (peek_char(file) == '/') {
                    change_state(file, lexer, &state, STATE_COMMENT,
                        current_char);
                    break;
                }
                // Je-li další znak *, je to začátek blokového komentáře
                else if (peek_char(file) == '*') {
                    change_state(file, lexer, &state, STATE_START_BLOCK_COMMENT,
                        current_char);
                    break;
                    lexer_consume_char(lexer, file);
                }
                // Je to prostě operátor dělení
                set_single_token(lexer, TOKEN_DIVISION, current_char);
                return;
            case STATE_PLUS:
                set_single_token(lexer, TOKEN_PLUS, current_char);
                return;
            case STATE_MINUS:
                set_single_token(lexer, TOKEN_MINUS, current_char);
                return;
            case STATE_MULTIPLY:
                set_single_token(lexer, TOKEN_MULTIPLY, current_char);
                return;
            case STATE_EXCLAMATION:
                if(peek_char(file) == '=') {
                    change_state(file, lexer, &state, STATE_NOT_EQUAL,
                        current_char);
                    lexer_consume_char(lexer, file);
                    break;
                }
                // Chyba: neočekávaný znak po '!'
                lexer_error(lexer, LEXER_ERROR,
                        "Unexpected character after '!'");
                break;
            case STATE_NOT_EQUAL:
                set_multi_token(lexer, TOKEN_NOT_EQUAL, file, strlen("!="));
                return;
            case STATE_ASSIGN:
                if (peek_char(file) == '=') {
                    change_state(file, lexer, &state, STATE_EQUAL,
                        current_char);
                    lexer_consume_char(lexer, file);
                    break;
                }
                set_single_token(lexer, TOKEN_ASSIGN, current_char);
                return;
            case STATE_EQUAL:
                set_multi_token(lexer, TOKEN_EQUAL, file, strlen("=="));
                return;
            case STATE_LESS:
                if (peek_char(file) == '=') {
                    change_state(file, lexer, &state, STATE_EQUAL_LESS,
                        current_char);
                    lexer_consume_char(lexer, file);
                    break;
                }
                set_single_token(lexer, TOKEN_LESS, current_char);
                return;
            case STATE_EQUAL_LESS:
                set_multi_token(lexer, TOKEN_EQUAL_LESS, file, strlen("<="));
                return;
            case STATE_GREATER:
                if (peek_char(file) == '=') {
                    change_state(file, lexer, &state, STATE_EQUAL_GREATER,
                        current_char);
                    lexer_consume_char(lexer, file);
                    break;
                }
                set_single_token(lexer, TOKEN_GREATER, current_char);
                return;
            case STATE_EQUAL_GREATER:
                set_multi_token(lexer, TOKEN_EQUAL_GREATER, file, strlen(">="));
                return;

            // =========================
            // ===== Stavy ostatních symbolů ======
            // =========================
            case STATE_COMMA:
                set_single_token(lexer, TOKEN_COMMA, current_char);
                return;
            case STATE_DOT:
                set_single_token(lexer, TOKEN_DOT, current_char);
                return;
            // =========================
            // ===== Stavy čísel =====
            // =========================
            case STATE_ZERO_START:
                // První znak je '0' - zpracujeme další znaky
                characters_read++;
                // Následující znak určuje typ čísla
                char next_char = peek_char(file);
                if (next_char == 'x') {
                    // Zpracování šestnáctkových čísel
                    lexer_consume_char(lexer, file); // spotřebovat 'x'
                    characters_read++;
                    change_state(file, lexer, &state, STATE_HEX_PREFIX,
                        current_char);
                    break;
                }
                else if (next_char == '.') {
                    // Zpracování čísel s desetinnou tečkou
                    characters_read++;
                    lexer_consume_char(lexer, file); // spotřebovat '.'
                    change_state(file, lexer, &state, STATE_DECIMAL_POINT,
                        current_char);
                    break;
                }
                else if (next_char == 'e' || next_char == 'E') {
                    // Zpracování čísel v exponenciálním tvaru
                    characters_read++;
                    lexer_consume_char(lexer, file); // spotřebovat 'e' nebo 'E'
                    change_state(file, lexer, &state, STATE_CHECK_EXPONENT,
                        current_char);
                    break;
                }
                else if (is_digit(next_char)) {
                    //! DODELAT
                    // Ukazuje se, že 0456 je normální číslo ve Wren, vypíše
                    // 456 Proto prostě odstraníme 0 a pokračujeme ve čtení jako běžné
                    // číslo Snižujeme čítač, protože '0' se v čísle nepočítá
                    // characters_read--;

                    // lexer_consume_char(lexer, file); // spotřebovat další číslici
                    // change_state(file, lexer, &state, STATE_NUMBER,
                    //     current_char);
                    lexer_error(lexer, LEXER_ERROR,
                        "Invalid number format: leading zeros are not allowed");
                    break;
                }

                // To je prostě '0'
                set_single_token(lexer, TOKEN_INT, '0');
                return;

            case STATE_NUMBER:
                // Čtení čísla začínajícího na 1-9
                if (is_digit(current_char)) {
                    // Pokračovat ve čtení čísla
                    characters_read++;
                    break;
                }
                // Zkontrolovat, zda začíná desetinná tečka 
                if (current_char == '.') {
                    // Ihned přidat k počtu znaků, po if
                    characters_read++;
                    change_state(file, lexer, &state, STATE_DECIMAL_POINT,
                        current_char);
                    break;
                }
                // Zkontrolovat, zda začíná exponent
                else if (current_char == 'e' || current_char == 'E') {
                    // Ihned přidat k počtu znaků, po if
                    characters_read++;
                    change_state(file, lexer, &state, STATE_CHECK_EXPONENT,
                        current_char);
                    break;
                }
                // Poslední přečtený znak nepatří k číslu, vrátit
                // ho zpět do proudu
                lexer_unconsume_char(lexer, file, current_char);
                // Nastavit typ tokenu na TOKEN_INT
                set_multi_token(lexer, TOKEN_INT, file, characters_read);
    
                return;

            case STATE_DECIMAL_POINT:
                // Čtení čísla s desetinnou tečkou
                current_char = lexer_consume_char(lexer, file);
                if (!is_digit(current_char)) {
                    lexer_error(lexer, SYNTAX_ERROR, "Invalid float format");
                }
                // Pokud je následující znak číslice, přejdeme do stavu
                // čtení čísla s plovoucí desetinnou čárkou
                change_state(file, lexer, &state, STATE_FLOAT_NUMBER,
                    current_char);
                break;

            case STATE_FLOAT_NUMBER:
                // Čtení čísla s plovoucí desetinnou čárkou
                if (is_digit(current_char)) {
                    // Pokračovat ve čtení čísla
                    characters_read++;
                    break;
                }
                // Zkontrolovat, zda začíná exponent
                if (current_char == 'e' || current_char == 'E') {
                    characters_read++;
                    change_state(file, lexer, &state, STATE_CHECK_EXPONENT,
                        current_char);
                    break;
                }
                // Poslední přečtený znak nepatří k číslu, vrátit
                // ho zpět do proudu
                lexer_unconsume_char(lexer, file, current_char);
                // Nastavit typ tokenu na TOKEN_FLOAT
                set_multi_token(lexer, TOKEN_FLOAT, file, characters_read);

                return;

            case STATE_HEX_PREFIX:
                // Čtení dalšího znaku po '0x'
                current_char = lexer_consume_char(lexer, file);
                // Zkontrolovat, zda je další znak šestnáctková číslice
                if (!is_hex_digit(current_char)) {
                    lexer_error(lexer, LEXER_ERROR,
                        "Invalid hexadecimal format");
                }
                change_state(file, lexer, &state, STATE_HEX_NUMBER,
                    current_char);
                break;
                
            case STATE_HEX_NUMBER:
                // Další znak bude čten na začátku celého cyklu
                if (is_hex_digit(current_char)) {
                    // Po přečtení šestnáctkové číslice
                    // zvýšíme počet přečtených znaků
                    characters_read++;
                    break;
                }
                // Poslední přečtený znak nepatří k šestnáctkové
                // číslici, vrátit ho zpět do proudu
                lexer_unconsume_char(lexer, file, current_char);
                // Nastavit typ tokenu na TOKEN_HEX
                set_multi_token(lexer, TOKEN_HEX, file, characters_read);
                return;

            case STATE_CHECK_EXPONENT:
                // Čtení dalšího znaku po 'e' nebo 'E'
                current_char = lexer_consume_char(lexer, file);
                if (current_char == '+' || current_char == '-') {
                    // Čtení znaménka exponentu
                    characters_read++;
                    change_state(file, lexer, &state, STATE_SIGN_EXP_NUMBER,
                        current_char);
                    break;
                }
                // Zkontrolovat, zda je následující znak číslice
                if (!is_digit(current_char)) {
                    lexer_error(lexer, LEXER_ERROR, "Invalid exponent format");
                }
                // Přejít do stavu čtení exponentu
                change_state(file, lexer, &state, STATE_EXP_NUMBER,
                    current_char);
                break;

            case STATE_SIGN_EXP_NUMBER:
                // Čtení dalšího znaku po '+' nebo '-'
                current_char = lexer_consume_char(lexer, file);
                // Zkontrolovat, zda je následující znak číslice
                if (!is_digit(current_char)) {
                    lexer_error(lexer, LEXER_ERROR, "Invalid exponent format");
                }
                // Přejít do stavu čtení exponentu
                change_state(file, lexer, &state, STATE_EXP_NUMBER,
                    current_char);
                break;

            case STATE_EXP_NUMBER:
                // Čtení dalšího znaku po 'e' nebo 'E'
                if (is_digit(current_char)) {
                    // Po přečtení číslice zvýšíme počet přečtených znaků
                    characters_read++;
                    continue;
                }
                // Poslední přečtený znak nepatří k číslici, vrátit ho zpět do proudu
                lexer_unconsume_char(lexer, file, current_char);
                // Nastavit typ tokenu na TOKEN_EXP
                set_multi_token(lexer, TOKEN_EXP, file, characters_read);
                return;
            // =========================
            // ==== Stavy řetězců =====
            // =========================
            case STATE_FIRST_QUOT:
                quote_len = 1;
                // Bud první znak po úvodní uvozovce je další uvozovka (prázdný řetězec)
                if (current_char == '"') {
                    state = STATE_SECOND_QUOT;
                    characters_read++;
                    break;
                }
                // Nebo další znak je konec řetězce nebo konec souboru
                // To je chyba
                else if (current_char == EOF || current_char == '\n') {
                    lexer_error(lexer, LEXER_ERROR,
                        "Unterminated string literal");
                    break;
                }
                // Nebo další znak jsou escape sekvence nebo běžné znaky
                else if (current_char == '\\') {
                    characters_read++;
                    state = STATE_SLASH;
                    break;
                }
                // Jinak pokračujeme ve čtení řetězce
                state = STATE_SINGLE_STRING;
                
                characters_read++;
                break;

            case STATE_SINGLE_STRING:
                // Pokud je další znak uvozovka, je to konec řetězce
                if (current_char == '"') {
                    characters_read++;
                    state = STATE_STRING_END;
                    break;
                }
                // Pokud je další znak konec řádku nebo souboru, je to chyba
                else if (current_char == EOF || current_char == '\n') {
                    lexer_error(lexer, LEXER_ERROR,
                        "Unterminated string literal");
                    break;
                }
                // Pokud je další znak zpětné lomítko, přecházíme do stavu escape sekvence
                else if (current_char == '\\') {
                    characters_read++;
                    state = STATE_SLASH;
                    break;
                }
                // Jinak pokračujeme ve čtení řetězce
                characters_read++;
                break;

            case STATE_SLASH:
                characters_read++;
                state = STATE_SINGLE_STRING;
                break;


            case STATE_SECOND_QUOT:
                if (current_char == EOF) {
                    state = STATE_STRING_END;
                    break;
                }
                // Pokud je další znak neni uvozovka, je to konec řetězce
                else if (current_char != '"') {
                    lexer_unconsume_char(lexer, file, current_char);
                    state = STATE_STRING_END;
                    break;
                }
                // Jinak je to další uvozovka, pokračujeme ve čtení řetězce
                quote_len = 3;
                characters_read++;
                state = STATE_MULTIPLE_STRING;
                break;

            case STATE_STRING_END:
                lexer_unconsume_char(lexer, file, current_char);
                set_string_token_trimmed(lexer, file, characters_read, quote_len);
                return;
            // =========================
            // ===== Stavy víceřádkové řetězce =====
            // =========================
            case STATE_MULTIPLE_STRING:
                if (current_char == '"') {
                    characters_read++;
                    state = STATE_CLOSING_QUOT;
                    break;
                }
                else if (current_char == EOF) {
                    lexer_error(lexer, LEXER_ERROR,
                        "Unterminated string literal");
                    break;
                }
                characters_read++;
                break;

            case STATE_CLOSING_QUOT:
                if (current_char == '"') {
                    characters_read++;
                    state = STATE_SECOND_CLOSING_QUOT;
                    break;
                }
                else if (current_char == EOF) {
                    lexer_error(lexer, LEXER_ERROR,
                        "Unterminated string literal");
                    break;
                }
                else {
                    characters_read++;
                    state = STATE_MULTIPLE_STRING;
                    break;
                }

            case STATE_SECOND_CLOSING_QUOT:
                if (current_char == '"') {
                    characters_read++;
                    state = STATE_STRING_END;
                    break;
                }
                else if (current_char == EOF) {
                    lexer_error(lexer, LEXER_ERROR,
                        "Unterminated string literal");
                    break;
                }
                else {
                    characters_read++;
                    state = STATE_MULTIPLE_STRING;
                    break;
                }

            // =========================
            // ===== Stavy komentářů =====
            // =========================
            case STATE_COMMENT:
                // Dokud nedojdeme na konec řádku, čteme komentář
                if (current_char == '\n')
                    change_state(file, lexer, &state, STATE_EOL, current_char);
                // nebo konec souboru
                else if (current_char == EOF) {
                    change_state(file, lexer, &state, STATE_EOF, current_char);
                }
                break;
            case STATE_START_BLOCK_COMMENT:
                // protože začátek komentáře jsou vždy 2 znaky,
                // musíme jeden přeskočit
                current_char = lexer_consume_char(lexer, file);
                // zvyšujeme čítač vnořených bloků
                count_block_comment++;  
                change_state(file, lexer, &state, STATE_BODY_BLOCK_COMMENT,
                    current_char);
                break;

            case STATE_BODY_BLOCK_COMMENT:
                if (current_char ==
                    '\n')  // sledujeme nové řádky uvnitř komentáře
                    lexer->line++;
                // Kontrolujeme, zda je další znak začátkem komentáře
                else if (is_comment_start(current_char, file) == 1) {
                    change_state(file, lexer, &state, STATE_START_BLOCK_COMMENT,
                        current_char);
                    lexer_consume_char(lexer, file);
                }
                else if (is_end_block_comment(current_char, file)) {
                    change_state(file, lexer, &state, STATE_END_BLOCK_COMMENT,
                        current_char);
                    lexer_consume_char(lexer, file);
                    // pokud není konec souboru, je to chyba
                }
                else if (current_char == EOF) {
                    lexer_error(lexer, LEXER_ERROR,
                        "Unterminated block comment");
                }
                // jinak zůstáváme v těle komentáře

                break;

            case STATE_END_BLOCK_COMMENT:
                // protože začátek komentáře jsou vždy 2 znaky,
                // musíme jeden přeskočit
                current_char = lexer_consume_char(lexer, file);
                count_block_comment--;
                // pokud před tím mimo komentář byl EOL, vracíme se do EOL
                if (count_block_comment == 0 && find_eol)
                    change_state(file, lexer, &state, STATE_EOL, current_char);
                // jinak do stavu START
                else if (count_block_comment == 0)
                    change_state(file, lexer, &state, STATE_START,
                        current_char);
                // pokud nejsou všechny bloky uzavřeny, zůstáváme v těle komentáře
                else
                    change_state(file, lexer, &state, STATE_BODY_BLOCK_COMMENT,
                        current_char);
                break;

            // =========================
            // ===== Stavy konce řádku a souboru =====
            // =========================
            case STATE_EOL:
                // nastavujeme příznak, že byl nalezen EOL
                find_eol = true;
                // sledujeme číslo řádku
                if (current_char == '\n') lexer->line++;
                // sledujeme komentáře po EOL
                else if (current_char == '/' && peek_char(file) == '/') {
                    change_state(file, lexer, &state, STATE_COMMENT,
                        current_char);
                    lexer_consume_char(lexer, file);
                }
                else if (current_char == EOF) {
                    change_state(file, lexer, &state, STATE_EOF, current_char);
                }
                else if (current_char == '/' && peek_char(file) == '*') {
                    change_state(file, lexer, &state, STATE_START_BLOCK_COMMENT,
                        current_char);
                    lexer_consume_char(lexer, file);
                }
                // pokud další znak není bílý znak,
                // vrátíme token EOL a znak ponecháme pro další token
                else if (!is_whitespace(current_char)) {
                    set_single_token(lexer, TOKEN_EOL, '\n');
                    lexer_unconsume_char(lexer, file, current_char);
                    return;
                }
                break;

            case STATE_EOF:
                set_single_token(lexer, TOKEN_EOF, EOF);
                return;
            default:
                // Chyba: neznámý stav
                lexer_error(lexer, LEXER_ERROR, "Unknown state in lexer FSM");
            }
        }
    // Pokud jsme někde v kódu vyskakovali z cyklu bez návratu,
    // je to neočekávané chování
    lexer_error(lexer, LEXER_ERROR, "Lexer exited unexpectedly");
}
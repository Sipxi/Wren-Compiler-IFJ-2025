#ifndef ERROR_CODES_H
#define ERROR_CODES_H

/*===== ANSI Коды для цветного вывода в терминал =====*/
// Можно использовать для "красивого" вывода ошибок
//? Оставить или убрать из финальной версии?
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_STYLE_BOLD    "\x1b[1m"
#define ANSI_STYLE_DIM     "\x1b[2m"


/*===== Коды ошибок =====*/
typedef enum {
    LEXER_ERROR = 1,
    SYNTAX_ERROR = 2,
    SEMANTIC_ERROR_UNDEFINED = 3,
    SEMANTIC_ERROR_REDECLARATION = 4,
    SEMANTIC_ERROR_ARGUMENT = 5,      
    SEMANTIC_ERROR_TYPE_MISMATCH = 6,
    SEMANTIC_ERROR_OTHER = 10,
    RUNTIME_ERROR_INVALID_TYPE = 25,
    RUNTIME_ERROR_TYPE_MISMATCH = 26,
    INTERNAL_ERROR = 99
} ErrorCode;

#endif 

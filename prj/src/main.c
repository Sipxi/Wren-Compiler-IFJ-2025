#include <stdio.h>

#include "lexer.h"
#include "parser.h"
/*
Основной файл который будет запускать сам проект пока что
Если хотите запустить этот файл:

make run
*/

int main(int argc, char *argv[]) {
    //TODO - нужно изменить с файла на stdin
    FILE *file = stdin;
    if (argc == 2) {
        file = fopen(argv[1], "r");
        if (!file) {
            fprintf(stderr, "Error opening file: %s\n", argv[1]);
            return 1;
        }
    }

    parser_run(file);
    
    return 0;
}
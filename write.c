#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    FILE *file;
    
    // Открываем файл для записи в папке prj
    file = fopen("prj/example.wren", "w");
    
    if (file == NULL) {
        printf("Ошибка: не удалось открыть файл prj/example.wren для записи\n");
        return 1;
    }
    
    // Записываем небольшую программу на Wren
    fprintf(file, "// Пример программы на языке Wren\n");
    fprintf(file, "var a = 0.2e+10\n");
    fprintf(file, "\n");
    fprintf(file, "\n");
    fprintf(file, "// Пример комментария\n");
    
    // Закрываем файл
    fclose(file);
    
    printf("Файл prj/example.wren успешно создан!\n");
    return 0;
}
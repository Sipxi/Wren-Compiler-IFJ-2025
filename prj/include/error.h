#ifndef ERROR_H
#define ERROR_H

#include "error_codes.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Вызывает ошибку и завершает программу.
 *
 * @param error_code Код ошибки.
 * @param line Номер строки, где произошла ошибка.
 * @param position Позиция в строке, где произошла ошибка.
 * @param message Сообщение об ошибке.
 */
void raise_error(int error_code, int line, int position, const char *message);



#endif
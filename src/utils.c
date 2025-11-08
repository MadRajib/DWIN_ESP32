#include "utils.h"
#include <stdarg.h>
#include <stdio.h>

void LOGF(const char *format, ...)
{
    char buffer[256];  // Adjust size depending on your needs
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
}

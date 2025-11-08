#ifndef _UTILS_H
#define _UTILS_H

#define FOR_L_N(itr, N) for (size_t itr=0; itr < N; ++itr)
#define FOR_LE_N(itr, N) for (size_t itr=0; itr <= N; ++itr)

#include <stdarg.h>

void LOGF(const char *format, ...);
#endif
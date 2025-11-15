#ifndef _UTILS_H
#define _UTILS_H

#include "cJSON.h"
#include <stdbool.h>

#define FOR_L_N(itr, N) for (size_t itr=0; itr < N; ++itr)
#define FOR_LE_N(itr, N) for (size_t itr=0; itr <= N; ++itr)

#ifndef BIT
#define BIT(x) (1U << (x))
#endif

float get_float(cJSON *obj, const char *key, float def);
bool get_bool(cJSON *obj, const char *key, bool def);
char* get_string(cJSON *obj, const char *key);
float get_array3(cJSON *arr, float out[3]);


#endif
#include "utils.h"
#include <string.h>

float get_float(cJSON *obj, const char *key, float def)
{
    cJSON *n = cJSON_GetObjectItem(obj, key);
    return cJSON_IsNumber(n) ? (float)n->valuedouble : def;
}

bool get_bool(cJSON *obj, const char *key, bool def)
{
    cJSON *b = cJSON_GetObjectItem(obj, key);
    return cJSON_IsBool(b) ? cJSON_IsTrue(b) : def;
}

char* get_string(cJSON *obj, const char *key)
{
    cJSON *s = cJSON_GetObjectItem(obj, key);
    return cJSON_IsString(s) ? strdup(s->valuestring) : NULL;
}

float get_array3(cJSON *arr, float out[3])
{
    if (!cJSON_IsArray(arr)) return 0;
    for (int i = 0; i < 3; i++) {
        cJSON *item = cJSON_GetArrayItem(arr, i);
        out[i] = cJSON_IsNumber(item) ? (float)item->valuedouble : 0.0f;
    }
    return 1;
}
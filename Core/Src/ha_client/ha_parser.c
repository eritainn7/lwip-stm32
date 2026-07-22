/**
 * @file    ha_parser.c
 * @brief   Простой парсер JSON для HomeAssistant API
 */

#include "ha_parser.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

/**
 * @brief  Извлечь значение по ключу из JSON строки
 */
int HA_Parser_GetValue(const char *json, const char *key, char *value, int max_len)
{
    if (!json || !key || !value || max_len <= 0) {
        return -1;
    }
    
    // Формируем строку поиска: "key":
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    // Ищем ключ в JSON
    const char *pos = strstr(json, search_key);
    if (!pos) {
        printf("HA_Parser: Key '%s' not found\r\n", key);
        return -1;
    }
    
    // Пропускаем ключ и двоеточие
    pos += strlen(search_key);
    
    // Пропускаем пробелы
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') {
        pos++;
    }
    
    if (*pos == '\0') {
        return -1;
    }
    
    // Определяем тип значения
    if (*pos == '"') {
        // Строковое значение
        pos++;
        int i = 0;
        while (*pos != '"' && *pos != '\0' && i < max_len - 1) {
            // Обработка escape-последовательностей
            if (*pos == '\\' && *(pos + 1) != '\0') {
                pos++;
                switch (*pos) {
                    case '"': value[i++] = '"'; break;
                    case '\\': value[i++] = '\\'; break;
                    case '/': value[i++] = '/'; break;
                    case 'n': value[i++] = '\n'; break;
                    case 't': value[i++] = '\t'; break;
                    default: value[i++] = *pos; break;
                }
            } else {
                value[i++] = *pos;
            }
            pos++;
        }
        value[i] = '\0';
    } else {
        // Числовое значение или boolean
        int i = 0;
        while ((*pos >= '0' && *pos <= '9') || *pos == '.' || *pos == '-' || *pos == '+') {
            if (i < max_len - 1) {
                value[i++] = *pos;
            }
            pos++;
        }
        value[i] = '\0';
        
        if (i == 0) {
            // Может быть true/false
            if (strncmp(pos, "true", 4) == 0) {
                strncpy(value, "1", max_len);
            } else if (strncmp(pos, "false", 5) == 0) {
                strncpy(value, "0", max_len);
            }
        }
    }
    
    return strlen(value);
}

/**
 * @brief  Извлечь числовое значение из JSON
 */
int HA_Parser_GetInt(const char *json, const char *key)
{
    char value[32];
    if (HA_Parser_GetValue(json, key, value, sizeof(value)) > 0) {
        return atoi(value);
    }
    return 0;
}

/**
 * @brief  Извлечь дробное значение из JSON
 */
float HA_Parser_GetFloat(const char *json, const char *key)
{
    char value[32];
    if (HA_Parser_GetValue(json, key, value, sizeof(value)) > 0) {
        return atof(value);
    }
    return 0.0f;
}

/**
 * @brief  Извлечь тело JSON из HTTP ответа
 */
const char* HA_Parser_ExtractBody(const char *http_response)
{
    if (!http_response) {
        return NULL;
    }
    
    // Ищем пустую строку после заголовков (разделитель \r\n\r\n)
    const char *body = strstr(http_response, "\r\n\r\n");
    if (body) {
        body += 4;  // Пропускаем \r\n\r\n
        return body;
    }
    
    // Fallback: пробуем найти { в начале JSON
    body = strchr(http_response, '{');
    if (body) {
        return body;
    }
    
    return NULL;
}
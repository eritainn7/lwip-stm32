/**
 * @file    ha_parser.h
 * @brief   Простой JSON парсер для ответов HomeAssistant API
 */

#ifndef __HA_PARSER_H__
#define __HA_PARSER_H__

#include <stdint.h>
#include <stddef.h>

/**
 * @brief  Извлечь значение по ключу из JSON строки
 * @param  json: строка JSON
 * @param  key: ключ для поиска
 * @param  value: буфер для значения
 * @param  max_len: размер буфера
 * @return длина значения или -1 при ошибке
 */
int HA_Parser_GetValue(const char *json, const char *key, char *value, int max_len);

/**
 * @brief  Извлечь числовое значение из JSON
 * @param  json: строка JSON
 * @param  key: ключ для поиска
 * @return числовое значение или 0 при ошибке
 */
int HA_Parser_GetInt(const char *json, const char *key);

/**
 * @brief  Извлечь дробное значение из JSON
 * @param  json: строка JSON
 * @param  key: ключ для поиска
 * @return значение типа float
 */
float HA_Parser_GetFloat(const char *json, const char *key);

/**
 * @brief  Извлечь тело JSON из HTTP ответа
 * @param  http_response: полный HTTP ответ
 * @return указатель на начало JSON или NULL
 */
const char* HA_Parser_ExtractBody(const char *http_response);

#endif /* __HA_PARSER_H__ */
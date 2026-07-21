#ifndef CGI_HANDLERS_H
#define CGI_HANDLERS_H

#include "lwip/apps/httpd.h"

// Прототипы функций-обработчиков
const char *hello_world_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

// Экспорт массива обработчиков
extern const tCGI cgi_handlers[];
extern const int cgi_handlers_count;

#endif /* CGI_HANDLERS_H */
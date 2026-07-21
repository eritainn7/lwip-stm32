#include "cgi_handlers.h"
#include "main.h"
#include <string.h>
#include <stdlib.h>
#include "lwip/apps/httpd.h"  

// Обработчик для главной страницы
const char *led_green_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    return NULL;
}

const char *led_blue_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
    return NULL;
}

const char *led_red_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
    return NULL;
}

// Массив обработчиков
const tCGI cgi_handlers[] = {
  {"/led_green.cgi", led_green_handler},
  {"/led_blue.cgi", led_blue_handler},
  {"/led_red.cgi", led_red_handler}
};

// Количество обработчиков
const int cgi_handlers_count = sizeof(cgi_handlers) / sizeof(cgi_handlers[0]);
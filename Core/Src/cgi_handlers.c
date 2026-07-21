#include "cgi_handlers.h"
#include "main.h"
#include <string.h>
#include <stdlib.h>

// Обработчик для главной страницы
const char *hello_world_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
      HAL_Delay(1000);
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);

  if (iIndex == 0)
  {
    return "Hello World!";
  }
  return NULL;
}

// Массив обработчиков
const tCGI cgi_handlers[] = {
  {"/hello_world", hello_world_handler},
};

// Количество обработчиков
const int cgi_handlers_count = sizeof(cgi_handlers) / sizeof(cgi_handlers[0]);
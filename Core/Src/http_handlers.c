/* USER CODE BEGIN Includes */
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"
#include "string.h"
#include "stdio.h"
#include "main.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
// Структура для SSI данных
typedef struct {
    char sensor_value[32];
    char device_status[32];
    char timestamp[32];
} SSI_Data_t;

static SSI_Data_t ssi_data;

// SSI теги
enum SSI_TAGS {
    SSI_SENSOR_VALUE = 0,
    SSI_DEVICE_STATUS,
    SSI_TIMESTAMP,
    SSI_COUNT
};

const char *ssi_tags[] = {
    "sensor",
    "status", 
    "time"
};

// Предварительное объявление CGI обработчиков (старый API)
static const char *cgi_control_handler(int iIndex, int iNumParams, 
                                        char *pcParam[], char *pcValue[]);
static const char *cgi_status_handler(int iIndex, int iNumParams, 
                                       char *pcParam[], char *pcValue[]);

// Структура CGI обработчиков (используем отдельные функции)
static const tCGI cgi_handlers[] = {
    {"/cgi/control", cgi_control_handler},
    {"/cgi/status", cgi_status_handler}
};

#define NUM_CGI_HANDLERS (sizeof(cgi_handlers) / sizeof(cgi_handlers[0]))
/* USER CODE END PV */

/* USER CODE BEGIN 0 */
// SSI обработчик
u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen)
{
    switch(iIndex) {
        case SSI_SENSOR_VALUE:
            strncpy(pcInsert, ssi_data.sensor_value, iInsertLen);
            break;
            
        case SSI_DEVICE_STATUS:
            strncpy(pcInsert, ssi_data.device_status, iInsertLen);
            break;
            
        case SSI_TIMESTAMP:
            strncpy(pcInsert, ssi_data.timestamp, iInsertLen);
            break;
            
        default:
            return 0;
    }
    
    return strlen(pcInsert);
}

// CGI обработчик для /cgi/control (старый API)
static const char *cgi_control_handler(int iIndex, int iNumParams, 
                                        char *pcParam[], char *pcValue[])
{
    printf("CGI Control handler called with %d params\r\n", iNumParams);
    
    // Обработка параметров
    for(int i = 0; i < iNumParams; i++) {
        printf("  Param[%d]: %s = %s\r\n", i, pcParam[i], pcValue[i]);
        
        if(strcmp(pcParam[i], "action") == 0) {
            if(strcmp(pcValue[i], "reset") == 0) {
                printf("CGI: Reset command received\r\n");
                strcpy(ssi_data.device_status, "Reset");
            } else if(strcmp(pcValue[i], "start") == 0) {
                printf("CGI: Start command received\r\n");
                strcpy(ssi_data.device_status, "Running");
            } else if(strcmp(pcValue[i], "stop") == 0) {
                printf("CGI: Stop command received\r\n");
                strcpy(ssi_data.device_status, "Stopped");
            } else if(strcmp(pcValue[i], "normal") == 0) {
                printf("CGI: Normal mode command received\r\n");
                strcpy(ssi_data.device_status, "Normal");
            }
        }
    }
    
    // Возвращаем путь к файлу для перенаправления
    return "/index.html";
}

// CGI обработчик для /cgi/status (старый API)
static const char *cgi_status_handler(int iIndex, int iNumParams, 
                                       char *pcParam[], char *pcValue[])
{
    printf("CGI Status handler called\r\n");
    
    // Обработка параметров статуса
    for(int i = 0; i < iNumParams; i++) {
        printf("  Status Param[%d]: %s = %s\r\n", i, pcParam[i], pcValue[i]);
    }
    
    // Возвращаем страницу статуса с SSI тегами
    return "/status.html";
}

// Заглушка для httpd_cgi_handler (требуется линковщиком, но не используется)
void httpd_cgi_handler(struct fs_file *file, const char* uri, 
                       int iNumParams, char **pcParam, char **pcValue)
{
    // Эта функция не вызывается, но нужна для линковки
    // Перенаправляем вызовы на наши обработчики
    if(strcmp(uri, "/cgi/control") == 0) {
        cgi_control_handler(0, iNumParams, pcParam, pcValue);
    } else if(strcmp(uri, "/cgi/status") == 0) {
        cgi_status_handler(1, iNumParams, pcParam, pcValue);
    }
}

// Обновление SSI данных
void update_ssi_data(void)
{
    static uint32_t last_update = 0;
    uint32_t now = HAL_GetTick();
    
    if(now - last_update >= 1000) {
        last_update = now;
        
        // Обновление значений датчика (имитация)
        float temp = 25.5 + (float)(HAL_GetTick() % 1000) / 100.0;
        snprintf(ssi_data.sensor_value, sizeof(ssi_data.sensor_value), 
                 "%.1f C", temp);
        
        // Обновление статуса (если не был изменен через CGI)
        if(strcmp(ssi_data.device_status, "Reset") != 0 &&
           strcmp(ssi_data.device_status, "Starting...") != 0 &&
           strcmp(ssi_data.device_status, "Running") != 0 &&
           strcmp(ssi_data.device_status, "Stopped") != 0 &&
           strcmp(ssi_data.device_status, "Normal") != 0) {
            if(HAL_GetTick() % 10000 < 5000) {
                strcpy(ssi_data.device_status, "Normal");
            } else {
                strcpy(ssi_data.device_status, "Warning");
            }
        }
        
        // Обновление времени работы
        uint32_t seconds = HAL_GetTick() / 1000;
        snprintf(ssi_data.timestamp, sizeof(ssi_data.timestamp),
                 "%lu sec", seconds);
    }
}

// Инициализация HTTP обработчиков
void http_handlers_init(void)
{
    // Начальные значения
    strcpy(ssi_data.sensor_value, "Initializing...");
    strcpy(ssi_data.device_status, "Starting...");
    strcpy(ssi_data.timestamp, "0 sec");
    
    // Регистрация SSI
    http_set_ssi_handler(ssi_handler, ssi_tags, SSI_COUNT);
    
    // Регистрация CGI обработчиков (используем массив tCGI)
    http_set_cgi_handlers(cgi_handlers, NUM_CGI_HANDLERS);
    
    printf("HTTP handlers initialized (SSI + CGI)\r\n");
    printf("SSI tags: sensor, status, time\r\n");
    printf("CGI handlers: /cgi/control, /cgi/status\r\n");
}
/* USER CODE END 0 */
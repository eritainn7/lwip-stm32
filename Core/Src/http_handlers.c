/* USER CODE BEGIN Includes */
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"
#include "string.h"
#include "stdio.h"
#include "main.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
// Данные датчиков
static char temperature_str[32];
static char light_str[32];
static char timestamp_str[32];

// Список SSI тегов
enum SSI_TAGS {
    SSI_TEMPERATURE = 0,
    SSI_LIGHT,
    SSI_TIMESTAMP,
    SSI_COUNT
};

const char *ssi_tags[SSI_COUNT] = {
    "temperature",   // <!--#temperature-->
    "light",         // <!--#light-->
    "timestamp"      // <!--#timestamp-->
};
/* USER CODE END PV */

/* USER CODE BEGIN 0 */
// SSI обработчик
u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen)
{
    switch(iIndex) {
        case SSI_TEMPERATURE:
            strncpy(pcInsert, temperature_str, iInsertLen);
            break;
            
        case SSI_LIGHT:
            strncpy(pcInsert, light_str, iInsertLen);
            break;
            
        case SSI_TIMESTAMP:
            strncpy(pcInsert, timestamp_str, iInsertLen);
            break;
            
        default:
            pcInsert[0] = '\0';
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
                strcpy(temperature_str, "Reset");
            } else if(strcmp(pcValue[i], "start") == 0) {
                printf("CGI: Start command received\r\n");
                strcpy(temperature_str, "Running");
            } else if(strcmp(pcValue[i], "stop") == 0) {
                printf("CGI: Stop command received\r\n");
                strcpy(temperature_str, "Stopped");
            } else if(strcmp(pcValue[i], "normal") == 0) {
                printf("CGI: Normal mode command received\r\n");
                strcpy(temperature_str, "Normal");
            }
        }
    }
    
    // Возвращаем путь к файлу для перенаправления
    return "/index.html";
}

// // CGI обработчик для /cgi/status (старый API)
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

// // Заглушка для httpd_cgi_handler (требуется линковщиком, но не используется)
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

// Имитация чтения датчика температуры
static float read_temperature(void)
{
    // Здесь должен быть реальный код чтения ADC или датчика
    return 25.5f + (float)(HAL_GetTick() % 1000) / 100.0f;
}

// Имитация чтения датчика освещенности
static uint16_t read_light(void)
{
    // Здесь должен быть реальный код чтения датчика
    return (HAL_GetTick() % 4096);
}

// Обновление данных датчиков
void ssi_update_data(void)
{
    static uint32_t last_update = 0;
    uint32_t now = HAL_GetTick();
    
    if(now - last_update >= 1000) {
        last_update = now;
        
        // Чтение температуры
        float temp = read_temperature();
        snprintf(temperature_str, sizeof(temperature_str), "%.1f °C", temp);
        
        // Чтение освещенности
        uint16_t light = read_light();
        float light_percent = (float)light * 100.0f / 4095.0f;
        snprintf(light_str, sizeof(light_str), "%d (%.1f%%)", light, light_percent);
        
        // Временная метка
        uint32_t seconds = now / 1000;
        uint32_t hours = seconds / 3600;
        uint32_t minutes = (seconds % 3600) / 60;
        uint32_t secs = seconds % 60;
        snprintf(timestamp_str, sizeof(timestamp_str), 
                 "%02lu:%02lu:%02lu", hours, minutes, secs);
    }
}

// Инициализация HTTP обработчиков
void http_handlers_init(void)
{
    // Начальные значения
    strcpy(temperature_str, "--.- C");
    strcpy(light_str, "--- lux");
    strcpy(timestamp_str, "0 sec");
    
    // Регистрация SSI
    http_set_ssi_handler(ssi_handler, ssi_tags, SSI_COUNT);
    
    // Регистрация CGI обработчиков (используем массив tCGI)
    //http_set_cgi_handlers(cgi_handlers, NUM_CGI_HANDLERS);
    
    printf("SSI handlers initialized\r\n");
    printf("Tags: temperature, light, timestamp\r\n");
}
/* USER CODE END 0 */
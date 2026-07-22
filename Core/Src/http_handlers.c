/* USER CODE BEGIN Includes */
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"
#include "string.h"
#include "stdio.h"
#include "main.h"
#include "ha_client.h"
#include "http_handlers.h"
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

// Заглушка для httpd_cgi_handler (требуется линковщиком, но не используется)
void httpd_cgi_handler(struct fs_file *file, const char* uri, 
                       int iNumParams, char **pcParam, char **pcValue)
{
        printf("CGI: %s called (ignored)\r\n", uri);
}

// Обновление данных датчиков
void ssi_update_data(void)
{
    static uint32_t last_update = 0;
    uint32_t now = HAL_GetTick();
    
    if(now - last_update >= 1000) {
        last_update = now;
        
        // Получаем температуру из HA
        int16_t temp_deci = HA_GetTemperature();
        if (temp_deci != -999) {
            snprintf(temperature_str, sizeof(temperature_str), 
                     "%d.%d °C", temp_deci / 10, abs(temp_deci % 10));
        } else if (!HA_IsAvailable()) {
            strcpy(temperature_str, "Offline");
        }
        
        // Получаем освещённость из HA
        int32_t light = HA_GetLight();
        if (light >= 0) {
            snprintf(light_str, sizeof(light_str), "%ld lx", light);
        } else if (!HA_IsAvailable()) {
            strcpy(light_str, "Offline");
        }
        
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
    strcpy(temperature_str, "--.- °C");
    strcpy(light_str, "--- lx");
    strcpy(timestamp_str, "0 sec");
    
    // Регистрация SSI
    http_set_ssi_handler(ssi_handler, ssi_tags, SSI_COUNT);
}
/* USER CODE END 0 */
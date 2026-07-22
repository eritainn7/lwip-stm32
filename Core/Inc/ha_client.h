/**
 * @file    ha_client.h
 * @brief   HomeAssistant REST API Client for STM32 + LwIP
 */

#ifndef __HA_CLIENT_H__
#define __HA_CLIENT_H__

#include <stdint.h>

#define HA_HOST "192.168.1.72"
#define HA_PORT 8123
#define HA_TOKEN "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJkOGFkNDM4Yzk0ZDI0ZDE1OWRiMzdiZjllOWI2N2I2OCIsImlhdCI6MTc4NDcyOTE5MCwiZXhwIjoyMTAwMDg5MTkwfQ.Grjd0Rtg9yMXssuPj-IZr8z24ZIBR55lc3wZPxYW0wM"


#define HA_ENTITY_TEMPERATURE  "sensor.esp32_light_and_temp_sensor_temperatura"
#define HA_ENTITY_LIGHT "sensor.esp32_light_and_temp_sensor_osveshchennost"

#define HA_TIMEOUT_MS           3000                    
#define HA_RX_BUFFER_SIZE       1024                    
#define HA_RETRY_COUNT          2  

/**
 * @brief Геттеры для инкапсулированных переменных
 */
int16_t getLastTemperature(void);
int32_t getLastLight(void);

/**
 * @brief  Инициализация HA клиента
 */
void HA_Client_Init(void);

/**
 * @brief  Получить температуру из HomeAssistant
 * @return Температура в десятых долях градуса (314 = 31.4°C)
 *         -999 при ошибке
 */
int16_t HA_GetTemperature(void);

/**
 * @brief  Получить освещённость из HomeAssistant
 * @return Освещённость в люксах
 *         -1 при ошибке
 */
int32_t HA_GetLight(void);

/**
 * @brief  Проверить доступность HomeAssistant
 * @return 1 - доступен, 0 - недоступен
 */
uint8_t HA_IsAvailable(void);

#endif /* __HA_CLIENT_H__ */

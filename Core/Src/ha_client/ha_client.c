/**
 * @file    ha_client.c
 * @brief   HTTP клиент для HomeAssistant API (Raw API LwIP)
 */

#include "ha_client.h"
#include "ha_parser.h"
#include "lwip/tcp.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"
#include "string.h"
#include "stdio.h"
#include "main.h"

/* Состояния HTTP клиента */
typedef enum {
    HA_STATE_IDLE = 0,
    HA_STATE_CONNECTING,
    HA_STATE_CONNECTED,
    HA_STATE_SENDING,
    HA_STATE_RECEIVING,
    HA_STATE_COMPLETE,
    HA_STATE_ERROR
} ha_state_t;

/* Структура HTTP запроса */
typedef struct {
    ha_state_t state;
    struct tcp_pcb *pcb;
    ip_addr_t server_ip;
    
    char rx_buffer[HA_RX_BUFFER_SIZE];
    int rx_len;
    int rx_expected_len;
    
    char request[512];
    int request_len;
    int request_sent;
    
    char response_data[256];
    int response_len;
    int success;
    
    uint32_t start_time;
    const char *entity_id;
    char *target_buffer;
    int target_size;
} ha_request_t;

/* Приватные переменные */
static ha_request_t ha_req;
static uint8_t ha_available = 0;
static uint32_t consecutive_errors = 0;
static int16_t last_temperature = -999;
static int32_t last_light = -1;

int16_t getLastTemperature(void) {
    return last_temperature;
}

int32_t getLastLight(void) {
    return last_light;
}

/* Прототипы callback-функций */
static err_t ha_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t ha_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t ha_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void ha_error_callback(void *arg, err_t err);
static err_t ha_poll_callback(void *arg, struct tcp_pcb *tpcb);

/**
 * @brief  Отправить HTTP запрос
 */
static err_t ha_send_request(ha_request_t *req)
{
    err_t err;
    
    // Отправляем запрос
    err = tcp_write(req->pcb, req->request, req->request_len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("HA: tcp_write failed: %d\r\n", err);
        return err;
    }
    
    // Отправляем данные
    err = tcp_output(req->pcb);
    if (err != ERR_OK) {
        printf("HA: tcp_output failed: %d\r\n", err);
        return err;
    }
    
    req->state = HA_STATE_SENDING;
    printf("HA: Request sent\r\n");
    
    return ERR_OK;
}

/**
 * @brief  Callback при подключении к серверу
 */
static err_t ha_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    ha_request_t *req = (ha_request_t *)arg;
    
    if (err != ERR_OK) {
        printf("HA: Connection failed: %d\r\n", err);
        req->state = HA_STATE_ERROR;
        return err;
    }
    
    printf("HA: Connected to server\r\n");
    req->state = HA_STATE_CONNECTED;
    
    // Устанавливаем callback'и
    tcp_recv(tpcb, ha_recv_callback);
    tcp_sent(tpcb, ha_sent_callback);
    tcp_err(tpcb, ha_error_callback);
    tcp_poll(tpcb, ha_poll_callback, 1);  // Таймаут каждую секунду
    
    // Отправляем HTTP запрос
    return ha_send_request(req);
}

/**
 * @brief  Callback при получении данных
 */
static err_t ha_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    ha_request_t *req = (ha_request_t *)arg;
    
    if (p == NULL) {
        // Соединение закрыто сервером
        printf("HA: Connection closed by server\r\n");
        req->state = HA_STATE_COMPLETE;
        
        // Парсим полученные данные
        if (req->rx_len > 0) {
            req->rx_buffer[req->rx_len] = '\0';
            
            // Извлекаем тело JSON
            const char *json_body = HA_Parser_ExtractBody(req->rx_buffer);
            if (json_body) {
                printf("HA: JSON body: %.100s...\r\n", json_body);
                
                // Парсим значение state
                int result = HA_Parser_GetValue(json_body, "state", 
                                                req->response_data, sizeof(req->response_data));
                if (result > 0) {
                    req->response_len = result;
                    req->success = 1;
                    printf("HA: Got value: '%s'\r\n", req->response_data);
                } else {
                    printf("HA: Failed to parse state\r\n");
                }
            } else {
                printf("HA: No JSON body found\r\n");
            }
        }
        
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    // Копируем полученные данные
    if (req->rx_len + p->tot_len < HA_RX_BUFFER_SIZE) {
        pbuf_copy_partial(p, req->rx_buffer + req->rx_len, p->tot_len, 0);
        req->rx_len += p->tot_len;
        printf("HA: Received %d bytes (total: %d)\r\n", p->tot_len, req->rx_len);
    } else {
        printf("HA: Buffer overflow!\r\n");
    }
    
    pbuf_free(p);
    
    // Освобождаем принятые данные
    tcp_recved(tpcb, p->tot_len);
    
    return ERR_OK;
}

/**
 * @brief  Callback при отправке данных
 */
static err_t ha_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    ha_request_t *req = (ha_request_t *)arg;
    req->request_sent += len;
    
    printf("HA: Sent %d bytes (total: %d/%d)\r\n", len, req->request_sent, req->request_len);
    
    return ERR_OK;
}

/**
 * @brief  Callback при ошибке
 */
static void ha_error_callback(void *arg, err_t err)
{
    ha_request_t *req = (ha_request_t *)arg;
    printf("HA: Error: %d\r\n", err);
    req->state = HA_STATE_ERROR;
    req->pcb = NULL;
}

/**
 * @brief  Callback для проверки таймаута
 */
static err_t ha_poll_callback(void *arg, struct tcp_pcb *tpcb)
{
    ha_request_t *req = (ha_request_t *)arg;
    uint32_t now = HAL_GetTick();
    
    if (now - req->start_time > HA_TIMEOUT_MS) {
        printf("HA: Timeout\r\n");
        req->state = HA_STATE_ERROR;
        tcp_abort(tpcb);
        return ERR_ABRT;
    }
    
    return ERR_OK;
}

/**
 * @brief  Выполнить HTTP GET запрос к HomeAssistant
 */
static int ha_http_request(const char *entity_id, char *response, int max_len)
{
    err_t err;
    struct tcp_pcb *pcb;
    ip_addr_t ha_ip;
    
    // Преобразуем IP адрес
    if (ipaddr_aton(HA_HOST, &ha_ip) != 1) {
        printf("HA: Invalid IP address: %s\r\n", HA_HOST);
        return -1;
    }
    
    // Создаём TCP PCB
    pcb = tcp_new();
    if (pcb == NULL) {
        printf("HA: Failed to create PCB\r\n");
        return -1;
    }
    
    // Инициализируем структуру запроса
    memset(&ha_req, 0, sizeof(ha_req));
    ha_req.state = HA_STATE_CONNECTING;
    ha_req.pcb = pcb;
    ha_req.start_time = HAL_GetTick();
    ha_req.entity_id = entity_id;
    ha_req.target_buffer = response;
    ha_req.target_size = max_len;
    
    // Формируем HTTP GET запрос
    snprintf(ha_req.request, sizeof(ha_req.request),
        "GET /api/states/%s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Authorization: Bearer %s\r\n"
        "Content-Type: application/json\r\n"
        "Connection: close\r\n"
        "\r\n",
        entity_id, HA_HOST, HA_PORT, HA_TOKEN
    );
    ha_req.request_len = strlen(ha_req.request);
    
    // Сохраняем аргумент для callback'ов
    tcp_arg(pcb, &ha_req);
    
    // Устанавливаем callback для подключения
    tcp_err(pcb, ha_error_callback);
    
    // Подключаемся к серверу
    err = tcp_connect(pcb, &ha_ip, HA_PORT, ha_connected_callback);
    if (err != ERR_OK) {
        printf("HA: tcp_connect failed: %d\r\n", err);
        tcp_close(pcb);
        return -1;
    }
    
    // Ждём завершения запроса
    uint32_t timeout = HA_TIMEOUT_MS;
    uint32_t start = HAL_GetTick();
    
    while (ha_req.state != HA_STATE_COMPLETE && ha_req.state != HA_STATE_ERROR) {
        // Вызываем LwIP обработку
        MX_LWIP_Process();
        
        if (HAL_GetTick() - start > timeout) {
            printf("HA: Request timeout\r\n");
            ha_req.state = HA_STATE_ERROR;
            break;
        }
        
        HAL_Delay(10);
    }
    
    if (ha_req.state == HA_STATE_COMPLETE && ha_req.success) {
        // Копируем результат
        if (ha_req.response_len < max_len) {
            strncpy(response, ha_req.response_data, max_len);
            return ha_req.response_len;
        }
    }
    
    return -1;
}

/**
 * @brief  Инициализация HA клиента
 */
void HA_Client_Init(void)
{
    printf("HA Client: Initialized\r\n");
    printf("  Host: %s:%d\r\n", HA_HOST, HA_PORT);
    printf("  Temperature entity: %s\r\n", HA_ENTITY_TEMPERATURE);
    printf("  Light entity: %s\r\n", HA_ENTITY_LIGHT);
    
    ha_available = 0;
    consecutive_errors = 0;
    last_temperature = -999;
    last_light = -1;
}

/**
 * @brief  Получить температуру из HomeAssistant
 */
int16_t HA_GetTemperature(void)
{
    char value[32];
    int retry;
    
    for (retry = 0; retry <= HA_RETRY_COUNT; retry++) {
        int result = ha_http_request(HA_ENTITY_TEMPERATURE, value, sizeof(value));
        
        if (result > 0) {
            printf("HA: Temperature = '%s'\r\n", value);
            
            // Пробуем преобразовать
            float temp_f = atof(value);
            if (temp_f != 0.0f || value[0] == '0') {
                last_temperature = (int16_t)(temp_f * 10);
                ha_available = 1;
                consecutive_errors = 0;
                return last_temperature;
            }
        }
        
        printf("HA: Temperature request failed (attempt %d)\r\n", retry + 1);
        HAL_Delay(100);
    }
    
    consecutive_errors++;
    if (consecutive_errors >= 3) {
        ha_available = 0;
    }
    
    return last_temperature;  // Возвращаем последнее успешное значение или -999
}

/**
 * @brief  Получить освещённость из HomeAssistant
 */
int32_t HA_GetLight(void)
{
    char value[32];
    int retry;
    
    for (retry = 0; retry <= HA_RETRY_COUNT; retry++) {
        int result = ha_http_request(HA_ENTITY_LIGHT, value, sizeof(value));
        
        if (result > 0) {
            printf("HA: Light = '%s'\r\n", value);
            last_light = atoi(value);
            ha_available = 1;
            consecutive_errors = 0;
            return last_light;
        }
        
        printf("HA: Light request failed (attempt %d)\r\n", retry + 1);
        HAL_Delay(100);
    }
    
    consecutive_errors++;
    if (consecutive_errors >= 3) {
        ha_available = 0;
    }
    
    return last_light;  // Возвращаем последнее успешное значение или -1
}

/**
 * @brief  Проверить доступность HomeAssistant
 */
uint8_t HA_IsAvailable(void)
{
    return ha_available;
}
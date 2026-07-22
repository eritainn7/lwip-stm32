#ifndef TM1637_H
#define TM1637_H

#include "stm32f7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

// Определения для TM1637
#define ADDR_AUTO  0x40
#define ADDR_FIXED 0x44

#define STARTADDR  0xC0

// Определения для точки на дисплее
#define POINT_ON   1
#define POINT_OFF  0

// Определения яркости
#define BRIGHT_DARKEST 0
#define BRIGHT_TYPICAL 2
#define BRIGHTEST      7

// Специальные символы
#define INDEX_NEGATIVE_SIGN 16
#define INDEX_BLANK         17

// Типы дисплеев
#define D4036B 0
#define D4056A 1

typedef struct {
    GPIO_TypeDef* CLK_Port;
    uint16_t CLK_Pin;
    GPIO_TypeDef* DIO_Port;
    uint16_t DIO_Pin;
    
    uint8_t Cmd_SetData;
    uint8_t Cmd_SetAddr;
    uint8_t Cmd_DispCtrl;
    bool _PointFlag;
    uint8_t _DispType;
    uint8_t DecPoint;
    bool BlankingFlag;
} TM1637_HandleTypeDef;

// Таблица символов для 7-сегментного индикатора
static const uint8_t TubeTab[] = {
    0x3F, 0x06, 0x5B, 0x4F,  // 0,1,2,3
    0x66, 0x6D, 0x7D, 0x07,  // 4,5,6,7
    0x7F, 0x6F, 0x77, 0x7C,  // 8,9,A,b
    0x39, 0x5E, 0x79, 0x71,  // C,d,E,F
    0x40, 0x00, 0x63, 0x50,  // -, ,°,r
    0x74, 0x54, 0x78, 0x6A,  // h,n,t,o
    0x1D, 0x5C, 0x73         // P,u_,u-
};

// Прототипы функций
void TM1637_Init(TM1637_HandleTypeDef* htm1637, 
                GPIO_TypeDef* clk_port, uint16_t clk_pin,
                GPIO_TypeDef* dio_port, uint16_t dio_pin,
                uint8_t disp_type);
void TM1637_DisplayArray(TM1637_HandleTypeDef* htm1637, uint8_t DispData[]);
void TM1637_DisplayDigit(TM1637_HandleTypeDef* htm1637, uint8_t BitAddr, uint8_t DispData);
void TM1637_DisplayDecimal(TM1637_HandleTypeDef* htm1637, double Decimal);
void TM1637_DisplayInt(TM1637_HandleTypeDef* htm1637, int16_t Decimal);
void TM1637_ClearDisplay(TM1637_HandleTypeDef* htm1637);
void TM1637_SetBrightness(TM1637_HandleTypeDef* htm1637, uint8_t brightness);
void TM1637_SetPoint(TM1637_HandleTypeDef* htm1637, bool PointFlag);
void TM1637_Start(TM1637_HandleTypeDef* htm1637);
void TM1637_Stop(TM1637_HandleTypeDef* htm1637);
void TM1637_WriteByte(TM1637_HandleTypeDef* htm1637, uint8_t wr_data);
void TM1637_Coding(TM1637_HandleTypeDef* htm1637, uint8_t DispData[]);
uint8_t TM1637_CodingSingle(TM1637_HandleTypeDef* htm1637, uint8_t DispData);
void TM1637_DelayUs(uint16_t us);

#endif /* TM1637_H */
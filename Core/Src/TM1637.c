#include "TM1637.h"
#include <stdlib.h>

// Функция задержки в микросекундах для STM32F767ZI (216 МГц)
void TM1637_DelayUs(uint16_t us) {
    uint32_t delay = us * 54;  // Приблизительно 54 такта на мкс при 216 МГц
    while(delay--) {
        __NOP();
    }
}

void TM1637_Init(TM1637_HandleTypeDef* htm1637,
                GPIO_TypeDef* clk_port, uint16_t clk_pin,
                GPIO_TypeDef* dio_port, uint16_t dio_pin,
                uint8_t disp_type) {
    htm1637->CLK_Port = clk_port;
    htm1637->CLK_Pin = clk_pin;
    htm1637->DIO_Port = dio_port;
    htm1637->DIO_Pin = dio_pin;
    htm1637->_DispType = disp_type;
    htm1637->BlankingFlag = true;
    htm1637->DecPoint = 3;
    htm1637->Cmd_SetData = ADDR_AUTO;
    htm1637->Cmd_SetAddr = STARTADDR;
    htm1637->Cmd_DispCtrl = 0x88 + BRIGHT_TYPICAL;
    
    TM1637_ClearDisplay(htm1637);
}

void TM1637_WriteByte(TM1637_HandleTypeDef* htm1637, uint8_t wr_data) {
    uint8_t i;
    uint16_t count1 = 0;
    
    for(i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(htm1637->CLK_Port, htm1637->CLK_Pin, GPIO_PIN_RESET);
        
        if(wr_data & 0x01)
            HAL_GPIO_WritePin(htm1637->DIO_Port, htm1637->DIO_Pin, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(htm1637->DIO_Port, htm1637->DIO_Pin, GPIO_PIN_RESET);
        
        TM1637_DelayUs(3);
        wr_data >>= 1;
        
        HAL_GPIO_WritePin(htm1637->CLK_Port, htm1637->CLK_Pin, GPIO_PIN_SET);
        TM1637_DelayUs(3);
    }
    
    // Ожидание ACK
    HAL_GPIO_WritePin(htm1637->CLK_Port, htm1637->CLK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(htm1637->DIO_Port, htm1637->DIO_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(htm1637->CLK_Port, htm1637->CLK_Pin, GPIO_PIN_SET);
    
    // Настройка DIO как вход для чтения ACK
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = htm1637->DIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(htm1637->DIO_Port, &GPIO_InitStruct);
    
    while(HAL_GPIO_ReadPin(htm1637->DIO_Port, htm1637->DIO_Pin)) {
        count1++;
        if(count1 == 200) {
            // Возвращаем DIO в режим выхода
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            HAL_GPIO_Init(htm1637->DIO_Port, &GPIO_InitStruct);
            HAL_GPIO_WritePin(htm1637->DIO_Port, htm1637->DIO_Pin, GPIO_PIN_RESET);
            count1 = 0;
            
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            HAL_GPIO_Init(htm1637->DIO_Port, &GPIO_InitStruct);
        }
    }
    
    // Возвращаем DIO в режим выхода
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(htm1637->DIO_Port, &GPIO_InitStruct);
}

void TM1637_Start(TM1637_HandleTypeDef* htm1637) {
    HAL_GPIO_WritePin(htm1637->CLK_Port, htm1637->CLK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(htm1637->DIO_Port, htm1637->DIO_Pin, GPIO_PIN_SET);
    TM1637_DelayUs(2);
    HAL_GPIO_WritePin(htm1637->DIO_Port, htm1637->DIO_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(htm1637->CLK_Port, htm1637->CLK_Pin, GPIO_PIN_RESET);
}

void TM1637_Stop(TM1637_HandleTypeDef* htm1637) {
    HAL_GPIO_WritePin(htm1637->CLK_Port, htm1637->CLK_Pin, GPIO_PIN_RESET);
    TM1637_DelayUs(2);
    HAL_GPIO_WritePin(htm1637->DIO_Port, htm1637->DIO_Pin, GPIO_PIN_RESET);
    TM1637_DelayUs(2);
    HAL_GPIO_WritePin(htm1637->CLK_Port, htm1637->CLK_Pin, GPIO_PIN_SET);
    TM1637_DelayUs(2);
    HAL_GPIO_WritePin(htm1637->DIO_Port, htm1637->DIO_Pin, GPIO_PIN_SET);
}

void TM1637_Coding(TM1637_HandleTypeDef* htm1637, uint8_t DispData[]) {
    uint8_t PointData;
    if(htm1637->_PointFlag == POINT_ON)
        PointData = 0x80;
    else
        PointData = 0;
    
    for(uint8_t i = 0; i < 4; i++) {
        if(DispData[i] == 0x7F)
            DispData[i] = 0x00;
        else
            DispData[i] = TubeTab[DispData[i]] + PointData;
    }
    
    if((htm1637->_DispType == D4056A) && (htm1637->DecPoint != 3)) {
        DispData[htm1637->DecPoint] += 0x80;
        htm1637->DecPoint = 3;
    }
}

uint8_t TM1637_CodingSingle(TM1637_HandleTypeDef* htm1637, uint8_t DispData) {
    uint8_t PointData;
    if(htm1637->_PointFlag == POINT_ON)
        PointData = 0x80;
    else
        PointData = 0;
    
    if(DispData == 0x7F)
        DispData = 0x00 + PointData;
    else
        DispData = TubeTab[DispData] + PointData;
    
    return DispData;
}

void TM1637_DisplayArray(TM1637_HandleTypeDef* htm1637, uint8_t DispData[]) {
    uint8_t SegData[4];
    for(uint8_t i = 0; i < 4; i++) {
        SegData[i] = DispData[i];
    }
    
    TM1637_Coding(htm1637, SegData);
    
    TM1637_Start(htm1637);
    TM1637_WriteByte(htm1637, ADDR_AUTO);
    TM1637_Stop(htm1637);
    
    TM1637_Start(htm1637);
    TM1637_WriteByte(htm1637, htm1637->Cmd_SetAddr);
    for(uint8_t i = 0; i < 4; i++) {
        TM1637_WriteByte(htm1637, SegData[i]);
    }
    TM1637_Stop(htm1637);
    
    TM1637_Start(htm1637);
    TM1637_WriteByte(htm1637, htm1637->Cmd_DispCtrl);
    TM1637_Stop(htm1637);
}

void TM1637_DisplayDigit(TM1637_HandleTypeDef* htm1637, uint8_t BitAddr, uint8_t DispData) {
    uint8_t SegData = TM1637_CodingSingle(htm1637, DispData);
    
    TM1637_Start(htm1637);
    TM1637_WriteByte(htm1637, ADDR_FIXED);
    TM1637_Stop(htm1637);
    
    TM1637_Start(htm1637);
    TM1637_WriteByte(htm1637, BitAddr | 0xC0);
    TM1637_WriteByte(htm1637, SegData);
    TM1637_Stop(htm1637);
    
    TM1637_Start(htm1637);
    TM1637_WriteByte(htm1637, htm1637->Cmd_DispCtrl);
    TM1637_Stop(htm1637);
}

void TM1637_DisplayInt(TM1637_HandleTypeDef* htm1637, int16_t Decimal) {
    uint8_t temp[4];
    
    if((Decimal > 9999) || (Decimal < -999))
        return;
    
    if(Decimal < 0) {
        temp[0] = INDEX_NEGATIVE_SIGN;
        Decimal = abs(Decimal);
        temp[1] = Decimal / 100;
        Decimal %= 100;
        temp[2] = Decimal / 10;
        temp[3] = Decimal % 10;
        
        if(htm1637->BlankingFlag) {
            if(temp[1] == 0) {
                temp[1] = INDEX_BLANK;
                if(temp[2] == 0)
                    temp[2] = INDEX_BLANK;
            }
        }
    } else {
        temp[0] = Decimal / 1000;
        Decimal %= 1000;
        temp[1] = Decimal / 100;
        Decimal %= 100;
        temp[2] = Decimal / 10;
        temp[3] = Decimal % 10;
        
        if(htm1637->BlankingFlag) {
            if(temp[0] == 0) {
                temp[0] = INDEX_BLANK;
                if(temp[1] == 0) {
                    temp[1] = INDEX_BLANK;
                    if(temp[2] == 0)
                        temp[2] = INDEX_BLANK;
                }
            }
        }
    }
    
    htm1637->BlankingFlag = true;
    TM1637_DisplayArray(htm1637, temp);
}

void TM1637_DisplayDecimal(TM1637_HandleTypeDef* htm1637, double Decimal) {
    int16_t temp;
    
    if(Decimal > 9999) return;
    else if(Decimal < -999) return;
    
    uint8_t i = 3;
    if(Decimal > 0) {
        for(; i > 0; i--) {
            if(Decimal < 1000)
                Decimal *= 10;
            else
                break;
        }
        temp = (int16_t)Decimal;
        if((Decimal - temp) > 0.5) temp++;
    } else {
        for(; i > 1; i--) {
            if(Decimal > -100)
                Decimal *= 10;
            else
                break;
        }
        temp = (int16_t)Decimal;
        if((temp - Decimal) > 0.5) temp--;
    }
    
    htm1637->DecPoint = i;
    htm1637->BlankingFlag = false;
    TM1637_DisplayInt(htm1637, temp);
}

void TM1637_ClearDisplay(TM1637_HandleTypeDef* htm1637) {
    TM1637_DisplayDigit(htm1637, 0x00, 0x7F);
    TM1637_DisplayDigit(htm1637, 0x01, 0x7F);
    TM1637_DisplayDigit(htm1637, 0x02, 0x7F);
    TM1637_DisplayDigit(htm1637, 0x03, 0x7F);
}

void TM1637_SetBrightness(TM1637_HandleTypeDef* htm1637, uint8_t brightness) {
    htm1637->Cmd_DispCtrl = 0x88 + brightness;
}

void TM1637_SetPoint(TM1637_HandleTypeDef* htm1637, bool PointFlag) {
    if(htm1637->_DispType == D4036B)
        htm1637->_PointFlag = PointFlag;
}
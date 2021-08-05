//
// Created by 陈宁 on 2021/7/26.
//

#ifndef STM32_4_CC1101_H
#define STM32_4_CC1101_H

#include "main.h"
#include "CC1101_Reg.h"
#include "stdint.h"
#include "stdbool.h"
#include "stdlib.h"

#define PA_TABLE                        {0xc2,0x00,0x00,0x00,0x00,0x00,0x00,0x00,}

#define PKT_SIZE    8

/** 枚举量定义 */

typedef enum {
    TX_MODE, RX_MODE
} CC1101_ModeType;

typedef enum {
    BROAD_ALL, BROAD_NO, BROAD_0, BROAD_0AND255
} CC1101_AddrModeType;

typedef enum {
    BROADCAST, ADDRESS_CHECK
} CC1101_TxDataModeType;

typedef struct __CC1101_HandleTypeDef{
    SPI_HandleTypeDef* spi_port;
    GPIO_TypeDef* CSN_port;
    uint16_t CSN_pin;
    GPIO_TypeDef* GDO0_port;
    uint16_t GDO0_pin;
    CC1101_ModeType tr_mode;
    CC1101_AddrModeType addr_mode;
    CC1101_TxDataModeType tx_data_mode;
    uint8_t address;
    uint8_t frame_head[2];
    uint8_t tx_buffer[PKT_SIZE];
    bool new_tx_flag;
    uint8_t rx_buffer[PKT_SIZE];
    bool new_rx_flag;
} CC1101_HandleTypeDef;

// Public functions
void CC1101_Set_Mode(CC1101_HandleTypeDef* pCC1101, CC1101_ModeType Mode);

void CC1101_Init(CC1101_HandleTypeDef* pCC1101);

bool CC1101_Fetch(CC1101_HandleTypeDef* pCC1101);

bool CC1101_Send(CC1101_HandleTypeDef* pCC1101);

bool Frame_Write(uint8_t* org, CC1101_HandleTypeDef* pCC1101);

bool Frame_Read(uint8_t* des, CC1101_HandleTypeDef* pCC1101);

// Private functions
static void CC1101_Clear_TxBuffer(CC1101_HandleTypeDef* pCC1101);

static void CC1101_Clear_RxBuffer(CC1101_HandleTypeDef* pCC1101);

static void CC1101_Tx_Packet(CC1101_HandleTypeDef* pCC1101, uint8_t *pTxBuff, uint8_t TxSize, CC1101_TxDataModeType DataMode);

static uint8_t CC1101_Get_RxCounter(CC1101_HandleTypeDef* pCC1101);

static uint8_t CC1101_Rx_Packet(CC1101_HandleTypeDef* pCC1101, uint8_t *RxBuff);

static void CC1101_Reset(CC1101_HandleTypeDef* pCC1101);

static void CC1101_Write_Cmd(CC1101_HandleTypeDef* pCC1101, uint8_t Command);

static void CC1101_Write_Reg(CC1101_HandleTypeDef* pCC1101, uint8_t Addr, uint8_t WriteValue);

static void CC1101_Write_Multi_Reg(CC1101_HandleTypeDef* pCC1101, uint8_t Addr, uint8_t *pWriteBuff, uint8_t WriteSize);

static uint8_t CC1101_Read_Reg(CC1101_HandleTypeDef* pCC1101, uint8_t Addr);

static void CC1101_Read_Multi_Reg(CC1101_HandleTypeDef* pCC1101, uint8_t Addr, uint8_t *pReadBuff, uint8_t ReadSize);

uint8_t CC1101_Read_Status(CC1101_HandleTypeDef* pCC1101, uint8_t Addr);

static void CC1101_Set_Idle_Mode(CC1101_HandleTypeDef* pCC1101);

static void CC1101_WOR_Init(CC1101_HandleTypeDef* pCC1101);

static void CC1101_Set_Address(CC1101_HandleTypeDef* pCC1101, uint8_t Address, CC1101_AddrModeType AddressMode);

static void CC1101_Set_Sync(CC1101_HandleTypeDef* pCC1101, uint16_t Sync);

static uint8_t spi_read_write_byte(SPI_HandleTypeDef *hspi, uint8_t TxByte);

static void SET_CSN(CC1101_HandleTypeDef* pCC1101, bool level);

static bool GET_GDO0_STATUS(CC1101_HandleTypeDef* pCC1101);

uint8_t read_status(CC1101_HandleTypeDef* pCC1101);

#endif //STM32_4_CC1101_H

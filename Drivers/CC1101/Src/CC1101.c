//
// Created by 陈宁 on 2021/7/26.
//
#include "CC1101.h"

//10, 7, 5, 0, -5, -10, -15, -20, dbm output power, 0x12 == -30dbm
static const uint8_t PaTabel[] = {0xc0, 0xC8, 0x84, 0x60, 0x68, 0x34, 0x1D, 0x0E};
static const uint8_t CC1101InitData[22][2] = {{CC1101_IOCFG0,   0x06},
                                              {CC1101_FIFOTHR,  0x47},
                                              {CC1101_PKTCTRL0, 0x05},
                                              {CC1101_CHANNR,   0x96},    //430M
                                              {CC1101_FSCTRL1,  0x06},
                                              {CC1101_FREQ2,    0x0F},
                                              {CC1101_FREQ1,    0x62},
                                              {CC1101_FREQ0,    0x76},
                                              {CC1101_MDMCFG4,  0xF6},
                                              {CC1101_MDMCFG3,  0x43},
                                              {CC1101_MDMCFG2,  0x13},
                                              {CC1101_DEVIATN,  0x15},
                                              {CC1101_MCSM0,    0x18},
                                              {CC1101_FOCCFG,   0x16},
                                              {CC1101_WORCTRL,  0xFB},
                                              {CC1101_FSCAL3,   0xE9},
                                              {CC1101_FSCAL2,   0x2A},
                                              {CC1101_FSCAL1,   0x00},
                                              {CC1101_FSCAL0,   0x1F},
                                              {CC1101_TEST2,    0x81},
                                              {CC1101_TEST1,    0x35},
                                              {CC1101_MCSM1,    0x33},};

/**
  * @brief :尝试发送数据帧
  * @param :
  * @note  :无
  * @retval:true:成功将数据发送给CC1101;false:发送缓冲区没有有效数据,发送失败
  */
bool CC1101_Send(CC1101_HandleTypeDef* pCC1101) {
    if(!pCC1101->new_tx_flag) { return false; }
    CC1101_Tx_Packet(pCC1101, pCC1101->tx_buffer, PKT_SIZE, BROADCAST);
    pCC1101->new_tx_flag = false;
    return true;
}

/**
  * @brief :尝试从CC1101的接收缓冲区读取数据
  * @param :
  * @note  :无
  * @retval:true:成功从缓冲区读取到数据;false:未能从缓冲区读取到数据
  */
bool CC1101_Fetch(CC1101_HandleTypeDef* pCC1101) {
    if(CC1101_Get_RxCounter(pCC1101) == 0) { return false; }
    CC1101_Rx_Packet(pCC1101, pCC1101->rx_buffer);
    pCC1101->new_rx_flag = true;
    return true;
}

/**
  * @brief :将主程序中的外部数据拷贝到发送缓冲区
  * @param :
  *			@Command：命令
  * @note  :无
  * @retval:true:拷贝成功;false:拷贝失败
  */
bool Frame_Write(uint8_t* org, CC1101_HandleTypeDef* pCC1101) {
    if(pCC1101->new_tx_flag) { return false; }
    for(int i = 0; i < PKT_SIZE; i++) { pCC1101->tx_buffer[i] = org[i]; }
    pCC1101->new_tx_flag = true;
    return true;
}


/**
  * @brief :将接收缓冲区中的数据拷贝到主程序
  * @param :
  *			@Command：命令
  * @note  :无
  * @retval:无
  */
bool Frame_Read(uint8_t* des, CC1101_HandleTypeDef* pCC1101) {
    if(!pCC1101->new_rx_flag) { return false; }
    for(int i = 0; i < PKT_SIZE; i++) { des[i] = pCC1101->rx_buffer[i]; }
    pCC1101->new_rx_flag = false;
    return true;
}

/**
  * @brief :CC1101写命令
  * @param :
  *			@Command：命令
  * @note  :无
  * @retval:无
  */
static void CC1101_Write_Cmd(CC1101_HandleTypeDef *pCC1101, uint8_t Command) {
    SET_CSN(pCC1101, false);            // 片选选中

    spi_read_write_byte(pCC1101->spi_port, Command);

    SET_CSN(pCC1101, true);             // 取消片选
}

/**
  * @brief :CC1101写寄存器
  * @param :
  *			@Addr：地址
  *			@WriteValue：写入的数据字节
  * @note  :无
  * @retval:无
  */
static void CC1101_Write_Reg(CC1101_HandleTypeDef *pCC1101, uint8_t Addr, uint8_t WriteValue) {
    SET_CSN(pCC1101, false);

    spi_read_write_byte(pCC1101->spi_port, Addr);           //写地址
    spi_read_write_byte(pCC1101->spi_port, WriteValue);     //写数据

    SET_CSN(pCC1101, true);
}

/**
  * @brief :CC1101连续写寄存器
  * @param :
  *			@Addr：地址
  *			@pWriteBuff：写入的数据串首地址
  *			@WriteSize：写入的数据个数
  * @note  :无
  * @retval:无
  */
static void CC1101_Write_Multi_Reg(CC1101_HandleTypeDef *pCC1101, uint8_t Addr, uint8_t *pWriteBuff, uint8_t WriteSize) {
    SET_CSN(pCC1101, false);

    spi_read_write_byte(pCC1101->spi_port, Addr | WRITE_BURST);    //连续写命令 及首地址
    for(int i = 0; i < WriteSize; i++) {
        spi_read_write_byte(pCC1101->spi_port, pWriteBuff[i]);    //连续写入数据
    }

    SET_CSN(pCC1101, true);
}

/**
  * @brief :CC1101读寄存器
  * @param :
  *			@Addr：地址
  * @note  :无
  * @retval:寄存器值
  */
static uint8_t CC1101_Read_Reg(CC1101_HandleTypeDef *pCC1101, uint8_t Addr) {
    uint8_t l_RegValue = 0;

    SET_CSN(pCC1101, false);

    spi_read_write_byte(pCC1101->spi_port, Addr | READ_SINGLE);    //单独读命令 及地址
    l_RegValue = spi_read_write_byte(pCC1101->spi_port, 0xFF);    //读取寄存器

    SET_CSN(pCC1101, true);

    return l_RegValue;
}

/**
  * @brief :CC1101读一个寄存器状态
  * @param :
  *			@Addr：地址
  * @note  :无
  * @retval:寄存器状态
  */
uint8_t CC1101_Read_Status(CC1101_HandleTypeDef *pCC1101, uint8_t Addr) {
    uint8_t l_RegStatus = 0;

    SET_CSN(pCC1101, false);

    spi_read_write_byte(pCC1101->spi_port, Addr | READ_BURST);    //连续读命令 及地址
    l_RegStatus = spi_read_write_byte(pCC1101->spi_port, 0xFF);    //读取状态

    SET_CSN(pCC1101, true);

    return l_RegStatus;
}

/**
  * @brief :CC1101连续读寄存器
  * @param :
  *			@Addr：地址
  *			@pReadBuff：读取数据存放首地址
  *			@ReadSize：读取数据的个数
  * @note  :无
  * @retval:无
  */
static void CC1101_Read_Multi_Reg(CC1101_HandleTypeDef *pCC1101, uint8_t Addr, uint8_t *pReadBuff, uint8_t ReadSize) {
    SET_CSN(pCC1101, false);

    spi_read_write_byte(pCC1101->spi_port, Addr | READ_BURST);    //连续读命令 及首地址
    for(int i = 0; i < ReadSize; i++) {
        //while(pCC1101->spi_port->State != HAL_SPI_STATE_READY) { /*Wait*/ }
        pReadBuff[i] = spi_read_write_byte(pCC1101->spi_port, 0xFF);
    }

    SET_CSN(pCC1101, true);
}

/**
  * @brief :CC1101发送接收模式设置
  * @param :
  *			@Mode：TX_MODE，发送模式 RX_MODE，接收模式
  * @note  :无
  * @retval:寄存器状态
  */
void CC1101_Set_Mode(CC1101_HandleTypeDef *pCC1101, CC1101_ModeType Mode) {
    if(Mode == TX_MODE) {
        CC1101_Write_Reg(pCC1101, CC1101_IOCFG0, 0x46);
        CC1101_Write_Cmd(pCC1101, CC1101_STX);
    }
    else if(Mode == RX_MODE) {
        CC1101_Write_Reg(pCC1101, CC1101_IOCFG0, 0x46);
        CC1101_Write_Cmd(pCC1101, CC1101_SRX);
    }
    pCC1101->tr_mode = Mode;
}

/**
  * @brief :CC1101进入空闲模式
  * @param :无
  * @note  :无
  * @retval:无
  */
static void CC1101_Set_Idle_Mode(CC1101_HandleTypeDef* pCC1101) {
    CC1101_Write_Cmd(pCC1101, CC1101_SIDLE);
}

/**
  * @brief :CC1101初始化WOR功能
  * @param :无
  * @note  :无
  * @retval:无
  */
static void CC1101_WOR_Init(CC1101_HandleTypeDef* pCC1101) {
    CC1101_Write_Reg(pCC1101, CC1101_MCSM0, 0x18);
    CC1101_Write_Reg(pCC1101, CC1101_WORCTRL, 0x78);
    CC1101_Write_Reg(pCC1101, CC1101_MCSM2, 0x00);
    CC1101_Write_Reg(pCC1101, CC1101_WOREVT1, 0x8C);
    CC1101_Write_Reg(pCC1101, CC1101_WOREVT0, 0xA0);
    CC1101_Write_Cmd(pCC1101, CC1101_SWORRST);        //写入WOR命令
}

/**
  * @brief :CC1101设置地址
  * @param :
  *			@Address：设置的设备地址值
  *			@AddressMode：地址检测模式
  * @note  :无
  * @retval:无
  */
static void CC1101_Set_Address(CC1101_HandleTypeDef* pCC1101, uint8_t Address, CC1101_AddrModeType AddressMode) {
    uint8_t btmp = 0;

    pCC1101->addr_mode = AddressMode;
    pCC1101->address = Address;
    btmp = CC1101_Read_Reg(pCC1101, CC1101_PKTCTRL1) & ~0x03;    //读取CC1101_PKTCTRL1寄存器初始值
    CC1101_Write_Reg(pCC1101, CC1101_ADDR, Address);            //设置设备地址

    if(AddressMode == BROAD_ALL) {}                //不检测地址
    else if(AddressMode == BROAD_NO) {
        btmp |= 0x01;                                    //检测地址 但是不带广播
    } else if(AddressMode == BROAD_0) {
        btmp |= 0x02;                                    //0x00为广播
    } else if(AddressMode == BROAD_0AND255) {
        btmp |= 0x03;                                    //0x00 0xFF为广播
    }

    CC1101_Write_Reg(pCC1101, CC1101_PKTCTRL1, btmp);            //写入地址模式
}

/**
  * @brief :CC1101设置同步字段
  * @param :无
  * @note  :无
  * @retval:无
  */
static void CC1101_Set_Sync(CC1101_HandleTypeDef* pCC1101, uint16_t Sync) {
    CC1101_Write_Reg(pCC1101, CC1101_SYNC1, 0xFF & (Sync >> 8));
    CC1101_Write_Reg(pCC1101, CC1101_SYNC0, 0xFF & Sync);    //写入同步字段 16Bit
}

/**
  * @brief :CC1101清空发送缓冲区
  * @param :无
  * @note  :无
  * @retval:无
  */
static void CC1101_Clear_TxBuffer(CC1101_HandleTypeDef* pCC1101) {
    CC1101_Set_Idle_Mode(pCC1101);                    //首先进入IDLE模式
    CC1101_Write_Cmd(pCC1101, CC1101_SFTX);            //写入清发送缓冲区命令
}

/**
  * @brief :CC1101清空接收缓冲区
  * @param :无
  * @note  :无
  * @retval:无
  */
static void CC1101_Clear_RxBuffer(CC1101_HandleTypeDef* pCC1101) {
    CC1101_Set_Idle_Mode(pCC1101);                        //首先进入IDLE模式
    CC1101_Write_Cmd(pCC1101, CC1101_SFRX);            //写入清接收缓冲区命令
}

/**
  * @brief :CC1101发送数据包
  * @param :
  *			@pTxBuff：发送数据缓冲区
  *			@TxSize：发送数据长度
  *			@DataMode：数据模式
  * @note  :无
  * @retval:无
  */
static void CC1101_Tx_Packet(CC1101_HandleTypeDef* pCC1101, uint8_t *pTxBuff, uint8_t TxSize, CC1101_TxDataModeType DataMode) {
    uint8_t Address;
    uint16_t l_RxWaitTimeout = 0;

    if(DataMode == BROADCAST) {
        Address = 0;
    } else if(DataMode == ADDRESS_CHECK) {
        Address = CC1101_Read_Reg(pCC1101, CC1101_ADDR);
    }

    CC1101_Clear_TxBuffer(pCC1101);

    if(CC1101_Read_Reg(pCC1101, CC1101_PKTCTRL1) & 0x03) {
        CC1101_Write_Reg(pCC1101, CC1101_TXFIFO, TxSize + 1);
        CC1101_Write_Reg(pCC1101, CC1101_TXFIFO, Address);            //写入长度和地址 由于多一个字节地址此时长度应该加1
    } else {
        CC1101_Write_Reg(pCC1101, CC1101_TXFIFO, TxSize);            //只写长度 不带地址
    }

    CC1101_Write_Multi_Reg(pCC1101, CC1101_TXFIFO, pTxBuff, TxSize);    //写入数据
    CC1101_Set_Mode(pCC1101, TX_MODE);                                //发送模式

    while(!GET_GDO0_STATUS(pCC1101)) {       //等待发送完成
        HAL_Delay(1);
        if(1000 == l_RxWaitTimeout++) {
            uint8_t status = read_status(pCC1101);
            l_RxWaitTimeout = 0;
            CC1101_Init(pCC1101);
            break;
        }
    }
}

/**
  * @brief :CC1101读取接收到的字节数
  * @param :无
  * @note  :无
  * @retval:接收到的数据个数
  */
static uint8_t CC1101_Get_RxCounter(CC1101_HandleTypeDef* pCC1101) {
    return (CC1101_Read_Status(pCC1101, CC1101_RXBYTES) & BYTES_IN_RXFIFO);
}

/**
  * @brief :CC1101接收数据包
  * @param :
  *			@RxBuff：发送数据缓冲区
  * @note  :无
  * @retval：接收到的字节数，0表示无数据
  */
static uint8_t CC1101_Rx_Packet(CC1101_HandleTypeDef* pCC1101, uint8_t *RxBuff) {
    uint8_t l_PktLen = 0;
    uint8_t l_Status[2] = {0};
    uint16_t l_RxWaitTimeout = 0;

    while(!GET_GDO0_STATUS(pCC1101))        //等待接收完成
    {
        HAL_Delay(1);
        if(3000 == l_RxWaitTimeout++) {
            l_RxWaitTimeout = 0;
            CC1101_Init(pCC1101);
            return 0;
        }
    }

    if(CC1101_Get_RxCounter(pCC1101) != 0) {
        l_PktLen = CC1101_Read_Reg(pCC1101, CC1101_RXFIFO);           // 获取长度信息

        if(CC1101_Read_Reg(pCC1101, CC1101_PKTCTRL1) & 0x03) {
            CC1101_Read_Reg(pCC1101, CC1101_RXFIFO);                    //如果数据包中包含地址信息 ，则读取地址信息
        }
        if(l_PktLen == 0) {
            return 0;            //无数据
        } else {
            l_PktLen--;        //减去一个地址字节
        }
        if(l_PktLen != PKT_SIZE) {      // If length is invalid, drop the received frame
            uint8_t* dummy = (uint8_t*)malloc(l_PktLen * sizeof(uint8_t));
            CC1101_Read_Multi_Reg(pCC1101, CC1101_RXFIFO, dummy, l_PktLen);
            free(dummy);
        }
        else {
            CC1101_Read_Multi_Reg(pCC1101, CC1101_RXFIFO, RxBuff, l_PktLen);    //读取数据
        }
        CC1101_Read_Multi_Reg(pCC1101, CC1101_RXFIFO, l_Status, 2);        //读取数据包最后两个额外字节，后一个为CRC标志位

        CC1101_Clear_RxBuffer(pCC1101);

        if(l_Status[1] & CRC_OK) {
            return l_PktLen;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

/**
  * @brief :CC1101复位
  * @param :无
  * @note  :无
  * @retval:无
  */
static void CC1101_Reset(CC1101_HandleTypeDef* pCC1101) {
    SET_CSN(pCC1101, true);
    SET_CSN(pCC1101, false);
    SET_CSN(pCC1101, true);
    HAL_Delay(1);
    CC1101_Write_Cmd(pCC1101, CC1101_SRES);
}

/**
  * @brief :CC1101初始化
  * @param :无
  * @note  :无
  * @retval:无
  */
void CC1101_Init(CC1101_HandleTypeDef* pCC1101) {
    CC1101_Reset(pCC1101);            //模块复位

    for(int i = 0; i < 22; i++) {
        CC1101_Write_Reg(pCC1101, CC1101InitData[i][0], CC1101InitData[i][1]);    //写入配置参数
    }
    CC1101_Set_Address(pCC1101, 0x05, BROAD_0AND255);        //写入设备地址 和地址模式
    CC1101_Set_Sync(pCC1101, 0x8799);                        //写入同步字段
    CC1101_Write_Reg(pCC1101, CC1101_MDMCFG1, 0x72);            //调制解调器配置

    CC1101_Write_Multi_Reg(pCC1101, CC1101_PATABLE, (uint8_t *) PaTabel, 8);
}

static uint8_t spi_read_write_byte(SPI_HandleTypeDef *hspi, uint8_t TxByte) {
    uint8_t RxByte;
    while(hspi->State != HAL_SPI_STATE_READY) { /*Wait*/ }
    HAL_SPI_TransmitReceive(hspi, &TxByte, &RxByte, 1, 1000);
    return RxByte;
}

/**
  * @brief :设置片选信号
  * @param :
  * @note  :无
  * @retval:无
  */
static void SET_CSN(CC1101_HandleTypeDef *pCC1101, bool level) {
    if(level) { pCC1101->CSN_port->BSRR |= (uint32_t) pCC1101->CSN_pin; }
    else { pCC1101->CSN_port->BSRR |= ((uint32_t) pCC1101->CSN_pin << 16); }
    return;
}

/**
  * @brief :读取GDO0 pin状态
  * @param :
  * @note  :无
  * @retval:无
  */
static bool GET_GDO0_STATUS(CC1101_HandleTypeDef *pCC1101) {
    return pCC1101->GDO0_port->IDR & (uint32_t) pCC1101->GDO0_pin;
}

uint8_t read_status(CC1101_HandleTypeDef* pCC1101) {
    return CC1101_Read_Status(pCC1101, CC1101_MARCSTATE);
}
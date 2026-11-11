#include "CH59x_common.h"
#include "pairKey.h"

#include "central.h"
#include "Work_uart.h"
#include "protocol.h"
uint8_t RxBuff[100];

void Uart_Init(void)
{
    /* 配置串口1：先配置IO口模式，再配置串口 */
    GPIOA_SetBits(GPIO_Pin_14);
    GPIOA_ModeCfg(GPIO_Pin_15, GPIO_ModeIN_PU);      // RXD-配置上拉输入
    GPIOA_ModeCfg(GPIO_Pin_14, GPIO_ModeOut_PP_5mA); // TXD-配置推挽输出，注意先让IO口输出高电平
    GPIOPinRemap(ENABLE,RB_PIN_UART0);
    UART0_DefInit();

    UART0_ByteTrigCfg(UART_1BYTE_TRIG);
    UART0_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
    PFIC_EnableIRQ(UART0_IRQn);

}

/*********************************************************************
 * @fn      UART0_IRQHandler
 *
 * @brief   UART0中断函数
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void UART0_IRQHandler(void)
{
    uint8_t data;
    switch(UART0_GetITFlag())
    {
        case UART_II_LINE_STAT:
            UART0_GetLinSTA();
            break;
        case UART_II_RECV_RDY:
            data = UART0_RecvByte();
            UART_ParseByte(&uart1_packet,data);
            break;
        default:

            break;
    }

}
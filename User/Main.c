/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2020/08/06
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "hal.h"
#include "central.h"
#include "time.h"

/* 自定义库文件 */
#include "usb.h"
#include "pairKey.h"
#include "sys_time.h"
#include "iap.h"
#include "rgbled.h"
#include "Work_uart.h"
#include "protocol.h"
/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__ ((aligned (4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if (defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

// 短按断开连接
#define PAIR_KEY_DISCONNECT_TIME_MS 100
// 长按忘记设备（清除配对信息）
#define PAIR_KEY_FORGET_TIME_MS 1100

uint32_t pairKeyTime = 0;
uint32_t heartTime = 0;
bool tong1 = false;
bool tong2 = false;
bool low_battary = false;
/* ================= ADC 配置 ================= */

#define BATTERY_ADC_CHANNEL       3       /* PA13 = AIN3 */


volatile uint16_t battery_adc_raw = 0;
volatile uint8_t battery_adc_ready = 0;

void Battery_ADC_Init(void)
{
    /* 配置外部单通道 ADC */
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_1_4);

    /* 选择 PA13 对应的 ADC 通道 */
    ADC_ChannelCfg(BATTERY_ADC_CHANNEL);

    /* 关闭 DMA */
    ADC_DMACfg(DISABLE, 0, 0, ADC_Mode_Single);

    /* 清除可能残留的转换完成标志 */
    ADC_ClearITFlag();

    /* 开启 ADC 转换完成中断 */
    R8_ADC_CTRL_DMA |= RB_ADC_IE_EOC;

    /* 根据工程的中断框架启用 ADC_IRQn */
    PFIC_EnableIRQ(ADC_IRQn);
}



/* ================= ADC 中断 ================= */

__INTERRUPT
__HIGH_CODE
void ADC_IRQHandler(void)
{
    if (ADC_GetITStatus())
    {
        battery_adc_raw = R16_ADC_DATA & RB_ADC_DATA;
        ADC_ClearITFlag();
        battery_adc_ready = 1;
    }
}
/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   主循环
 *
 * @return  none
 */
__HIGH_CODE
__attribute__ ((noinline)) 
void Main_Circulation()  // 函数
{
    while (1)                                       // 程序
    {
        TMOS_SystemProcess();
        battery_adc_ready = 0;

        ADC_StartUp();

        while (!battery_adc_ready)
        {
            /* 等待 ADC 转换完成 */
        }
        if (battery_adc_raw <= 2840){
            low_battary = true;
            rgbled_setmod(5);
        }
        else{
            low_battary = false;
        }
        if (!central_isState() && !central_isDiscState())  // 全部空闲是扫描中
        {
            if (!tong1){
                Send_Packet(0x10,0,0,0);
                tong1 = true;
                tong2 = false;
            }
            // Send_Packet(0x10,0,0,0);
            if (isPeerAddrEmpty == 0){
                if (!low_battary){
                    rgbled_setmod(1);
                }
            }
            else{
                if (!low_battary){
                    rgbled_setmod(2);
                }
            }
        }                                
        else if (central_isDiscState())  // 连接中 慢速
        {
            if (!low_battary){
                rgbled_setmod(3);
            }
        }                         // ..
        else if (BLE_IS_READY())  // 已经连接
        {
        }

        uint8_t now = get_pairKey();

        if (now && !pairKeyTime)
        {
            pairKeyTime = millis();
        }                      //
        else if (pairKeyTime)  // 检查时间-意图
        {
            uint32_t holdTime = millis() - pairKeyTime;
            if (!now)                                // 松开检查
            {
                if (holdTime >= PAIR_KEY_FORGET_TIME_MS)  // 中等按压，忘记设备
                {

                    if (central_isState())  // 已连接
                    {
                        centralDisconnect();
                        Send_Packet(0x10,0,0,0);
                    }  // 重新扫描

#if USB_PRINT_DEBUG
                    USB_PRINT ("ERASE\n");
#endif              
                    EEPROM_ERASE (0, EEPROM_BLOCK_SIZE);
                    isPeerAddrEmpty = 0;
                    if (!low_battary){
                        rgbled_setmod(1);
                    }
                    pairKeyTime = 0;                               // 重置
                }                                                  //
                else if (holdTime >= PAIR_KEY_DISCONNECT_TIME_MS)  // 快速按压，断开连接
                {
                    if (central_isState())                         // 已连接
                    {
#if USB_PRINT_DEBUG
                        USB_PRINT ("-------/-----\n");
#endif
                        centralDisconnect();
                        Send_Packet(0x10,0,0,0);
                    }
                    else
                    {
                        startScanAgain();// 重新扫描
                    }
                    pairKeyTime = 0;
                }
                pairKeyTime = 0;
            }
        }
        if (millis() - heartTime >= 100)
        {
            heartTime = millis();

            Send_Packet(0x0A,0,0,0);

        }
        TMOS_SystemProcess();


        USB_IRQProcessHandler();


        TMOS_SystemProcess();
        Protocol_Process();
        TMOS_SystemProcess();
        // 重新检查
    }
}



/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main (void)
{

    SetSysClock (CLK_SOURCE_PLL_60MHz);

    InitUSBDevPara();
    InitCDCDevice();

    // PFIC_SetPriority (USB_IRQn, 0xf0);
    // PFIC_SetPriority (USB_IRQn, 0xf0); // 设置为较低优先级
    PFIC_EnableIRQ (USB_IRQn);
    
    Key_Init();

    CH59x_BLEInit();
    HAL_Init();
    GAPRole_CentralInit();
    Central_Init();

    rgbled_init();
    sys_time_init();  // 初始化 SysTick提供millis()
    Uart_Init();
    Protocol_Init();
    USBMatrix_ControlTask (1);
    Battery_ADC_Init();
    Main_Circulation();
}

/******************************** endfile @ main ******************************/

#include "rgbled.h"

#include "pairKey.h"


volatile uint8_t num = 0;
volatile uint8_t rgbmode = -1;


void rgbled_init() {
    
    GPIOB_ModeCfg(R_ledpin, GPIO_ModeOut_PP_5mA);
    GPIOB_SetBits(R_ledpin);
    GPIOA_ModeCfg(G_ledpin, GPIO_ModeOut_PP_5mA);
    GPIOA_SetBits(G_ledpin);
    GPIOA_ModeCfg(B_ledpin, GPIO_ModeOut_PP_5mA);
    GPIOA_SetBits(B_ledpin);

    TMR0_TimerInit(FREQ_SYS / 20);         // 设置定时时间 50ms
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END); // 开启中断
    PFIC_EnableIRQ(TMR0_IRQn);
}


void rgbled_setmod (uint8_t mode) {
    if(mode != rgbmode)
    {
        if(mode == 1){
            rgbmode = 1;
            GPIOB_ResetBits(R_ledpin);
            GPIOA_SetBits(G_ledpin);
            GPIOA_SetBits(B_ledpin);
            
        }
        if(mode == 5){
            rgbmode = 5;
            GPIOB_ResetBits(R_ledpin);
            GPIOA_SetBits(G_ledpin);
            GPIOA_SetBits(B_ledpin);
            
        }
        if(mode == 2){
            rgbmode = 2;
            GPIOB_SetBits(R_ledpin);
            GPIOA_ResetBits(G_ledpin);
            GPIOA_SetBits(B_ledpin);
        }
        if(mode == 3){
            rgbmode = 3;
            GPIOB_SetBits(R_ledpin);
            GPIOA_ResetBits(G_ledpin);
            GPIOA_SetBits(B_ledpin);
            
        }
        if(mode == 4){
            rgbmode = 4;
            GPIOB_SetBits(R_ledpin);
            GPIOA_ResetBits(G_ledpin);
            GPIOA_SetBits(B_ledpin);
            
        }
    }
}

/*********************************************************************
 * @fn      TMR0_IRQHandler
 *
 * @brief   TMR0中断函数
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void TMR0_IRQHandler(void) // TMR0 定时中断
{
    if(TMR0_GetITFlag(TMR0_3_IT_CYC_END))
    {
        TMR0_ClearITFlag(TMR0_3_IT_CYC_END); // 清除中断标志
        if (rgbmode == 5){
            GPIOB_InverseBits(R_ledpin);
        }
        if (rgbmode == 2){
            GPIOA_InverseBits(G_ledpin);
        }
        if (rgbmode == 3){
            num ++;
            if(num==3)
            {
                num = 0;
                GPIOA_InverseBits(G_ledpin);
            }
        }
        if (rgbmode == 1){
            num ++;
            if(num==3)
            {
                num = 0;
                GPIOB_InverseBits(R_ledpin);
            }
        }
    }
}

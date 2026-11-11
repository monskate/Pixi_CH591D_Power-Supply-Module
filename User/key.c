#include "pairKey.h"

#include "CH59xBLE_LIB.h"



void Key_Init (void)
{

    //KEY
    GPIOA_ModeCfg (GPIO_Pin_12, GPIO_ModeIN_PU);

}


// 获取 蓝牙配对按键状态 1:按下
uint8_t get_pairKey()
{
    // GPIOB_ModeCfg (GPIO_Pin_22, GPIO_ModeIN_PU);
    return (!GPIOA_ReadPortPin (GPIO_Pin_12));
}



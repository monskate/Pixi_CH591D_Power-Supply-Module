#ifndef __PAIRKEY_H__
#define __PAIRKEY_H__

#include "CH59x_common.h"

// 初始化GPIO口 
void Key_Init(void);


//获取 蓝牙配对按键状态 0:按下
uint8_t get_pairKey();


#endif

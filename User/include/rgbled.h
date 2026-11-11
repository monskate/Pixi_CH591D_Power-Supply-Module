#ifndef RGBLED_H
#define RGBLED_H

#include <stdint.h> 
#include <stdbool.h> 



#define R_ledpin    GPIO_Pin_7
#define G_ledpin    GPIO_Pin_8
#define B_ledpin    GPIO_Pin_9
/**
 * @brief 配置并初始化
 */
void rgbled_init();

/**
 * @brief 设置rgb灯闪烁模式
 * 
 * @param mode  1 无保留蓝牙设备        红灯慢闪
                2 扫描设备中            绿灯快闪
                3 连接设备中            绿灯慢闪
                4 已连接                绿灯常亮
                5 低电量                红灯快闪
 */
void rgbled_setmod(uint8_t mode);

#endif
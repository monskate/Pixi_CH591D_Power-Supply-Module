#include "sys_time.h"
#include "CH59x_common.h"

static volatile uint32_t systick_ms = 0;


// 如果需要 micros 支持，添加一个额外 SysTick 子计数
// static volatile uint32_t last_tick_val = 0;

//初始化
void sys_time_init(void)
{
    // 初始化为 1ms SysTick
    SysTick_Config(FREQ_SYS / 1000);  // 例如 60MHz 下 = 60000

    // // 开启中断清除，部分 SDK 可能需要写 SR
    // SysTick->SR = 0;
}




uint32_t millis(void)
{
    return systick_ms;
}

// uint32_t micros(void)
// {
//     return systick_ms * 1000 + (SysTick->CNT / (FREQ_SYS / 1000000));
// }

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void)
{
    ++systick_ms;

    SysTick->SR = 0;  // 清除中断标志位
}

#ifndef __SYS_TIME_H__
#define __SYS_TIME_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void sys_time_init(void);


uint32_t millis(void);
// uint32_t micros(void);

#ifdef __cplusplus
}
#endif

#endif

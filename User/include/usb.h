#ifndef _USB_CDC_H_
#define _USB_CDC_H_

// ≤‚ ‘
#define USB_PRINT_DEBUG 1
 
#include "CH59x_common.h"
#define USB_PRINT(...) USB_Print(__VA_ARGS__)



//#define USB_PRINT(...) PRINT(__VA_ARGS__)

UINT8 SendUSBData (UINT8 *p_send_dat, UINT16 send_len);
void InitUSBDevPara (void);
void InitCDCDevice (void);
void USBParaInit (void);
void USBDevEPnINSetStatus (UINT8 ep_num, UINT8 type, UINT8 sta);
void USB_IRQProcessHandler (void);

uint8_t *USB_GetPointerByType(int type);
uint8_t* USB_GetEp1DataOutFlag();
void USB_Print(const char *format, ...);

void USBMatrix_ControlTask(uint8_t enable);
void USB_StartTask (void);

void USB_PrintDisable(uint8_t lock);
#endif

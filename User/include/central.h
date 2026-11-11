/********************************** (C) COPYRIGHT *******************************
 * File Name          : central.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        : ?????????????????????????????????
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef CENTRAL_H
#define CENTRAL_H

#ifdef __cplusplus
extern "C" {
#endif



/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Simple BLE Observer Task Events
#define START_DEVICE_EVT              0x0001
#define START_DISCOVERY_EVT           0x0002
#define START_SCAN_EVT                0x0004
#define START_SVC_DISCOVERY_EVT       0x0008
#define START_PARAM_UPDATE_EVT        0x0010
#define START_PHY_UPDATE_EVT          0x0020
#define START_READ_OR_WRITE_EVT       0x0040
#define START_WRITE_CCCD_EVT          0x0080
#define START_READ_RSSI_EVT           0x0100
#define ESTABLISH_LINK_TIMEOUT_EVT    0x0200
// ??????????—¨????
#define TX_BUF_SIZE 7

// UUID
#define UUID_LEN_16 2
#define UUID_LEN_128 16

// BLE???????????? ???? ???????????
#define BLE_IS_READY()    ((central_isState() == 1) && (central_isDiscState() == 0))
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */
extern uint8_t isPeerAddrEmpty;
/*
 * Task Initialization for the BLE Application
 */
extern void Central_Init(void);

// extern void central_writeData (uint8_t *val, size_t size);
extern void central_sendData(const uint8_t *data, uint8_t size);

extern uint8_t central_isDiscState();
extern uint8_t central_isState();
extern void centralDisconnect(void);
extern void startScanAgain (void);
/*
 * Task Event Processor for the BLE Application
 */
extern uint16_t Central_ProcessEvent(uint8_t task_id, uint16_t events);
 
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif

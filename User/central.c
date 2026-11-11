/********************************** (C) COPYRIGHT *******************************
 * BLE peripheral role and custom GATT service.
 *******************************************************************************/

#include "CONFIG.h"
#include "central.h"
#include "CH59x_flash.h"
#include "protocol.h"
#include "usb.h"

#define BLE_DEVICE_NAME_LEN 6
#define BLE_VALUE_ATTR_INDEX 2
#define BLE_CCCD_ATTR_INDEX 3

static const uint8_t peripheralServiceUuid[ATT_UUID_SIZE] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x7E
};

static const uint8_t peripheralCharUuid[ATT_UUID_SIZE] = {
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x7E
};

static gattAttrType_t peripheralService = {ATT_UUID_SIZE, peripheralServiceUuid};
static uint8_t peripheralCharProperties = GATT_PROP_READ |
                                          GATT_PROP_WRITE_NO_RSP |
                                          GATT_PROP_WRITE |
                                          GATT_PROP_NOTIFY;
static uint8_t peripheralCharValue[TX_BUF_SIZE];
static gattCharCfg_t peripheralCharCfg[GATT_MAX_NUM_CONN];
static uint8_t peripheralTaskId;
static uint8_t peripheralConnected;
static uint8_t peripheralAdvertising;
static uint16_t peripheralConnHandle = GAP_CONNHANDLE_INIT;
static uint16_t peripheralMTU = ATT_MTU_SIZE;

uint8_t isPeerAddrEmpty = 0;

static uint8_t peripheralReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                    uint8_t *pValue, uint16_t *pLen,
                                    uint16_t offset, uint16_t maxLen,
                                    uint8_t method);
static uint8_t peripheralWriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                     uint8_t *pValue, uint16_t len,
                                     uint16_t offset, uint8_t method);
static void peripheralStateChangeCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);

static gattAttribute_t peripheralAttrTbl[] = {
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID},
        GATT_PERMIT_READ,
        0,
        (uint8_t *)&peripheralService
    },
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &peripheralCharProperties
    },
    {
        {ATT_UUID_SIZE, peripheralCharUuid},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        peripheralCharValue
    },
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8_t *)peripheralCharCfg
    }
};

static gattServiceCBs_t peripheralServiceCBs = {
    peripheralReadAttrCB,
    peripheralWriteAttrCB,
    NULL
};

static gapRolesCBs_t peripheralRoleCBs = {
    peripheralStateChangeCB,
    NULL,
    NULL
};

static gapBondCBs_t peripheralBondCBs = {NULL, NULL, NULL};

static uint8_t peripheralAdvData[] = {
    2, GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
    BLE_DEVICE_NAME_LEN + 1, GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'c', 'h', '0', '0', '0', '0',
    17, GAP_ADTYPE_128BIT_COMPLETE,
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x7E
};

static void peripheralBuildDeviceName(void)
{
    static const char hexDigits[] = "0123456789ABCDEF";
    uint32_t uniqueId[2];
    uint8_t *idBytes = (uint8_t *)uniqueId;
    uint8_t name[GAP_DEVICE_NAME_LEN] = {0};

    GET_UNIQUE_ID(idBytes);
    name[0] = 'c';
    name[1] = 'h';
    name[2] = hexDigits[(idBytes[6] >> 4) & 0x0F];
    name[3] = hexDigits[idBytes[6] & 0x0F];
    name[4] = hexDigits[(idBytes[7] >> 4) & 0x0F];
    name[5] = hexDigits[idBytes[7] & 0x0F];

    tmos_memcpy(&peripheralAdvData[5], name, BLE_DEVICE_NAME_LEN);
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, name);
    USB_PRINT("BLE name: %c%c%c%c%c%c\n",
              name[0], name[1], name[2], name[3], name[4], name[5]);
}

static uint8_t peripheralReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                    uint8_t *pValue, uint16_t *pLen,
                                    uint16_t offset, uint16_t maxLen,
                                    uint8_t method)
{
    uint16_t config;
    uint16_t copyLen;

    (void)method;
    if (pAttr == &peripheralAttrTbl[BLE_VALUE_ATTR_INDEX])
    {
        if (offset > sizeof(peripheralCharValue))
        {
            return ATT_ERR_INVALID_OFFSET;
        }

        copyLen = sizeof(peripheralCharValue) - offset;
        if (copyLen > maxLen)
        {
            copyLen = maxLen;
        }
        tmos_memcpy(pValue, &peripheralCharValue[offset], copyLen);
        *pLen = copyLen;
        return SUCCESS;
    }
    if (pAttr != &peripheralAttrTbl[BLE_CCCD_ATTR_INDEX])
    {
        return ATT_ERR_READ_NOT_PERMITTED;
    }
    if (offset > sizeof(config))
    {
        return ATT_ERR_INVALID_OFFSET;
    }

    config = GATTServApp_ReadCharCfg(connHandle, peripheralCharCfg);
    copyLen = sizeof(config) - offset;
    if (copyLen > maxLen)
    {
        copyLen = maxLen;
    }
    tmos_memcpy(pValue, ((uint8_t *)&config) + offset, copyLen);
    *pLen = copyLen;
    return SUCCESS;
}

static uint8_t peripheralWriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                     uint8_t *pValue, uint16_t len,
                                     uint16_t offset, uint8_t method)
{
    uint16_t index;

    (void)method;
    if (pAttr == &peripheralAttrTbl[BLE_CCCD_ATTR_INDEX])
    {
        uint8_t status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                        offset, GATT_CLIENT_CFG_NOTIFY);
        USB_PRINT("BLE CCCD write: status=0x%02X, value=0x%04X\n",
                  status, GATTServApp_ReadCharCfg(connHandle, peripheralCharCfg));
        return status;
    }
    if (pAttr != &peripheralAttrTbl[BLE_VALUE_ATTR_INDEX])
    {
        return ATT_ERR_WRITE_NOT_PERMITTED;
    }
    if (offset != 0)
    {
        return ATT_ERR_INVALID_OFFSET;
    }
    if (len > peripheralMTU - 3)
    {
        return ATT_ERR_INVALID_VALUE_SIZE;
    }
    USB_PRINT("BLE RX len=%u data:", (unsigned int)len);
    for (uint16_t rxIndex = 0; rxIndex < len; rxIndex++)
    {
        USB_PRINT(" %02X", pValue[rxIndex]);
    }
    USB_PRINT("\n");
    if (len == 1 && uart1_packet.state == WAIT_AA && pValue[0] != 0xAA)
    {
        USB_PRINT("BLE RX ACK: 0x%02X\n", pValue[0]);
        Protocol_BLE_ACK(pValue[0]);
        return SUCCESS;
    }

    USB_PRINT("BLE RX protocol data\n");
    for (index = 0; index < len; index++)
    {
        UART_ParseByte(&uart1_packet, pValue[index]);
    }
    return SUCCESS;
}

static void peripheralStateChangeCB(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
    uint8_t state = newState & GAPROLE_STATE_ADV_MASK;

    if (state == GAPROLE_CONNECTED && pEvent != NULL &&
        pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
    {
        peripheralConnHandle = pEvent->linkCmpl.connectionHandle;
        peripheralConnected = TRUE;
        peripheralAdvertising = FALSE;
        USB_PRINT("BLE connected, handle: 0x%04X\n", peripheralConnHandle);
    }
    else if (pEvent != NULL && pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT &&
             pEvent->linkTerminate.connectionHandle == peripheralConnHandle)
    {
        USB_PRINT("BLE disconnected, reason: 0x%02X\n", pEvent->linkTerminate.reason);
        GATTServApp_InitCharCfg(peripheralConnHandle, peripheralCharCfg);
        peripheralConnHandle = GAP_CONNHANDLE_INIT;
        peripheralConnected = FALSE;
        peripheralAdvertising = FALSE;
        peripheralMTU = ATT_MTU_SIZE;

        uint8_t advertisingEnabled = TRUE;
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(advertisingEnabled),
                             &advertisingEnabled);
    }
    else if (state == GAPROLE_STARTED)
    {
        USB_PRINT("BLE peripheral initialized\n");
    }
    else if (state == GAPROLE_ADVERTISING && pEvent != NULL &&
             pEvent->gap.opcode == GAP_MAKE_DISCOVERABLE_DONE_EVENT)
    {
        peripheralAdvertising = TRUE;
        USB_PRINT("BLE advertising\n");
    }
    else if (state == GAPROLE_WAITING && pEvent != NULL &&
             pEvent->gap.opcode == GAP_END_DISCOVERABLE_DONE_EVENT)
    {
        USB_PRINT("BLE waiting for advertising\n");
    }
    else if (state == GAPROLE_ERROR)
    {
        USB_PRINT("BLE role error\n");
    }
}

uint8_t central_isState(void)
{
    return peripheralConnected;
}

uint8_t central_isDiscState(void)
{
    return peripheralAdvertising;
}

void centralDisconnect(void)
{
    if (peripheralConnected && peripheralConnHandle != GAP_CONNHANDLE_INIT)
    {
        GAPRole_TerminateLink(peripheralConnHandle);
    }
}

void startScanAgain(void)
{
    uint8_t advertisingEnabled = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(advertisingEnabled),
                         &advertisingEnabled);
}

void central_sendData(const uint8_t *data, uint8_t size)
{
    attHandleValueNoti_t notification;
    uint8_t *notificationValue;
    uint16_t cccdValue;
    bStatus_t status;

    if (!peripheralConnected || peripheralConnHandle == GAP_CONNHANDLE_INIT)
    {
        USB_PRINT("BLE notify skipped: not connected\n");
        return;
    }
    if (data == NULL || size == 0 || size > sizeof(peripheralCharValue))
    {
        USB_PRINT("BLE notify skipped: invalid length %u\n", (unsigned int)size);
        return;
    }

    cccdValue = GATTServApp_ReadCharCfg(peripheralConnHandle, peripheralCharCfg);
    if (!(cccdValue & GATT_CLIENT_CFG_NOTIFY))
    {
        USB_PRINT("BLE notify skipped: phone Notify disabled, CCCD=0x%04X\n", cccdValue);
        return;
    }

    tmos_memcpy(peripheralCharValue, data, size);
    notificationValue = GATT_bm_alloc(peripheralConnHandle, ATT_HANDLE_VALUE_NOTI,
                                      size, NULL, 0);
    if (notificationValue == NULL)
    {
        USB_PRINT("BLE notify skipped: GATT buffer allocation failed\n");
        return;
    }
    tmos_memcpy(notificationValue, data, size);
    notification.handle = peripheralAttrTbl[BLE_VALUE_ATTR_INDEX].handle;
    notification.len = size;
    notification.pValue = notificationValue;
    USB_PRINT("BLE notify payload:");
    for (uint8_t index = 0; index < size; index++)
    {
        USB_PRINT(" %02X", notificationValue[index]);
    }
    USB_PRINT("\n");
    status = GATT_Notification(peripheralConnHandle, &notification, FALSE);
    USB_PRINT("BLE notify: status=0x%02X, handle=0x%04X, len=%u\n",
              status, notification.handle, (unsigned int)notification.len);
    if (status != SUCCESS)
    {
        GATT_bm_free((gattMsg_t *)&notification, ATT_HANDLE_VALUE_NOTI);
    }
}

void Central_Init(void)
{
    uint8_t pairingMode = GAPBOND_PAIRING_MODE_NO_PAIRING;
    uint8_t advertisingEnabled = TRUE;
    uint8_t advertisingType = GAP_ADTYPE_ADV_IND;

    peripheralTaskId = TMOS_ProcessEventRegister(Central_ProcessEvent);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, peripheralCharCfg);

    GGS_AddService(GATT_ALL_SERVICES);
    GATTServApp_AddService(GATT_ALL_SERVICES);
    peripheralBuildDeviceName();
    GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(pairingMode), &pairingMode);
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(peripheralAdvData), peripheralAdvData);
    GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(advertisingType), &advertisingType);
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(advertisingEnabled), &advertisingEnabled);
    GATTServApp_RegisterService(peripheralAttrTbl, GATT_NUM_ATTRS(peripheralAttrTbl),
                                GATT_MAX_ENCRYPT_KEY_SIZE, &peripheralServiceCBs);

    tmos_set_event(peripheralTaskId, START_DEVICE_EVT);
}

uint16_t Central_ProcessEvent(uint8_t task_id, uint16_t events)
{
    (void)task_id;
    if (events & SYS_EVENT_MSG)
    {
        uint8_t *message = tmos_msg_receive(peripheralTaskId);
        if (message != NULL)
        {
            tmos_event_hdr_t *header = (tmos_event_hdr_t *)message;
            if (header->event == GATT_MSG_EVENT)
            {
                gattMsgEvent_t *gattMessage = (gattMsgEvent_t *)message;
                if (gattMessage->method == ATT_MTU_UPDATED_EVENT)
                {
                    peripheralMTU = gattMessage->msg.mtuEvt.MTU;
                }
            }
            tmos_msg_deallocate(message);
        }
        return events ^ SYS_EVENT_MSG;
    }
    if (events & START_DEVICE_EVT)
    {
        GAPRole_PeripheralStartDevice(peripheralTaskId, &peripheralBondCBs, &peripheralRoleCBs);
        return events ^ START_DEVICE_EVT;
    }
    return 0;
}

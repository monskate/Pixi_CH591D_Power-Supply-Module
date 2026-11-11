/********************************** (C) COPYRIGHT *******************************
* File Name          : central.c
* Author             : WCH
* Version            : V1.1
* Date               : 2020/08/06
* Description        : 中央角色处理文件，用于扫描周围设备，并连接目标设备地址，
*                      寻找并自动执行各类读写操作，连接目标设备后可通过蓝牙
*                      进行读写。默认目标设备地址为(84:C2:E4:03:02:02)
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "gattprofile.h"
#include "central.h"


// 自定义模块
#include "usb.h"       // USB 调试输出
#include "sys_time.h"  // 获取时间 millis()
#include "rgbled.h"
#include "protocol.h"
/*********************************************************************
 * MACROS
 */

// BD 地址字符串长度，设备地址为 6 字节
#define B_ADDR_STR_LEN 15


/*********************************************************************
 * 参数配置
 */

// 最大可扫描到的设备数量
#define DEFAULT_MAX_SCAN_RES 5

// 是否在扫描失败后再次扫描
#define ENABLE_SCAN_RETRY 1

// 扫描时长，单位 0.625 ms
#define DEFAULT_SCAN_DURATION 2400

// 最小连接间隔，单位 1.25 ms
#define DEFAULT_MIN_CONNECTION_INTERVAL 20

// 最大连接间隔，单位 1.25 ms
#define DEFAULT_MAX_CONNECTION_INTERVAL 100

// 连接超时时间，单位 10 ms
#define DEFAULT_CONNECTION_TIMEOUT 100

// 设备发现模式，默认：全部设备
#define DEFAULT_DISCOVERY_MODE DEVDISC_MODE_ALL

// 是否使用主动扫描（TRUE 表示启用）
#define DEFAULT_DISCOVERY_ACTIVE_SCAN TRUE

// 是否在发现过程中使用白名单
#define DEFAULT_DISCOVERY_WHITE_LIST FALSE

// 连接时是否使用高占空比扫描
#define DEFAULT_LINK_HIGH_DUTY_CYCLE FALSE

// 连接时是否使用白名单
#define DEFAULT_LINK_WHITE_LIST FALSE

// 默认读取 RSSI 的时间窗口，单位 0.625 ms
#define DEFAULT_RSSI_PERIOD 2400

// 最小参数更新连接间隔，单位 1.25 ms
#define DEFAULT_UPDATE_MIN_CONN_INTERVAL 20

// 最大参数更新连接间隔，单位 1.25 ms
#define DEFAULT_UPDATE_MAX_CONN_INTERVAL 100

// 从设备延迟，表示从机会在多少个连接事件中跳过响应
#define DEFAULT_UPDATE_SLAVE_LATENCY 0

// 参数更新后的监督超时时间，单位 10 ms
#define DEFAULT_UPDATE_CONN_TIMEOUT 600


// 默认 GAP 配对模式：等待请求
#define DEFAULT_PAIRING_MODE GAPBOND_PAIRING_MODE_WAIT_FOR_REQ

// 默认 MITM 保护模式：TRUE 表示要求中间人保护
#define DEFAULT_MITM_MODE TRUE

// 默认绑定模式：TRUE 表示启用绑定，最多支持 6 台设备
#define DEFAULT_BONDING_MODE TRUE

// 默认 GAP IO 能力配置
#define DEFAULT_IO_CAPABILITIES GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT

// 服务发现延时，单位 0.625 ms
#define DEFAULT_SVC_DISCOVERY_DELAY 1600

// 连接参数更新延时，单位 0.625 ms
#define DEFAULT_PARAM_UPDATE_DELAY 3200

// PHY 更新延时，单位 0.625 ms
#define DEFAULT_PHY_UPDATE_DELAY 2400

// 读写前等待时间，单位 0.625 ms
#define DEFAULT_READ_OR_WRITE_DELAY 1600

// 写 CCCD（Client Characteristic Configuration Descriptor）时的延时，单位：0.625 ms
#define DEFAULT_WRITE_CCCD_DELAY 1600

// 建立连接超时时间，单位：0.625 ms
#define ESTABLISH_LINK_TIMEOUT 4200


// BLE 连接建立超时时间，单位：毫秒
#define BLE_CONNECT_TIMEOUT_MS 10000  // 超时（毫秒）

// 应用当前状态
enum
{
    BLE_STATE_IDLE,       // 空闲状态
    BLE_STATE_CONNECTED,  // 已连接
};
extern bool tong1;
extern bool tong2;
extern bool low_battary;
// 发现流程状态
enum
{
    BLE_DISC_STATE_IDLE,  // 空闲状态
    BLE_DISC_STATE_SVC,   // 发现服务
    BLE_DISC_STATE_CHAR,  // 发现特征
    BLE_DISC_STATE_CCCD   // 发现 CCCD（客户端特征配置描述符）
};

#define ATT_PROP_BROADCAST 0x01          // 属性：广播（Broadcast）
#define ATT_PROP_READ 0x02               // 属性：可读（Read）
#define ATT_PROP_WRITE_NO_RSP 0x04       // 属性：支持无响应写（Write Without Response）
#define ATT_PROP_WRITE 0x08              // 属性：支持带响应写（Write With Response）
#define ATT_PROP_NOTIFY 0x10             // 属性：支持通知（Notification）
#define ATT_PROP_INDICATE 0x20           // 属性：支持指示（Indication）
#define ATT_PROP_AUTH_SIGNED_WRITE 0x40  // 属性：支持认证签名写（Authenticated Signed Write）


/*********************************************************************
 * LOCAL VARIABLES
 */
// 任务 ID / 事件处理器标识
static uint8_t centralTaskId;

// 扫描结果计数
static uint8_t centralScanRes;

// 扫描到的设备列表
static gapDevRec_t centralDevList[DEFAULT_MAX_SCAN_RES];

// 扫描到的设备列表中包含的 UUID 信息
typedef struct
{
    uint8_t uuid[UUID_LEN_128];  // UUID 内容
    uint8_t uuid_len;            // UUID 长度，单位：字节
} DeviceUUIDInfo_t;

static DeviceUUIDInfo_t scannedDeviceUUIDs[DEFAULT_MAX_SCAN_RES];

// 目标设备的地址
static uint8_t PeerAddrDef[B_ADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// 目标设备的地址是否为 0，若为 0，则表示未设置
uint8_t isPeerAddrEmpty = 0;

// 目标设备 UUID  UUID: 0000fff0-0000-1000-8000-00805f9b34fb
static uint8_t PeerUUIDdef[UUID_LEN_128] = {0xfb, 0x34, 0x9b, 0x5f,
                                            0x80, 0x00, 0x00, 0x80,
                                            0x00, 0x10, 0x00, 0x00,
                                            0xf0, 0xff, 0x00, 0x00};

// 服务 UUID: 0000fff1-0000-1000-8000-00805f9b34fb
static uint8_t service_char_uuid[UUID_LEN_128] = {
    0xfb, 0x34, 0x9b, 0x5f,
    0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00,
    0xf1, 0xff, 0x00, 0x00};

// 是否读取 RSSI 查询
static uint8_t centralRssi = false;

// 是否执行参数更新
static uint8_t centralParamUpdate = TRUE;

// 是否执行 PHY 更新
static uint8_t centralPhyUpdate = TRUE;

// 当前连接的句柄
static uint16_t centralConnHandle = GAP_CONNHANDLE_INIT;

// 应用当前状态
static uint8_t centralState = BLE_STATE_IDLE;

// 当前发现状态
static uint8_t centralDiscState = BLE_DISC_STATE_IDLE;

// 已发现服务的起始句柄
static uint16_t centralSvcStartHdl = 0;

// 已发现服务的结束句柄
static uint16_t centralSvcEndHdl = 0;

// 已发现特征的句柄
static uint16_t centralCharHdl = 0;

// 已发现的客户端配置描述符句柄（CCCD）
static uint16_t centralCCCDHdl = 0;

// 待写入的数据信息
uint8_t device_info_buf[TX_BUF_SIZE] = {0};
size_t device_info_len = 0;
// 当前是否执行写入操作，TRUE 表示写入，FALSE 表示读取
static uint8_t centralDoWrite = TRUE;

// GATT 写操作是否正在进行
static uint8_t centralProcedureInProgress = FALSE;

// 当前连接开始时间，用于判断是否超时
static uint32_t bleConnectStartTime = 0;
// 最近一次 BLE 写操作开始时间，如果未收到响应，则超时后可重置
static uint32_t lastWriteStartTime = 0;
// 当前 ATT 特征属性
static uint8_t centralCharProperties = 0;
/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void centralProcessGATTMsg (gattMsgEvent_t *pMsg);
static void centralRssiCB (uint16_t connHandle, int8_t rssi);
static void centralEventCB (gapRoleEvent_t *pEvent);
static void centralHciMTUChangeCB (uint16_t connHandle, uint16_t maxTxOctets, uint16_t maxRxOctets);
// static void centralPasscodeCB (uint8_t *deviceAddr, uint16_t connectionHandle,uint8_t uiInputs, uint8_t uiOutputs);
static void centralPairStateCB (uint16_t connHandle, uint8_t state, uint8_t status);
static void central_ProcessTMOSMsg (tmos_event_hdr_t *pMsg);
static void centralGATTDiscoveryEvent (gattMsgEvent_t *pMsg);
static void centralStartDiscovery (void);
static void centralAddDeviceInfo (uint8_t *pAddr, uint8_t addrType);
// 从广播数据中解析 16-bit/128-bit UUID，成功返回 0，失败返回 -1
int central_ParseUUIDFromAdvData (uint8_t *pData, uint8_t dataLen, DeviceUUIDInfo_t *uuidBuf);

/*
    获取连接状态
    0: 空闲状态
    1: 已连接
*/
uint8_t central_isState()
{
    return centralState;

    // GAPRole_TerminateLink
}

/*
    获取发现状态
    0: 空闲状态
    1: 正在发现服务
    2: 正在发现特征
    3: 正在发现 CCCD
*/
uint8_t central_isDiscState()
{
    return centralDiscState;
}

// 主动断开连接
void centralDisconnect (void)
{
#if USB_PRINT_DEBUG
    USB_PRINT ("Disconnect\n");
#endif

    if (centralState == BLE_STATE_CONNECTED && centralConnHandle != GAP_CONNHANDLE_INIT)
    {
        GAPRole_TerminateLink (centralConnHandle);  // 断开当前连接
        centralConnHandle = GAP_CONNHANDLE_INIT;
    }
}

// 重新开始扫描
void startScanAgain (void)
{

    GAPRole_CentralStartDiscovery (  // 重新开始扫描
        DEFAULT_DISCOVERY_MODE,
        DEFAULT_DISCOVERY_ACTIVE_SCAN,
        DEFAULT_DISCOVERY_WHITE_LIST);

}

// 发送一串十六进制数据到特征
__HIGH_CODE
void central_sendHexToChar (size_t size)
{
    if ((centralState == BLE_STATE_CONNECTED && !centralDiscState))
    {
        device_info_len = size;
        centralDoWrite = TRUE;                                        // 第一次写入
        tmos_start_task (centralTaskId, START_READ_OR_WRITE_EVT, 0);  // 触发写入事件
    }
}

// GAP 中心角色回调结构
static gapCentralRoleCB_t centralRoleCB = {
    centralRssiCB,         // RSSI 回调，用于处理信号强度变化
    centralEventCB,        // 事件回调，用于处理 GAP 事件，例如连接、断开等
    centralHciMTUChangeCB  // HCI MTU 变化回调，用于通知 MTU 值发生变化
};

// 配对状态回调结构
static gapBondCBs_t centralBondCB = {
    NULL,               // centralPasscodeCB,  // 配对时的 passcode 回调
    centralPairStateCB  // 配对状态回调，用于处理配对状态变化
};

/*********************************************************************
 * PUBLIC FUNCTIONS - 公共功能
 */

/*********************************************************************
 * @fn      Central_Init
 * @brief   Central 模块初始化
 *          该函数在初始化期间调用，应处理任何应用相关的初始化逻辑，
 *          如硬件初始化、资源初始化、GATT 客户端注册等。
 * @param   task_id - TMOS 任务 ID
 *                    该 ID 用于注册事件和消息处理
 * @return  无
 */
void Central_Init()
{
    // 注册事件处理器，获取任务 ID
    centralTaskId = TMOS_ProcessEventRegister (Central_ProcessEvent);

#if USB_PRINT_DEBUG
    USB_PRINT ("\n---->EEPROM_MAC:");
#endif
    // 从 EEPROM 起始地址 0 读取 B_ADDR_LEN 字节 MAC 地址
    EEPROM_READ (0, PeerAddrDef, B_ADDR_LEN);
    uint8_t i, bo = 0;

    isPeerAddrEmpty = 1;  // 初始认为地址已设置


    // 打印读取到的数据；若为 0xFF 或 0x00 则表示空
    for (i = 0; i < B_ADDR_LEN; i++)
    {
#if USB_PRINT_DEBUG
        USB_PRINT ("%02x ", PeerAddrDef[i]);
#endif
        if (PeerAddrDef[i] == 0xff || PeerAddrDef[i] == 0x00)
            bo++;
    }
#if USB_PRINT_DEBUG
    USB_PRINT ("\n");
#endif


    if (bo >= i)
    {
        isPeerAddrEmpty = 0;  // 地址为空
        if (!low_battary)
            rgbled_setmod(1);
    }


    // 配置 GAP 参数
    GAP_SetParamValue (TGAP_DISC_SCAN, DEFAULT_SCAN_DURATION);                     // 扫描时长
    GAP_SetParamValue (TGAP_CONN_EST_INT_MIN, DEFAULT_MIN_CONNECTION_INTERVAL);    // 最小连接间隔
    GAP_SetParamValue (TGAP_CONN_EST_INT_MAX, DEFAULT_MAX_CONNECTION_INTERVAL);    // 最大连接间隔
    GAP_SetParamValue (TGAP_CONN_EST_SUPERV_TIMEOUT, DEFAULT_CONNECTION_TIMEOUT);  // 连接超时时间

    // 初始化 GAP 配对参数
    {
        // 设置为无配对模式，在需要时可改为需要输入密码
        uint8_t pairMode = GAPBOND_PAIRING_MODE_NO_PAIRING;
        GAPBondMgr_SetParameter (GAPBOND_CENT_PAIRING_MODE, sizeof (uint8_t), &pairMode);

        // 关闭 MITM 中间人保护
        uint8_t mitm = FALSE;
        GAPBondMgr_SetParameter (GAPBOND_CENT_MITM_PROTECTION, sizeof (uint8_t), &mitm);

        // 设置 IO 能力为无输入无输出
        uint8_t ioCap = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
        GAPBondMgr_SetParameter (GAPBOND_CENT_IO_CAPABILITIES, sizeof (uint8_t), &ioCap);

        // 关闭绑定，避免存储配对信息
        uint8_t bonding = FALSE;
        GAPBondMgr_SetParameter (GAPBOND_CENT_BONDING_ENABLED, sizeof (uint8_t), &bonding);
    }

    // 初始化 GATT 客户端
    GATT_InitClient();
    // 注册 ATT 通知/指示
    GATT_RegisterForInd (centralTaskId);
    // 触发设备初始化事件
    tmos_set_event (centralTaskId, START_DEVICE_EVT);
}

// *Central_ProcessEvent (事件处理)
//    处理连接、写 CCCD、写特征等基础任务流程
//    注意：
//    事件队列、初始化流程和自动发现/连接
//
// 事件队列接收响应通知
//    说明：
//    TMOS 系统消息由 central_ProcessTMOSMsg 处理
//    其中：
//    central_ProcessTMOSMsg 负责分发到 centralProcessGATTMsg
//    处理完成后，返回并更新状态


/*********************************************************************
 * @fn      Central_ProcessEvent
 * @brief   Central 事件处理入口
 *          该函数处理各种事件，包括系统消息、连接状态变化、GATT 读写，以及用户自定义事件。
 * @param   task_id - 任务 ID，由 TMOS 分配。
 * @param   events - 需要处理的事件位图，可包含多个事件标志。
 * @return  未处理的事件位图
 */
uint16_t Central_ProcessEvent (uint8_t task_id, uint16_t events)
{
    // 处理系统消息事件
    if (events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        // 处理 TMOS 消息
        if ((pMsg = tmos_msg_receive (centralTaskId)) != NULL)
        {
            // 处理接收到的 TMOS 消息
            central_ProcessTMOSMsg ((tmos_event_hdr_t *)pMsg);
            // 释放消息内存
            tmos_msg_deallocate (pMsg);
        }
        // 返回未处理事件
        return (events ^ SYS_EVENT_MSG);
    }

    // 设备启动事件
    if (events & START_DEVICE_EVT)
    {
        // 启动 Central 模式设备并注册 Bond / GAP 回调
        GAPRole_CentralStartDevice (centralTaskId, &centralBondCB, &centralRoleCB);
        return (events ^ START_DEVICE_EVT);
    }

    // 建立连接超时事件
    if (events & ESTABLISH_LINK_TIMEOUT_EVT)
    {
        // 强制断开连接
        GAPRole_TerminateLink (INVALID_CONNHANDLE);
        return (events ^ ESTABLISH_LINK_TIMEOUT_EVT);
    }

    // 服务发现事件
    if (events & START_SVC_DISCOVERY_EVT)
    {
        // 开始服务发现
        centralStartDiscovery();
        return (events ^ START_SVC_DISCOVERY_EVT);
    }

    // 连接参数更新事件
    if (events & START_PARAM_UPDATE_EVT)
    {

        /*
         * 更新连接参数以优化链路性能。
         * @param centralConnHandle          当前连接的句柄，作为唯一标识。
         * @param DEFAULT_UPDATE_MIN_CONN_INTERVAL    最小连接间隔，单位 1.25 ms。
         * @param DEFAULT_UPDATE_MAX_CONN_INTERVAL    最大连接间隔，单位 1.25 ms。
         * @param DEFAULT_UPDATE_SLAVE_LATENCY        从机延迟，表示从机会在多少个连接事件中跳过响应。
         * @param DEFAULT_UPDATE_CONN_TIMEOUT         连接超时时间，单位 10 ms。
         *
         * 该函数用于在连接建立后动态更新连接参数，以改善通信响应时间和稳定性。
         * 连接参数与底层协议栈和实际硬件有关，需根据应用需求进行调整。
         */
        GAPRole_UpdateLink (centralConnHandle,
                            DEFAULT_UPDATE_MIN_CONN_INTERVAL,
                            DEFAULT_UPDATE_MAX_CONN_INTERVAL,
                            DEFAULT_UPDATE_SLAVE_LATENCY,
                            DEFAULT_UPDATE_CONN_TIMEOUT);
        return (events ^ START_PARAM_UPDATE_EVT);
    }

    // PHY 更新事件
    if (events & START_PHY_UPDATE_EVT)
    {
        uint8_t phys = GAPRole_UpdatePHY (centralConnHandle, 0,
                                          GAP_PHY_BIT_LE_2M, GAP_PHY_BIT_LE_2M, 0);
#if USB_PRINT_DEBUG
        // 在 PHY 更新完成后，打印当前使用的 PHY 方式（如 2M PHY）
        USB_PRINT ("PHY Update %x...\n", phys);
#endif
        return (events ^ START_PHY_UPDATE_EVT);
    }

    // 读写特征值事件-------------------
    if (events & START_READ_OR_WRITE_EVT)
    {
        if (centralProcedureInProgress == TRUE)
        {
            if ((millis() - lastWriteStartTime) > 2000UL)
            {
#if USB_PRINT_DEBUG
                USB_PRINT ("Write timeout, clear pending state\n");
#endif
                centralProcedureInProgress = FALSE;
                lastWriteStartTime = 0;
            }
            else
            {
                return (events ^ START_READ_OR_WRITE_EVT);
            }
        }

        if (centralDoWrite)
        {
            attWriteReq_t req;
            if (centralCharProperties & ATT_PROP_WRITE_NO_RSP)  // 无应答写（Write Without Response）
            {
                if (device_info_len > 0)
                {
                    req.cmd = TRUE;   // 标志：表示 Write Without Response
                    req.sig = FALSE;  // 签名标志
                    req.handle = centralCharHdl;
                    req.len = device_info_len;
                    req.pValue = GATT_bm_alloc (centralConnHandle, ATT_WRITE_CMD, req.len, NULL, 0);

                    if (req.pValue != NULL)
                    {
                        tmos_memcpy (req.pValue, device_info_buf, req.len);

                        uint8_t status = GATT_WriteNoRsp (centralConnHandle, &req);  // 发送无响应写请求
                        if (status == SUCCESS)
                        {
                            centralProcedureInProgress = TRUE;
                            lastWriteStartTime = millis();
                            centralDoWrite = !centralDoWrite;
                            tmos_start_task (centralTaskId, START_READ_OR_WRITE_EVT, DEFAULT_READ_OR_WRITE_DELAY);
                        }
                        else
                        {
#if USB_PRINT_DEBUG
                            USB_PRINT ("GATT_WriteNoRsp failed: 0x%02x\n", status);
#endif
                            GATT_bm_free ((gattMsg_t *)&req, ATT_WRITE_CMD);  // 发送失败时释放缓冲
                        }

                        device_info_len = 0;
                    }
#if USB_PRINT_DEBUG
                    else
                    {

                        USB_PRINT ("GATT_bm_alloc failed\n");
                    }
#endif
                }
            }                                                  // 写入处理分支结束
            else if (centralCharProperties & ATT_PROP_WRITE)  // 有响应写（Write With Response）
            {
                req.cmd = FALSE;
                req.sig = FALSE;

                req.handle = centralCharHdl;                                                      // 目标特征句柄
                req.len = device_info_len;
                req.pValue = GATT_bm_alloc (centralConnHandle, ATT_WRITE_REQ, req.len, NULL, 0);  // 申请写缓冲
                if (req.pValue != NULL && req.len)
                {                                                                                 // ����д���ֵ
                    tmos_memcpy (req.pValue, device_info_buf, req.len);                           // 将数据复制到 GATT 缓冲区

                    if (GATT_WriteCharValue (centralConnHandle, &req, centralTaskId) == SUCCESS)
                    {
                        centralProcedureInProgress = TRUE;                                                      // 标记写入过程正在进行
                        lastWriteStartTime = millis();
                        tmos_start_task (centralTaskId, START_READ_OR_WRITE_EVT, DEFAULT_READ_OR_WRITE_DELAY);  // 延时后再次触发读写事件
                    }
                    else
                    {
                        GATT_bm_free ((gattMsg_t *)&req, ATT_WRITE_REQ);  // 写失败时释放缓冲
                    }

                    device_info_len = 0;  // 清除数据长度
                }
            }
        }     //......
        return (events ^ START_READ_OR_WRITE_EVT);
    }

    // 写 CCCD 事件，用于开启 Notify
    if (events & START_WRITE_CCCD_EVT)
    {
        if (centralProcedureInProgress == FALSE)
        {
            attWriteReq_t req;

            req.cmd = FALSE;
            req.sig = FALSE;
            req.handle = centralCCCDHdl;  // CCCD 句柄
            req.len = 2;
            req.pValue = GATT_bm_alloc (centralConnHandle, ATT_WRITE_REQ, req.len, NULL, 0);
            if (req.pValue != NULL)
            {
                req.pValue[0] = 1;  // 启用 Notify (0x0001)
                req.pValue[1] = 0;

                if (GATT_WriteCharValue (centralConnHandle, &req, centralTaskId) == SUCCESS)
                {
                    centralProcedureInProgress = TRUE;
                }
                else
                {
                    GATT_bm_free ((gattMsg_t *)&req, ATT_WRITE_REQ);
                }
            }
        }
        return (events ^ START_WRITE_CCCD_EVT);
    }

    // RSSI 读取事件
    if (events & START_READ_RSSI_EVT)
    {
        GAPRole_ReadRssiCmd (centralConnHandle);                                    // 读取 RSSI
        tmos_start_task (centralTaskId, START_READ_RSSI_EVT, DEFAULT_RSSI_PERIOD);  // 周期性读取 RSSI
        return (events ^ START_READ_RSSI_EVT);
    }

    // 未识别的事件
    return 0;
}

/*********************************************************************
 * @fn      central_ProcessTMOSMsg
 *
 * @brief   处理接收到的 TMOS 消息，并转发到底层协议栈。
 *
 * @param   pMsg - 传入的消息指针
 *
 * @return  无
 */
static void central_ProcessTMOSMsg (tmos_event_hdr_t *pMsg)
{
    // 事件分发，根据消息类型执行对应处理
    switch (pMsg->event)
    {
        case GATT_MSG_EVENT:                                 // GATT 事件，包含写入响应、通知等
            centralProcessGATTMsg ((gattMsgEvent_t *)pMsg);  // 转发到 GATT 消息处理函数
            break;
    }
}

/*********************************************************************
 * @fn      centralProcessGATTMsg
 *
 * @brief   处理 GATT 相关事件，包括写入响应、通知、MTU 协议等。
 *
 * @param   pMsg - GATT 事件结构体指针
 *
 * @return  无
 */
static void centralProcessGATTMsg (gattMsgEvent_t *pMsg)
{
    // 如果当前设备未处于已连接状态，则忽略 GATT 消息并释放资源
    if (centralState != BLE_STATE_CONNECTED)
    {
        // 防止在断开连接后仍收到有效 GATT 消息，导致内存泄漏
        GATT_bm_free (&pMsg->msg, pMsg->method);
        return;
    }

    // 判断是否为 MTU 协商响应或错误响应
    if ((pMsg->method == ATT_EXCHANGE_MTU_RSP) ||
        ((pMsg->method == ATT_ERROR_RSP) &&
         (pMsg->msg.errorRsp.reqOpcode == ATT_EXCHANGE_MTU_REQ)))
    {
        if (pMsg->method == ATT_ERROR_RSP)
        {
#if USB_PRINT_DEBUG
            uint8_t status = pMsg->msg.errorRsp.errCode;
            USB_PRINT ("Exchange MTU Error: %x\n", status);  // 打印 MTU 协商错误信息
#endif
        }
        // 无论成功或失败，清除当前过程标志
        centralProcedureInProgress = FALSE;


        // 如果连接仍处于已连接状态，则触发后续服务发现事件
        if (centralState == BLE_STATE_CONNECTED)
        {
            tmos_set_event (centralTaskId, START_SVC_DISCOVERY_EVT);
        }
    }
#if USB_PRINT_DEBUG
    // 当收到 MTU 更新事件时，打印双方协商后的 MTU 值
    if (pMsg->method == ATT_MTU_UPDATED_EVENT)
    {

        USB_PRINT ("MTU: %x\n", pMsg->msg.mtuEvt.MTU);  // 打印当前协商的 MTU 值
    }
#endif
    // 处理读取响应或读取失败响应
    if ((pMsg->method == ATT_READ_RSP) ||
        ((pMsg->method == ATT_ERROR_RSP) &&
         (pMsg->msg.errorRsp.reqOpcode == ATT_READ_REQ)))
    {
        if (pMsg->method == ATT_ERROR_RSP)
        {
#if USB_PRINT_DEBUG
            uint8_t status = pMsg->msg.errorRsp.errCode;
            USB_PRINT ("Read Error: %x\n", status);  // 打印读取失败代码
#endif
        }
#if USB_PRINT_DEBUG
        else
        {
            // 读取成功时，打印读取到的第一个字节
            USB_PRINT ("Read rsp: %x\n", *pMsg->msg.readRsp.pValue);
        }
#endif
        centralProcedureInProgress = FALSE;  // 结束读取流程
    }
    // 处理写入响应或写入失败响应
    else if ((pMsg->method == ATT_WRITE_RSP) ||
             ((pMsg->method == ATT_ERROR_RSP) &&
              (pMsg->msg.errorRsp.reqOpcode == ATT_WRITE_REQ)))
    {
        if (pMsg->method == ATT_ERROR_RSP)
        {
#if USB_PRINT_DEBUG
            uint8_t status = pMsg->msg.errorRsp.errCode;
            USB_PRINT ("Write Error: %x\n", status);  // 打印写入失败代码
#endif
        }
#if USB_PRINT_DEBUG
        else
        {
            USB_PRINT ("Write success\n");  // 写入成功
            
            if (!tong2){
                USB_PRINT ("write 0x0f\n");  // 写入成功
                if (!low_battary)
                {
                    rgbled_setmod(4);
                }
                
                Send_Packet(0x0f,0,0,0);
                tong2 = true;
                tong1 = false;
            }
        }
#endif

        centralProcedureInProgress = FALSE;  // 结束写入流程
        lastWriteStartTime = 0;
    }
    // 处理特征值通知事件
    else if (pMsg->method == ATT_HANDLE_VALUE_NOTI)
    {
#if USB_PRINT_DEBUG
        // 打印通知数据，取第一个字节
        USB_PRINT ("Receive noti: %x\n", *pMsg->msg.handleValueNoti.pValue);
#endif
        Protocol_BLE_ACK(*pMsg->msg.handleValueNoti.pValue);
    }
    // 如果当前处于服务发现流程中，则继续处理发现相关事件
    else if (centralDiscState != BLE_DISC_STATE_IDLE)
    {
        centralGATTDiscoveryEvent (pMsg);  // 继续处理服务发现
    }

    // 统一释放 GATT 消息内存，避免内存泄漏
    GATT_bm_free (&pMsg->msg, pMsg->method);
}

/*********************************************************************
 * @fn      centralRssiCB
 *
 * @brief   RSSI 回调：用于接收并处理信号强度变化通知
 *
 * @param   connHandle - 连接句柄，用于标识连接
 * @param   rssi - 接收到的 RSSI 值，单位 dBm（为负数）
 *
 * @return  none
 */
static void centralRssiCB (uint16_t connHandle, int8_t rssi)
{

    if (central_isDiscState())
    {
        // 如果处于发现流程且建立连接超时，则断开连接
        if (bleConnectStartTime && (millis() - bleConnectStartTime >= BLE_CONNECT_TIMEOUT_MS))
        {
            centralDisconnect();
            bleConnectStartTime = 0;
        }
    }

#if USB_PRINT_DEBUG || USB_PRINT_DEBUG_RSSI
    // ��ӡ��ǰ���ӵ�RSSIֵ��һ��Ϊ����������� -rssi ��ʾ����dBm��
    USB_PRINT ("RSSI : -%d dB \n", -rssi);
#endif
}

/*********************************************************************
 * @fn      centralHciMTUChangeCB
 *
 * @brief   HCI MTU 改变回调。注意：ATT MTU 与 HCI 数据包长度不同
 *
 * @param   connHandle - 当前连接的句柄
 * @param   maxTxOctets - 本端可发送的 HCI 数据包最大字节数（含头部）
 * @param   maxRxOctets - 对端可接收的 HCI 数据包最大字节数（含头部）
 *
 * @return  none
 */
static void centralHciMTUChangeCB (uint16_t connHandle, uint16_t maxTxOctets, uint16_t maxRxOctets)
{
#if USB_PRINT_DEBUG
    // ��ӡHCI���ݰ����ȸ��º��Tx/Rx��󳤶ȣ����ֽ�Ϊ��λ��
    USB_PRINT (" HCI data length changed, Tx: %d, Rx: %d\n", maxTxOctets, maxRxOctets);
#endif
    // ��Ǵ�ʱ����Э������У����ܻ�Ӱ�������ATT��������
    centralProcedureInProgress = TRUE;
}

/*********************************************************************
 * @fn      centralEventCB
 *
 * @brief   Central 模式的 GAP 事件回调处理函数。
 *          处理 BLE 设备在 GAP 层产生的事件，例如设备初始化、扫描到设备、发现、连接、断开、参数更新等。
 *
 * @param   pEvent - 指向 GAP 事件结构的指针，包含事件码（opcode）及相关数据。
 *
 * @return  none
 */
static void centralEventCB (gapRoleEvent_t *pEvent)  // Central 模式 GAP 事件回调函数
{
    uint8_t zeroAddr[B_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    switch (pEvent->gap.opcode)           // 根据事件的 opcode 分发处理
    {
        case GAP_DEVICE_INIT_DONE_EVENT:  // GAP 设备初始化完成事件
        {

            if(isPeerAddrEmpty == 1){

                GAPRole_CentralStartDiscovery (     // 启动扫描
                    DEFAULT_DISCOVERY_MODE,         // 扫描模式（默认：全部设备）
                    DEFAULT_DISCOVERY_ACTIVE_SCAN,  // 是否使用主动扫描以获取更多广告数据
                    DEFAULT_DISCOVERY_WHITE_LIST);  // 是否使用白名单
            }
        }
        break;

        case GAP_DEVICE_INFO_EVENT:  // 发现设备事件
        {
            // 检查是否已达到扫描结果上限
            if (centralScanRes < DEFAULT_MAX_SCAN_RES)
            {
                // 将设备添加到列表
                centralAddDeviceInfo (             // 将发现的设备加入设备列表
                    pEvent->deviceInfo.addr,       // 设备地址
                    pEvent->deviceInfo.addrType);  // 地址类型


                uint8_t deviceIndex = centralScanRes - 1;  // 获取当前设备索引
                uint8_t *advData = pEvent->deviceInfo.pEvtData;
                uint8_t advLen = pEvent->deviceInfo.dataLen;

                // 解析广播数据中的 UUID
                if (!central_ParseUUIDFromAdvData (advData, advLen, &scannedDeviceUUIDs[deviceIndex]))
                {

                }
            }
        }
        break;

        case GAP_DEVICE_DISCOVERY_EVENT:  // 扫描完成事件
        {
            uint8_t i;                    // 循环索引，用于遍历设备列表

            // See if peer device has been discovered
            for (i = 0; i < centralScanRes; i++)                     // 遍历已扫描到的设备
            {
                if (scannedDeviceUUIDs[i].uuid_len != UUID_LEN_128)  // 仅处理 128-bit UUID
                {
                    continue;
                }

                if (!tmos_memcmp (PeerUUIDdef, scannedDeviceUUIDs[i].uuid, UUID_LEN_128))  // UUID 与目标设备不匹配，跳过
                {
                    continue;
                }

                if (tmos_memcmp (PeerAddrDef, zeroAddr, B_ADDR_LEN) || tmos_memcmp (PeerAddrDef, centralDevList[i].addr, B_ADDR_LEN))  // 检查是否为目标地址或与目标地址匹配
                {
                    break;                                                                                                             // 找到匹配设备，退出循环
                }
            }

            // 未找到目标设备
            if (i == centralScanRes)  // 未找到目标设备
            {
                centralScanRes = 0;                   // 重置设备计数
#if ENABLE_SCAN_RETRY
                GAPRole_CentralStartDiscovery (       // ���¿�ʼɨ��
                    DEFAULT_DISCOVERY_MODE,
                    DEFAULT_DISCOVERY_ACTIVE_SCAN,
                    DEFAULT_DISCOVERY_WHITE_LIST);

#endif
            }
            else  // ����ҵ���Ŀ���豸
            {
#if USB_PRINT_DEBUG
                USB_PRINT ("Device found...\n");   // ��ӡ��־���豸���ҵ�
#endif
                GAPRole_CentralEstablishLink (     // ������������
                    DEFAULT_LINK_HIGH_DUTY_CYCLE,  // ʹ�ø�ռ�ձȿ�������ģʽ
                    DEFAULT_LINK_WHITE_LIST,       // �Ƿ����ð�����
                    centralDevList[i].addrType,    // Ŀ���豸��ַ����
                    centralDevList[i].addr);       // Ŀ���豸��ַ

                // Start establish link timeout event
                tmos_start_task (  // �������ӳ�ʱ����
                    centralTaskId,
                    ESTABLISH_LINK_TIMEOUT_EVT,
                    ESTABLISH_LINK_TIMEOUT);
#if USB_PRINT_DEBUG
                USB_PRINT ("Connecting...\n");  // ��ӡ��־����������
#endif
            }
        }
        break;

        case GAP_LINK_ESTABLISHED_EVENT:                                 // ���ӽ�������¼�
        {
            tmos_stop_task (centralTaskId, ESTABLISH_LINK_TIMEOUT_EVT);  // ֹͣ���ӳ�ʱ��ʱ��
            if (pEvent->gap.hdr.status == SUCCESS)                       // ������ӳɹ�
            {

                centralConnHandle = pEvent->linkCmpl.connectionHandle;  // �������Ӿ��
                centralProcedureInProgress = TRUE;                      // ����в������ڽ�����

                // Update MTU
                attExchangeMTUReq_t req = {
                    // ����MTU����ṹ��
                    .clientRxMTU = BLE_BUFF_MAX_LEN - 4,  // �������MTU����
                };

                GATT_ExchangeMTU (  // ����MTU��������
                    centralConnHandle, &req, centralTaskId);

                // Initiate service discovery
                tmos_start_task (centralTaskId, START_SVC_DISCOVERY_EVT, DEFAULT_SVC_DISCOVERY_DELAY);  // ��������������

                // See if initiate connect parameter update
                if (centralParamUpdate)                                                                   // �Ƿ���Ҫ�������Ӳ���
                {
                    tmos_start_task (centralTaskId, START_PARAM_UPDATE_EVT, DEFAULT_PARAM_UPDATE_DELAY);  // ��ʱ�������Ӳ�������
                }

                // See if initiate phy update
                if (centralPhyUpdate)                                                                 // �Ƿ���Ҫ����PHY����
                {
                    tmos_start_task (centralTaskId, START_PHY_UPDATE_EVT, DEFAULT_PHY_UPDATE_DELAY);  // ��ʱ����PHY����
                }

                // See if start RSSI polling
                if (centralRssi)                                                                // �Ƿ���Ҫ��ѯRSSI
                {
                    tmos_start_task (centralTaskId, START_READ_RSSI_EVT, DEFAULT_RSSI_PERIOD);  // ����RSSI��ѯ����
                }
                if(isPeerAddrEmpty == 0){
                    tmos_memcpy (PeerAddrDef, pEvent->linkCmpl.devAddr, B_ADDR_LEN);
                    // ������EEPROM
                    EEPROM_WRITE (0, PeerAddrDef, B_ADDR_LEN);
                    isPeerAddrEmpty = 1;  // ������
                    USB_PRINT ("saved\n");   
                }
#if USB_PRINT_DEBUG
                // ��ӡ�豸��ַ����ʽΪ6�ֽڵ�ַ���ֽ���ʾ
                USB_PRINT ("\n-->Addr %x %x %x %x %x %x \n",
                           PeerAddrDef[0],
                           PeerAddrDef[1],
                           PeerAddrDef[2],
                           PeerAddrDef[3],
                           PeerAddrDef[4],
                           PeerAddrDef[5]);
                USB_PRINT ("Connected...\n");        // ��ӡ��־��������
#endif
                centralState = BLE_STATE_CONNECTED;  // ����״̬Ϊ������

                // ����ʱ��
                bleConnectStartTime = millis();
            }
            else  // �������ʧ��
            {
#if USB_PRINT_DEBUG
                USB_PRINT ("Connect Failed...Reason:%X\n", pEvent->gap.hdr.status);  // ��ӡʧ��ԭ��
                USB_PRINT ("Discovering...\n");                                      // ��ӡ��־�����¿�ʼɨ��
#endif
                centralScanRes = 0;                                                  // ����豸�б�
                GAPRole_CentralStartDiscovery (                                      // ���¿�ʼɨ��
                    DEFAULT_DISCOVERY_MODE,
                    DEFAULT_DISCOVERY_ACTIVE_SCAN,
                    DEFAULT_DISCOVERY_WHITE_LIST);
            }
        }
        break;

        case GAP_LINK_TERMINATED_EVENT:  // ���ӶϿ��¼�
        {
            // 断开连接时必须清理正在执行的 BLE 程序状态，否则重新连接后旧状态仍被当作有效执行
            Protocol_Reset_BLE_Exec_State(MAX_START_NUM);

            // centralState = BLE_STATE_DISCONNECTING;
            centralState = BLE_STATE_IDLE;                                           // �豸״̬�ÿ���
            centralConnHandle = GAP_CONNHANDLE_INIT;                                 // �������Ӿ��
            centralDiscState = BLE_DISC_STATE_IDLE;                                  // ���÷���״̬
            centralCharHdl = 0;                                                      // ����������
            centralScanRes = 0;                                                      // ����豸�б�
            centralProcedureInProgress = FALSE;                                      // ���������־
            tmos_stop_task (centralTaskId, START_READ_RSSI_EVT);                     // ֹͣRSSI��ѯ����
#if USB_PRINT_DEBUG
            USB_PRINT ("Disconnected...Reason:%x\n", pEvent->linkTerminate.reason);  // ��ӡ�Ͽ�ԭ��
            USB_PRINT ("Discovering...\n");                                          // ��ӡ��־����ʼ����ɨ��
#endif
        if(pEvent->linkTerminate.reason != 0x16 && isPeerAddrEmpty==1)
            {

        #if USB_PRINT_DEBUG
                USB_PRINT("Auto reconnect...\n");
        #endif

               startScanAgain();
            }
        }
        break;

        case GAP_LINK_PARAM_UPDATE_EVENT:  // ���Ӳ��������¼�
        {
#if USB_PRINT_DEBUG
            USB_PRINT ("Param Update...\n");  // ��ӡ��־�������Ѹ���
#endif
            centralDiscState = BLE_DISC_STATE_IDLE;
            centralStartDiscovery();
        }
        break;

        case GAP_PHY_UPDATE_EVENT:  // PHY���������¼������1M�л���2M��
        {
#if USB_PRINT_DEBUG
            USB_PRINT ("PHY Update...\n");  // ��ӡ��־��PHY�Ѹ���
#endif
        }


        break;


        default:  // Ĭ�Ϸ�֧��δ�������¼���
            break;
    }
}

/*********************************************************************
 * @fn      pairStateCB
 *
 * @brief   Pairing state callback.
 *
 * @return  none
 // ���״̬�ص��������������Ӿ�������״̬��״̬��
 */
static void centralPairStateCB (uint16_t connHandle, uint8_t state, uint8_t status)
{
#if USB_PRINT_DEBUG
    if (state == GAPBOND_PAIRING_STATE_STARTED)        // ��Կ�ʼ�¼�
    {
        USB_PRINT ("Pairing started:%d\n", status);    // ��ӡ��Կ�ʼ��־��status���������
    }
    else if (state == GAPBOND_PAIRING_STATE_COMPLETE)  // �������¼�
    {
        if (status == SUCCESS)                         // �ж�����Ƿ�ɹ�
        {
            USB_PRINT ("Pairing success\n");           // ��ӡ��Գɹ���־
        }
        else
        {
            USB_PRINT ("Pairing fail\n");  // ��ӡ���ʧ����־
        }
    }
    else if (state == GAPBOND_PAIRING_STATE_BONDED)  // �豸����Բ�����bonding��Ϣ
    {
        if (status == SUCCESS)                       // �жϰ��Ƿ�ɹ�
        {
            USB_PRINT ("Bonding success\n");         // ��ӡ�󶨳ɹ���־
        }
    }
    else if (state == GAPBOND_PAIRING_STATE_BOND_SAVED)  // Bonding��Ϣ�ѱ��浽�豸�У���Flash��
    {
        if (status == SUCCESS)                           // �жϱ�������Ƿ�ɹ�
        {
            USB_PRINT ("Bond save success\n");           // ��ӡbond����ɹ���־
        }
        else
        {
            USB_PRINT ("Bond save failed: %d\n", status);  // ��ӡbond����ʧ����־��״̬��
        }
    }
#endif
}

/*********************************************************************
 * @fn      centralStartDiscovery
 *
 * @brief   Start service discovery.
 *
 * @return  none
 */
static void centralStartDiscovery (void)  // ����GATT�����ֺ���
{

    if (centralDiscState == BLE_DISC_STATE_IDLE)                     // BLE_DISC_STATE_IDLE
    {
        centralSvcStartHdl = centralSvcEndHdl = centralCharHdl = 0;  // �������ľ��ֵ
        centralDiscState = BLE_DISC_STATE_SVC;                       // ���õ�ǰ����״̬Ϊ������

        // Discovery simple BLE service
        GATT_DiscPrimaryServiceByUUID (centralConnHandle,  // ��ǰ���Ӿ��
                                       PeerUUIDdef,        // UUID����
                                       UUID_LEN_128,       // UUID����
                                       centralTaskId);     // ��ǰ����ID�����ڻص���Ϣ��

    }
}

/*********************************************************************
 * @fn      centralGATTDiscoveryEvent
 *
 * @brief   Process GATT discovery event
 *
 * @return  none
 */
static void centralGATTDiscoveryEvent (gattMsgEvent_t *pMsg)  // ����GATT�������¼��ص�
{
    attReadByTypeReq_t req;                                   // ����ReadByType����ṹ��

    if (centralDiscState == BLE_DISC_STATE_SVC)  // ��ǰ���ڷ����ֽ׶�
    {
        if (pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP &&
            pMsg->msg.findByTypeValueRsp.numInfo > 0)  // �ҵ����񲢰�����Ϣ
        {


            if (pMsg->msg.findByTypeValueRsp.numInfo > 0)
            {
                centralSvcStartHdl = ATT_ATTR_HANDLE (pMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);   // ���������ʼ���
                centralSvcEndHdl = ATT_GRP_END_HANDLE (pMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);  // �������������

            }
        }


        // If procedure complete
        if ((pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP && pMsg->hdr.status == bleProcedureComplete) ||
            (pMsg->method == ATT_ERROR_RSP))
        {

            if (centralSvcStartHdl != 0)  // ����ɹ����ַ���
            {
                // Discover characteristic
                centralDiscState = BLE_DISC_STATE_CHAR;  // �л�Ϊ���������׶�
                req.startHandle = centralSvcStartHdl;    // ���ÿ�ʼ���
                req.endHandle = centralSvcEndHdl;        // ���ý������

                // ʹ��128λ UUID ��������
                req.type.len = UUID_LEN_128;
                tmos_memcpy (req.type.uuid, service_char_uuid, UUID_LEN_128);

                GATT_DiscAllChars (centralConnHandle, centralSvcStartHdl, centralSvcEndHdl, centralTaskId);
            }
        }
    }
    else if (centralDiscState == BLE_DISC_STATE_CHAR)
    {

        if (pMsg->method == ATT_READ_BY_TYPE_RSP && pMsg->msg.readByTypeRsp.numPairs > 0)
        {
            // ÿ�������ṹ����
            uint8_t len = pMsg->msg.readByTypeRsp.len;
            uint8_t *p = pMsg->msg.readByTypeRsp.pDataList;

            for (uint8_t i = 0; i < pMsg->msg.readByTypeRsp.numPairs; i++, p += len)
            {
                uint16_t declHandle = BUILD_UINT16 (p[0], p[1]);   // �����������
                centralCharProperties = p[2];                      // ���ԣ���/д/֪ͨ�ȣ�
                uint16_t valueHandle = BUILD_UINT16 (p[3], p[4]);  // ����ֵ���

                // ���� UUID�����ݳ����ж���16λ����128λ
                if (len == 7)  // 16-bit UUID: 1+1+1+1+1+2
                {
#if USB_PRINT_DEBUG
                    uint16_t uuid = BUILD_UINT16 (p[5], p[6]);
                    USB_PRINT ("UUID (16-bit): 0x%04x\n", uuid);
#endif
                }
                else if (len == 21)  // 128-bit UUID: 1+1+1+1+1+16
                {
                    uint8_t *uuid = &p[5];
#if USB_PRINT_DEBUG
                    USB_PRINT ("UUID (128-bit): ");
                    for (int j = 0; j < 16; j++) USB_PRINT ("%02x ", uuid[j]);
                    USB_PRINT ("\n");
#endif
                    // �ж��Ƿ����� UUID
                    if (tmos_memcmp (uuid, service_char_uuid, 16))
                    {
                        centralCharHdl = valueHandle;
#if USB_PRINT_DEBUG
                        USB_PRINT ("==> Matched target char UUID! Handle: 0x%x\n", valueHandle);
#endif
                        // ��ʼ��д����
                        tmos_start_task (centralTaskId, START_READ_OR_WRITE_EVT, DEFAULT_READ_OR_WRITE_DELAY);
                    }
                }
            }
        }

        if ((pMsg->method == ATT_READ_BY_TYPE_RSP && pMsg->hdr.status == bleProcedureComplete) ||
            (pMsg->method == ATT_ERROR_RSP))
        {
#if USB_PRINT_DEBUG
            USB_PRINT ("-> Characteristic discovery complete (or error)\n");
#endif
            // ��ʼ���� CCCD
            centralDiscState = BLE_DISC_STATE_CCCD;
            req.startHandle = centralSvcStartHdl;
            req.endHandle = centralSvcEndHdl;

            req.type.len = ATT_BT_UUID_SIZE;
            req.type.uuid[0] = LO_UINT16 (GATT_CLIENT_CHAR_CFG_UUID);
            req.type.uuid[1] = HI_UINT16 (GATT_CLIENT_CHAR_CFG_UUID);

#if USB_PRINT_DEBUG
            USB_PRINT ("-> Start discovering CCCD from handle %x to %x...\n", req.startHandle, req.endHandle);
#endif
            GATT_ReadUsingCharUUID (centralConnHandle, &req, centralTaskId);
        }
    }

    else if (centralDiscState == BLE_DISC_STATE_CCCD)  // ��ǰ����CCCD���ֽ׶�
    {
#if USB_PRINT_DEBUG
        USB_PRINT ("-> Processing CCCD Discovery...\n");
#endif
        // Characteristic found, store handle
        if (pMsg->method == ATT_READ_BY_TYPE_RSP &&
            pMsg->msg.readByTypeRsp.numPairs > 0)  // �ҵ�CCCD
        {
            centralCCCDHdl = BUILD_UINT16 (pMsg->msg.readByTypeRsp.pDataList[0],
                                           pMsg->msg.readByTypeRsp.pDataList[1]);                   // ��ȡCCCD���
            centralProcedureInProgress = FALSE;                                                     // ��ǹ��������
#if USB_PRINT_DEBUG
            USB_PRINT ("Found client characteristic configuration handle : %x\n", centralCCCDHdl);  // ��ӡCCCD���
#endif
            // Start do write CCCD
            tmos_start_task (centralTaskId, START_WRITE_CCCD_EVT, DEFAULT_WRITE_CCCD_DELAY);  // ����дCCCD����
        }
#if USB_PRINT_DEBUG
        else
        {
            USB_PRINT ("!! CCCD not found.\n");
        }
#endif
        centralDiscState = BLE_DISC_STATE_IDLE;  // ��������ɣ�״̬�ÿ���
    }
#if USB_PRINT_DEBUG
    else
    {
        USB_PRINT ("!! Unknown discovery state: %d\n", centralDiscState);
    }
#endif
}

/*********************************************************************
 * @fn      centralAddDeviceInfo
 *
 * @brief   Add a device to the device discovery result list
 *
 * @return  none
 */
static void centralAddDeviceInfo (uint8_t *pAddr, uint8_t addrType)
{
    uint8_t i;  // ѭ���������ڱ����Ѵ��豸�б�

    // �жϵ�ǰɨ���������Ƿ�ﵽ�������
    if (centralScanRes < DEFAULT_MAX_SCAN_RES)
    {
        // ���������豸�б�������豸��ַ�Ƿ��Ѵ���
        for (i = 0; i < centralScanRes; i++)
        {
            // ����豸��ַ�Ѵ��ڣ���ֱ�ӷ��أ����ظ�����
            if (tmos_memcmp (pAddr, centralDevList[i].addr, B_ADDR_LEN))
            {
                return;  // �豸�Ѵ��ڣ��˳�����
            }
        }
        // �豸��ַ�����ڣ��������豸��ַ���豸�б�
        tmos_memcpy (centralDevList[centralScanRes].addr, pAddr, B_ADDR_LEN);

        // �����豸��ַ���ͣ��繫����ַ�������ַ��
        centralDevList[centralScanRes].addrType = addrType;

        // ɨ�����豸������1
        centralScanRes++;
    }
}

// �ӹ㲥�����н�����һ��16λ��128λUUID������0��ʾ�ɹ���-1��ʾδ�ҵ�
int central_ParseUUIDFromAdvData (uint8_t *pData, uint8_t dataLen, DeviceUUIDInfo_t *uuidBuf)
{
    uint8_t index = 0;

    if (uuidBuf != NULL)
    {
        while (index < dataLen)
        {
            uint8_t fieldLen = pData[index];

            if (fieldLen == 0 || (index + fieldLen >= dataLen))
            {
                break;  // ��ֹ�Ƿ� fieldLen Խ��
            }

            // ����Ҫ�� type �ֶ�
            if (index + 1 >= dataLen)
            {
                break;  //  ��ֹ��ȡ adType Խ��
            }

            uint8_t adType = pData[index + 1];

            if ((adType == 0x02 || adType == 0x03) && fieldLen >= UUID_LEN_16 + 1)
            {
                memcpy (uuidBuf->uuid, &pData[index + 2], UUID_LEN_16);
                uuidBuf->uuid_len = UUID_LEN_16;
                return 0;
            }
            else if ((adType == 0x06 || adType == 0x07) && fieldLen >= UUID_LEN_128 + 1)
            {
                memcpy (uuidBuf->uuid, &pData[index + 2], UUID_LEN_128);
                uuidBuf->uuid_len = UUID_LEN_128;
                return 0;
            }

            index += fieldLen + 1;
        }
    }
    return -1;
}

/************************ endfile @ central **************************/

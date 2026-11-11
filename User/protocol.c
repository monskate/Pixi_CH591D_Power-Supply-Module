#include "protocol.h"
#include "usb.h"
/*
    UART 串口协议
*/

UART_PACKET uart1_packet;

PROGRAM_NODE program;
BLE_EXEC_STATE ble_exec;
uint8_t exe = 0;
/*
    串口数据包准备标志
*/
static bool uart1_packet_ready = false;

/*
    初始化
*/
void Protocol_Init(void)
{
    uart1_packet.state = WAIT_AA;
    uart1_packet.index = 0;
    memset(&program, 0, sizeof(program));
    memset(&ble_exec, 0, sizeof(ble_exec));
    exe = 0;
}

/*
    校验

    前 6 个字节求和

*/
uint8_t checksum(uint8_t *data)
{
    uint8_t sum = 0;
    for (int i = 0; i < 6; i++)
    {
        sum += data[i];
    }
    return sum;
}

/*
    发送一个数据包

*/
void Send_Packet(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4)
{
    uint8_t buf[7];
    buf[0] = 0xAA;
    buf[1] = 0x55;
    buf[2] = d1;
    buf[3] = d2;
    buf[4] = d3;
    buf[5] = d4;
    buf[6] = checksum(buf);

    UART0_SendString(buf, 7);
}

static void Send_BLE(uint8_t *packet, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4)
{
    packet[0] = 0xAA;
    packet[1] = 0x55;
    packet[2] = d1;
    packet[3] = d2;
    packet[4] = d3;
    packet[5] = d4;
    packet[6] = checksum(packet);
}
void Send_Next_BLE(void)
{
    if(!ble_exec.running)
        return;
    if (central_isState() == 0)
    {
        Protocol_Reset_BLE_Exec_State(MAX_START_NUM);
        return;
    }
    if (ble_exec.start_id >= MAX_START_NUM)
    {
        ble_exec.running = 0;
        return;
    }
    uint8_t start = ble_exec.start_id;
    while(ble_exec.current_exec < MAX_EXEC_NUM)
    {

        uint8_t id = ble_exec.current_exec;
        EXEC_NODE *node = &program.start[start].exec[id];
        
        if(node->used)
        {
#if USB_PRINT_DEBUG
            USB_PRINT(
              "BLE send exec:%d type:%d\n",
              id,
              program.start[start].exec[id].type
            );
#endif
            if(node->type==0x32)
            {

                if(ble_exec.loop_depth < 3)
                {
                    uint8_t d = ble_exec.loop_depth;
                    ble_exec.loop[d].start=id+1;
                    ble_exec.loop[d].current = 0;
                    ble_exec.loop_depth++;
                }
                ble_exec.current_exec++;
                continue;
            }
            if(node->type==0x30 || node->type==0x31)
            {
                if (ble_exec.loop_depth == 0)
                {
                    ble_exec.current_exec++;
                    continue;
                }
                uint8_t d = ble_exec.loop_depth-1;
                ble_exec.loop[d].count =node->loop_count;
                
                Protocol_BLE_ACK(node->type);
                return;

            }
            exe = id;
            uint8_t blePacket[PACKET_LEN];
            Send_Packet(0x08,start,id,node->type);
            Send_BLE(blePacket, 0x08,start,id,node->type);
            central_sendData(blePacket, sizeof(blePacket));

            // 等待 ESP32 通知
            return;
        }

        break;
    }
    // 全部执行完成
    ble_exec.running = 0;

    Send_Packet(0x0E, start, exe, 0);
#if USB_PRINT_DEBUG
    USB_PRINT("All execute done%d\n", exe);
#endif
    exe = 0;
}

void Protocol_Reset_BLE_Exec_State(uint8_t start)
{
    if (ble_exec.running)
    {
        if (start >= MAX_START_NUM || ble_exec.start_id == start)
        {
            Send_Packet(0x0E, ble_exec.start_id, exe, 0);
        }
    }

    ble_exec.running = 0;
    ble_exec.start_id = MAX_START_NUM;
    ble_exec.current_exec = 0;
    ble_exec.loop_depth = 0;
    memset(ble_exec.loop, 0, sizeof(ble_exec.loop));
    exe = 0;
}

void Protocol_BLE_ACK(uint8_t ack)
{
    if (central_isState() == 0)
    {
        Protocol_Reset_BLE_Exec_State(MAX_START_NUM);
        return;
    }

    if (ble_exec.start_id >= MAX_START_NUM || ble_exec.current_exec >= MAX_EXEC_NUM)
    {
        ble_exec.running = 0;
        return;
    }

    EXEC_NODE *node = &program.start[ble_exec.start_id].exec[ble_exec.current_exec];

    if(ack != node->type)
        return;

    /*
       普通执行
    */

    if (node->type != 0x30 && node->type != 0x31)
    {
        ble_exec.current_exec++;
    }
    /*
       循环执行
    */
    else
    {
        if (ble_exec.loop_depth > 0)
        {
            uint8_t id = ble_exec.loop_depth - 1;
            ble_exec.loop[id].current++;
            USB_Print("current%d,count%d\r\n", ble_exec.loop[id].current, ble_exec.loop[id].count);
            if (ble_exec.loop[id].current < ble_exec.loop[id].count)
            {
                /*
                    回到循环起始处
                */
                ble_exec.current_exec = ble_exec.loop[id].start;
            }
            else
            {
                /*
                   循环结束
                */
                ble_exec.loop[ble_exec.loop_depth - 1].current = 0;
                ble_exec.loop_depth--;
                ble_exec.current_exec++;
            }
        }
        else
        {
            ble_exec.current_exec++;
        }
    }
    Send_Next_BLE();
}

/*
    UART 字节状态机

*/
void UART_ParseByte(UART_PACKET *packet, uint8_t data)
{
    switch (packet->state)
    {
        case WAIT_AA:
            if (data == 0xAA)
            {
                packet->buf[0] = data;
                packet->state = WAIT_55;
            }
            break;
        case WAIT_55:
            if (data == 0x55)
            {
                packet->buf[1] = data;
                packet->index = 2;
                packet->state = RECEIVING;
            }
            else
            {
                packet->state = WAIT_AA;
            }
            break;
        case RECEIVING:
            packet->buf[packet->index++] = data;
            if (packet->index >= PACKET_LEN)
            {
                if (packet->buf[6] == checksum(packet->buf))
                {
                    uart1_packet_ready = true;
                }
                packet->index = 0;
                packet->state = WAIT_AA;
            }
            break;
        }
}

/*
    主循环处理

*/

void Protocol_Process(void)
{
    uint8_t packet[7];

    if (uart1_packet_ready)
    {
        uart1_packet_ready = false;
        for (int i = 0; i < 7; i++)
        {
            packet[i] = uart1_packet.buf[i];
        }
        Process_Packet(packet);
    }
}

/*
    数据处理

*/
void Process_Packet(uint8_t *buf)
{

    uint8_t cmd = buf[2];

    /*
        创建程序/启动配置
    */
    if (cmd == 0x01)
    {
        if(buf[3] == 0){
            memset(&program,0,sizeof(program));
            program.start[1].used = 1;
            #if USB_PRINT_DEBUG
                    USB_PRINT ("start%02x\r\n ", buf[3]+1);
            #endif
            Send_Packet(0x02,1,0,0);
            Send_Packet(0x03,1,0,buf[5]);
            if (central_isState())  // 已连接
            {
                Send_Packet(0x0F,0,0,0);
            }  else{
                Send_Packet(0x10,0,0,0);
            }
        }
        else {
            #if USB_PRINT_DEBUG
                    USB_PRINT ("start%02x\r\n ", buf[3]);
            #endif
            if(buf[3] < MAX_START_NUM)
            {
                program.start[buf[3]].used = 1;

                // 清空之前的执行程序
                memset(
                    program.start[buf[3]].exec,
                    0,
                    sizeof(program.start[buf[3]].exec)
                );
            }
            Send_Packet(0x03,buf[3],buf[4],buf[5]);
            if (central_isState())  // 已连接
            {
                Send_Packet(0x0F,0,0,0);
            }  else{
                Send_Packet(0x10,0,0,0);
            }
        }
    }

    /*
        执行指令配置
    */
    if (cmd == 0x04)
    {
        uint8_t start = buf[3];
        uint8_t exec = buf[4];
        uint8_t type = buf[5];
        if(start < MAX_START_NUM && exec < MAX_EXEC_NUM)
        {

            // 如果该程序未定义，则直接忽略
            // 防止写入到未启用的项目
            if(program.start[start].used == 0)
            {
                return;
            }

            program.start[start].exec[exec].used = 1;

            program.start[start].exec[exec].type = type;

        }
        #if USB_PRINT_DEBUG
                USB_PRINT ("start%02x zhixing%02x type%02x \r\n ", buf[3],buf[4],buf[5]);
        #endif
        if(type == 0x30)
        {
            program.start[start].exec[exec].loop_count = 3;
        }

        if(type == 0x31)
        {
            program.start[start].exec[exec].loop_count = 10;
        }

        if(type == 0x32)
        {
            program.start[start].exec[exec].loop_start = exec;
        }
        Send_Packet(0x06,buf[3],buf[4],buf[5]);
    }

    /*
        开始执行

    */
    if (cmd == 0x07)
    {
        uint8_t start = buf[3];

        if (ble_exec.running && ble_exec.start_id == start)
        {
            Protocol_Reset_BLE_Exec_State(start);
        }

        ble_exec.running = 1;
        ble_exec.start_id = start;
        ble_exec.current_exec = 1;
        ble_exec.loop_depth = 0;
        memset(ble_exec.loop, 0, sizeof(ble_exec.loop));

        Send_Next_BLE();
    }
    /*
        停止/清除

    */
    if (cmd == 0x0C)
    {
        uint8_t start = buf[3];
        uint8_t exec = buf[4];

        if(exec == 0){
            memset(&program.start[start],0,sizeof(START_NODE));
            #if USB_PRINT_DEBUG
                    USB_PRINT ("stop%02x\r\n ", buf[3]);
            #endif
        }
        else {
            memset(&program.start[start].exec[exec],0,sizeof(EXEC_NODE) * (MAX_EXEC_NUM - exec));

            #if USB_PRINT_DEBUG
                    USB_PRINT ("stop%02x zhixing%02x type%02x \r\n ", buf[3],buf[4],buf[5]);
            #endif
        }

        if (start == ble_exec.start_id)
        {
            Protocol_Reset_BLE_Exec_State(start);
        }
    }
}

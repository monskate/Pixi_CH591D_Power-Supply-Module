#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "CH59x_common.h"
#include <string.h>
#include "central.h"
#include "CH59xBLE_LIB.h"
/*
    ���ݳ���
*/
#define PACKET_LEN      7

/*
    Э��״̬
*/
#define WAIT_AA         0
#define WAIT_55         1
#define RECEIVING       2

/*
    UART����״̬��
*/
typedef struct
{
    uint8_t buf[PACKET_LEN];
    uint8_t index;
    uint8_t state;
}UART_PACKET;

typedef struct
{
    uint8_t running;     // �Ƿ�����ִ��
    uint8_t start_id;    // ����ģ��
    uint8_t current_exec;// ��ǰִ��ģ��
    //ѭ��ջ
    struct
    {
        uint8_t start;
        uint8_t count;
        uint8_t current;

    }loop[3];


    uint8_t loop_depth;
} BLE_EXEC_STATE;



#define MAX_START_NUM  8
#define MAX_EXEC_NUM   20


//ִ��ģ��
typedef struct
{
    uint8_t used;       //��λ���Ƿ�ռ��
    uint8_t type;       //ģ������

    /*
        ѭ������
    */
    uint8_t loop_count;     //ѭ������
    uint8_t loop_start;     //ѭ����ʼλ��

}EXEC_NODE;


//����ģ��
typedef struct
{
    uint8_t used;       

    EXEC_NODE exec[MAX_EXEC_NUM];

}START_NODE;


//��������
typedef struct
{
    START_NODE start[MAX_START_NUM];

}PROGRAM_NODE;


extern PROGRAM_NODE program;
/*
    �ⲿ����
*/
extern UART_PACKET uart1_packet;

extern bool heartbeat_wait;
extern uint32_t heartbeat_send_time;



/*
    ����
*/
void Protocol_Init(void);

void UART_ParseByte(UART_PACKET *packet,uint8_t data);
void Process_Packet(uint8_t *buf);
void Protocol_Process(void);

uint8_t checksum(uint8_t *data);

void Send_Packet(uint8_t d1,uint8_t d2,uint8_t d3,uint8_t d4);
void Protocol_Reset_BLE_Exec_State(uint8_t start);
void Protocol_BLE_ACK(uint8_t buf);


#endif

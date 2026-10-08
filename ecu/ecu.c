/*
 * CAN-Based UDS Diagnostic System
 * ECU / Server Node
 *
 * MCU: NXP LPC1768
 * Role: Diagnostic ECU
 *
 * Reconstructed from the original project report because the original
 * source/project directory was no longer available.
 */

#include "LPC17xx.h"
#include <stdint.h>
#include <stdio.h>

#define THRE (1 << 5)
#define DLAB (1 << 7)

typedef struct {
    uint32_t id;
    uint8_t len;
    uint8_t data[8];
} CAN_msg;

void UART0_Init(void);
void UART0_Write(char c);
void UART0_WriteStr(const char *s);
void CAN_Init(void);
int CAN2_Receive(CAN_msg *msg);
void CAN2_Send(CAN_msg *msg);

int main(void)
{
    SystemInit();
    UART0_Init();
    CAN_Init();

    UART0_WriteStr("UDS ECU Ready\r\n");

    CAN_msg rx, tx;

    while (1) {
        if (CAN2_Receive(&rx)) {

            UART0_WriteStr("UDS Req: ");

            for (int i = 0; i < rx.len; i++) {
                char buf[5];
                sprintf(buf, "%02X ", rx.data[i]);
                UART0_WriteStr(buf);
            }

            UART0_WriteStr("\r\n");

            /* VIN Request F190 */
            if (rx.data[1] == 0x22 &&
                rx.data[2] == 0xF1 &&
                rx.data[3] == 0x90) {

                tx.id = 0x7E8;
                tx.len = 8;

                tx.data[0] = 0x06;
                tx.data[1] = 0x62;
                tx.data[2] = 0xF1;
                tx.data[3] = 0x90;
                tx.data[4] = 'E';
                tx.data[5] = 'C';
                tx.data[6] = 'U';
                tx.data[7] = 0x00;

                CAN2_Send(&tx);

                UART0_WriteStr(
                    "UDS Resp Sent: 62 F190 ECU\r\n"
                );
            }

            /* ECU Hardware Number F113 */
            else if (rx.data[1] == 0x22 &&
                     rx.data[2] == 0xF1 &&
                     rx.data[3] == 0x13) {

                tx.id = 0x7E8;
                tx.len = 8;

                tx.data[0] = 0x06;
                tx.data[1] = 0x62;
                tx.data[2] = 0xF1;
                tx.data[3] = 0x13;
                tx.data[4] = 'A';
                tx.data[5] = 'B';
                tx.data[6] = '1';
                tx.data[7] = '2';

                CAN2_Send(&tx);

                UART0_WriteStr(
                    "UDS Resp Sent: 62 F113 AB12\r\n"
                );
            }
        }
    }
}

/* UART Functions */
void UART0_Init(void)
{
    LPC_PINCON->PINSEL0 |= (1 << 4) | (1 << 6);
    LPC_UART0->LCR = 0x83;
    LPC_UART0->DLL = 162;
    LPC_UART0->DLM = 0;
    LPC_UART0->LCR = 0x03;
}

void UART0_Write(char c)
{
    while (!(LPC_UART0->LSR & THRE));
    LPC_UART0->THR = c;
}

void UART0_WriteStr(const char *s)
{
    while (*s)
        UART0_Write(*s++);
}

/* CAN Initialization */
void CAN_Init(void)
{
    LPC_SC->PCONP |= (1 << 13) | (1 << 14);
    LPC_SC->PCLKSEL0 &= ~((3 << 26) | (3 << 28));

    LPC_PINCON->PINSEL4 &= ~0x0003C000;
    LPC_PINCON->PINSEL4 |= 0x00014000;

    LPC_CANAF->AFMR = 0x02;

    LPC_CAN1->MOD = LPC_CAN2->MOD = 1;
    LPC_CAN1->BTR =
        (0 << 23) | (0 << 22) | (5 << 20) | (6 << 16) | (2);

    LPC_CAN2->BTR = LPC_CAN1->BTR;

    LPC_CAN1->MOD = LPC_CAN2->MOD = 0;
}

/* CAN2 Receive */
int CAN2_Receive(CAN_msg *msg)
{
    if (LPC_CAN2->GSR & 1) {

        msg->id = LPC_CAN2->RID;
        msg->len = (LPC_CAN2->RFS >> 16) & 0x0F;

        uint32_t A = LPC_CAN2->RDA;
        uint32_t B = LPC_CAN2->RDB;

        msg->data[0] = A & 0xFF;
        msg->data[1] = (A >> 8) & 0xFF;
        msg->data[2] = (A >> 16) & 0xFF;
        msg->data[3] = (A >> 24) & 0xFF;

        msg->data[4] = B & 0xFF;
        msg->data[5] = (B >> 8) & 0xFF;
        msg->data[6] = (B >> 16) & 0xFF;
        msg->data[7] = (B >> 24) & 0xFF;

        LPC_CAN2->CMR = (1 << 2);

        return 1;
    }

    return 0;
}

/* CAN2 Send */
void CAN2_Send(CAN_msg *msg)
{
    while (!(LPC_CAN2->SR & (1 << 2)));

    LPC_CAN2->TFI1 = (msg->len & 0x0F) << 16;
    LPC_CAN2->TID1 = msg->id & 0x7FF;

    LPC_CAN2->TDA1 =
        (msg->data[3] << 24) |
        (msg->data[2] << 16) |
        (msg->data[1] << 8)  |
        (msg->data[0]);

    LPC_CAN2->TDB1 =
        (msg->data[7] << 24) |
        (msg->data[6] << 16) |
        (msg->data[5] << 8)  |
        (msg->data[4]);

    LPC_CAN2->CMR = 0x21;
}

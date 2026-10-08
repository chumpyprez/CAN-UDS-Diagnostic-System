/*
 * CAN-Based UDS Diagnostic System
 * Tester / Client Node
 *
 * MCU: NXP LPC1768
 * Role: UDS Tester
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

volatile int sendPeriodicFlag = 0;

/* Function Prototypes */
void UART0_Init(void);
void UART0_Write(char c);
void UART0_WriteStr(const char *s);
void CAN_Init(void);
void CAN1_Send(CAN_msg *msg);
void setup_timer(void);
void setup_GPIO_interrupt(void);

/* UDS CAN Messages */
CAN_msg udsReqPeriodic;
CAN_msg udsReqKey;

/* TIMER0 Interrupt (Every 3s) */
void TIMER0_IRQHandler(void)
{
    LPC_TIM0->IR = 1;
    sendPeriodicFlag = 1;
}

/* GPIO Interrupt (Button P2.13) */
void EINT3_IRQHandler(void)
{
    if (LPC_GPIOINT->IO2IntStatF & (1 << 13)) {
        LPC_GPIOINT->IO2IntClr = (1 << 13);

        CAN1_Send(&udsReqKey);
        UART0_WriteStr("Button P2.13: ECU HW Number request sent\r\n");
    }
}

int main(void)
{
    SystemInit();
    UART0_Init();
    CAN_Init();

    UART0_WriteStr("UDS Tester Ready\r\n");

    /* Prepare VIN request (DID F190) */
    udsReqPeriodic.id = 0x7DF;
    udsReqPeriodic.len = 8;
    udsReqPeriodic.data[0] = 0x03;
    udsReqPeriodic.data[1] = 0x22;
    udsReqPeriodic.data[2] = 0xF1;
    udsReqPeriodic.data[3] = 0x90;

    for (int i = 4; i < 8; i++)
        udsReqPeriodic.data[i] = 0x00;

    /* Prepare ECU Hardware Number request (DID F113) */
    udsReqKey.id = 0x7DF;
    udsReqKey.len = 8;
    udsReqKey.data[0] = 0x03;
    udsReqKey.data[1] = 0x22;
    udsReqKey.data[2] = 0xF1;
    udsReqKey.data[3] = 0x13;

    for (int i = 4; i < 8; i++)
        udsReqKey.data[i] = 0x00;

    setup_timer();
    setup_GPIO_interrupt();

    while (1) {
        if (sendPeriodicFlag) {
            sendPeriodicFlag = 0;
            CAN1_Send(&udsReqPeriodic);
            UART0_WriteStr("Periodic VIN request sent\r\n");
        }

        __WFI();
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

/* Timer Setup (3s) */
void setup_timer(void)
{
    LPC_SC->PCONP |= (1 << 1);
    LPC_SC->PCLKSEL0 &= ~(3 << 2);
    LPC_SC->PCLKSEL0 |= (1 << 2);

    LPC_TIM0->PR = 25000 - 1;
    LPC_TIM0->MR0 = 3000 - 1;
    LPC_TIM0->MCR = 3;
    LPC_TIM0->TCR = 1;

    NVIC_EnableIRQ(TIMER0_IRQn);
}

/* GPIO Interrupt Setup (P2.13) */
void setup_GPIO_interrupt(void)
{
    LPC_PINCON->PINSEL4 &= ~(3 << 26);
    LPC_GPIO2->FIODIR &= ~(1 << 13);
    LPC_GPIOINT->IO2IntEnF |= (1 << 13);
    LPC_GPIOINT->IO2IntClr = (1 << 13);

    NVIC_EnableIRQ(EINT3_IRQn);
}

/* CAN Init + Send */
void CAN_Init(void)
{
    LPC_SC->PCONP |= (1 << 13) | (1 << 14);
    LPC_SC->PCLKSEL0 &= ~((3 << 26) | (3 << 28));

    LPC_PINCON->PINSEL0 &= ~0x0000000F;
    LPC_PINCON->PINSEL0 |= 0x00000005;

    LPC_CANAF->AFMR = 0x02;

    LPC_CAN1->MOD = LPC_CAN2->MOD = 1;
    LPC_CAN1->BTR =
        (0 << 23) | (0 << 22) | (5 << 20) | (6 << 16) | (2);

    LPC_CAN2->BTR = LPC_CAN1->BTR;

    LPC_CAN1->MOD = LPC_CAN2->MOD = 0;
}

void CAN1_Send(CAN_msg *msg)
{
    while (!(LPC_CAN1->SR & (1 << 2)));

    LPC_CAN1->TFI1 = (msg->len & 0x0F) << 16;
    LPC_CAN1->TID1 = msg->id & 0x7FF;

    LPC_CAN1->TDA1 =
        (msg->data[3] << 24) |
        (msg->data[2] << 16) |
        (msg->data[1] << 8)  |
        (msg->data[0]);

    LPC_CAN1->TDB1 =
        (msg->data[7] << 24) |
        (msg->data[6] << 16) |
        (msg->data[5] << 8)  |
        (msg->data[4]);

    LPC_CAN1->CMR = 0x21;
}

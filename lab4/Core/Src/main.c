#include "stm32f446xx.h"
#include <stdio.h>

#define BUF_SIZE 128

volatile char ring[BUF_SIZE];
volatile uint16_t head = 0;
volatile uint16_t tail = 0;
volatile uint8_t lineReady = 0;

/* UART transmit function */
void USART2_SendChar(char c)
{
    while(!(USART2->SR & USART_SR_TXE));
    USART2->DR = c;
}

void USART2_SendString(char *s)
{
    while(*s)
    {
        USART2_SendChar(*s++);
    }
}

/* USART2 Interrupt */
void USART2_IRQHandler(void)
{
    if(USART2->SR & USART_SR_RXNE)
    {
        char c = USART2->DR;

        ring[head] = c;
        head = (head + 1) % BUF_SIZE;

        if(c == '\r')
            lineReady = 1;
    }

    if(USART2->SR & USART_SR_ORE)
    {
        (void)USART2->DR;
    }
}

/* MAIN */
int main(void)
{
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Enable USART2 clock */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* Configure PA2 and PA3 as Alternate Function */
    GPIOA->MODER &= ~((3<<4) | (3<<6));
    GPIOA->MODER |=  ((2<<4) | (2<<6));

    /* Select AF7 for USART2 */
    GPIOA->AFR[0] &= ~((0xF<<8) | (0xF<<12));
    GPIOA->AFR[0] |=  ((7<<8) | (7<<12));

    /* Set baud rate (115200 @ 45MHz) */
    USART2->BRR = (8 << 4) | 11;

    /* Enable TX, RX and RX interrupt */
    USART2->CR1 = USART_CR1_TE |
                  USART_CR1_RE |
                  USART_CR1_RXNEIE;

    /* Enable USART */
    USART2->CR1 |= USART_CR1_UE;

    /* Enable NVIC interrupt */
    NVIC_SetPriority(USART2_IRQn,1);
    NVIC_EnableIRQ(USART2_IRQn);

    USART2_SendString("UART Interrupt Demo\r\n");

    while(1)
    {
        if(lineReady)
        {
            lineReady = 0;

            char line[BUF_SIZE];
            uint16_t i = 0;

            while(tail != head)
            {
                line[i++] = ring[tail];
                tail = (tail + 1) % BUF_SIZE;
            }

            line[i] = '\0';

            USART2_SendString("Echo: ");
            USART2_SendString(line);
            USART2_SendString("\r\n");
        }
    }
}

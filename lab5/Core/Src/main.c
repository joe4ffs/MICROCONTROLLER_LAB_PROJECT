#include "stm32f4xx.h"


#define RX_BUF_SIZE 64

#define MSG "Baremetal DMA UART Demo -- send 64 bytes\r\n"


uint8_t rxBuf[RX_BUF_SIZE];

volatile uint8_t txDone = 1;


/* ================= UART2 + DMA INIT ================= */

void uart2_dma_init(void)

{

    /* 1. Enable clocks */

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_DMA1EN;

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;


    /* 2. Configure PA2 (TX), PA3 (RX) as Alternate Function */

    GPIOA->MODER &= ~((3UL << 4) | (3UL << 6));

    GPIOA->MODER |=  ((2UL << 4) | (2UL << 6));  // AF mode


    GPIOA->AFR[0] &= ~((0xFUL << 8) | (0xFUL << 12));

    GPIOA->AFR[0] |=  ((7UL << 8) | (7UL << 12)); // AF7 = USART2


    /* 3. USART2 configuration */

    USART2->BRR = (16000000 / 115200); // assuming 16 MHz

    USART2->CR3 |= USART_CR3_DMAT | USART_CR3_DMAR; // enable DMA

    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;


    /* ================= DMA TX (Stream6, Channel4) ================= */

    DMA1_Stream6->CR = 0;

    while (DMA1_Stream6->CR & DMA_SxCR_EN);


    DMA1_Stream6->PAR = (uint32_t)&USART2->DR;


    DMA1_Stream6->CR =

        (4 << 25) |              // Channel 4

        DMA_SxCR_MINC |          // Memory increment

        DMA_SxCR_DIR_0 |         // Memory → Peripheral

        DMA_SxCR_TCIE;           // Transfer complete interrupt


    /* ================= DMA RX (Stream5, Channel4) ================= */

    DMA1_Stream5->CR = 0;

    while (DMA1_Stream5->CR & DMA_SxCR_EN);


    DMA1_Stream5->PAR  = (uint32_t)&USART2->DR;

    DMA1_Stream5->M0AR = (uint32_t)rxBuf;

    DMA1_Stream5->NDTR = RX_BUF_SIZE;


    DMA1_Stream5->CR =

        (4 << 25) |              // Channel 4

        DMA_SxCR_MINC |          // Memory increment

        DMA_SxCR_TCIE;           // Transfer complete interrupt


    /* Enable DMA interrupts */

    NVIC_EnableIRQ(DMA1_Stream6_IRQn);

    NVIC_EnableIRQ(DMA1_Stream5_IRQn);

}


/* ================= SEND USING DMA ================= */

void uart2_send_dma(uint8_t *data, uint16_t len)

{

    txDone = 0;


    DMA1_Stream6->CR &= ~DMA_SxCR_EN;

    while (DMA1_Stream6->CR & DMA_SxCR_EN);


    DMA1_Stream6->M0AR = (uint32_t)data;

    DMA1_Stream6->NDTR = len;


    DMA1->HIFCR |= DMA_HIFCR_CTCIF6; // clear interrupt flag


    DMA1_Stream6->CR |= DMA_SxCR_EN;

}


/* ================= START RX ================= */

void uart2_start_rx(void)

{

    DMA1_Stream5->CR |= DMA_SxCR_EN;

}


/* ================= INTERRUPTS ================= */


/* TX complete */

void DMA1_Stream6_IRQHandler(void)

{

    if (DMA1->HISR & DMA_HISR_TCIF6)

    {

        DMA1->HIFCR |= DMA_HIFCR_CTCIF6;

        txDone = 1;

    }

}


/* RX complete */

void DMA1_Stream5_IRQHandler(void)

{

    if (DMA1->HISR & DMA_HISR_TCIF5)

    {

        DMA1->HIFCR |= DMA_HIFCR_CTCIF5;


        /* Echo received data */

        uart2_send_dma(rxBuf, RX_BUF_SIZE);


        /* Restart RX */

        DMA1_Stream5->CR &= ~DMA_SxCR_EN;

        while (DMA1_Stream5->CR & DMA_SxCR_EN);


        DMA1_Stream5->NDTR = RX_BUF_SIZE;

        DMA1_Stream5->CR |= DMA_SxCR_EN;

    }

}


/* ================= MAIN ================= */

int main(void)

{

    uart2_dma_init();


    /* Send initial message */

    while (!txDone);

    uart2_send_dma((uint8_t*)MSG, sizeof(MSG) - 1);


    /* Start receiving */

    uart2_start_rx();


    /* Main loop */

    while (1)

    {

        __WFI();  // sleep until interrupt

    }

}

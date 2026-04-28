#include "stm32f446xx.h"

void GPIO_Init(void)
{
    /* Enable GPIOA and GPIOC clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    /* PA5 = output (LD2 LED) */
    GPIOA->MODER &= ~(3UL << (5 * 2));
    GPIOA->MODER |=  (1UL << (5 * 2));

    /* PC13 = input (B1 button) */
    GPIOC->MODER &= ~(3UL << (13 * 2));
}

void EXTI13_Init(void)
{
    /* Enable SYSCFG clock */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* Route PC13 to EXTI13
       EXTI13 uses EXTICR[3], bits [7:4]
       Port C = 0010
    */
    SYSCFG->EXTICR[3] &= ~(0xF << 4);
    SYSCFG->EXTICR[3] |=  (0x2 << 4);

    /* Unmask EXTI13 */
    EXTI->IMR |= (1UL << 13);

    /* Select falling edge trigger */
    EXTI->FTSR |= (1UL << 13);

    /* Disable rising edge trigger */
    EXTI->RTSR &= ~(1UL << 13);

    /* Enable EXTI15_10 interrupt in NVIC */
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    /* Optional: set priority */
    NVIC_SetPriority(EXTI15_10_IRQn, 2);
}

void EXTI15_10_IRQHandler(void)
{
    /* Check if EXTI13 caused the interrupt */
    if (EXTI->PR & (1UL << 13))
    {
        /* Clear pending flag by writing 1 */
        EXTI->PR = (1UL << 13);

        /* Toggle PA5 */
        GPIOA->ODR ^= (1UL << 5);
    }
}

int main(void)
{
    GPIO_Init();
    EXTI13_Init();

    while (1)
    {
        /* main loop does nothing
           interrupt handles button press */
    }
}

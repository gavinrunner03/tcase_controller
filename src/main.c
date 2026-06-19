#include "main.h"
static volatile uint8_t motor_target_position = 255;
static volatile uint8_t user_target_position = 255;
/* PC13 debounce */
static volatile uint8_t PC13_DEBOUNCE_PENDING = 0;
static volatile uint32_t PC13_LAST_EDGE_MS = 0;

static volatile uint8_t PC13_STABLE_STATE = 1; /* 1 = released, 0 = pressed */
static volatile uint32_t g_msticks = 0;
int main(void)
{
    HAL_Init();
    SysTick_1ms_Init();    /* Systick to 1ms interrupts*/
    MX_USART2_UART_Init(); /* Debug Printf */
    gpio_c_pins_init();
#ifdef DEBUG_MAIN
    printf("GPIOC Inputs Initialized\r\n");
#endif
    gpio_b_pins_init();
#ifdef DEBUG_MAIN
    printf("GPIOB Outputs Initialized\r\n");
#endif
    HAL_Delay(1000); /* Wait 1 second for everything to stabilize*/
#ifdef DEBUG_MAIN
    printf("Boot Successful\r\n");
#endif
    while (1)
    {
        PC13_press_routine(); /* Check if PC13 is pressed, do somoe stuff, and reset it */
    }
    return 0;
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    g_msticks++;
    /* Keep systick simple, just scan input registers and set the global position stage
     * position is the variable containing the state of the target position, ie the motor
     * has spun to this position, but it is not confirmable yet until we read feedback from the
     * other inputs. Not to be confused with the user_target_position!
     */

    /* 6 Pin Connector Polling */
    if ((!(GPIOC->IDR & (GPIO_PIN_3))) && (GPIOC->IDR & (GPIO_PIN_6)))
    {
        /*
         * if ONLY P3 is active (low) then the target mode is the AWD position, only
         * P6 is relavent for error checking because it has an overlap with 4x4 Locked mode.
         */
        motor_target_position = MODE_AWD;
    }
    else if ((!(GPIOC->IDR & (GPIO_PIN_3))) && (!(GPIOC->IDR & (GPIO_PIN_6))))
    {
        /*
         * If P3 AND P6 are both active low then the target mode is the 4x4 locked position.
         * Other pins are irrelavent because there is no overlap in mode selection
         */

        motor_target_position = MODE_4X4;
    }
    else if ((!(GPIOC->IDR & (GPIO_PIN_4))) && (!(GPIOC->IDR & (GPIO_PIN_5))) && (!(GPIOC->IDR & (GPIO_PIN_6))))
    {
        /*
         * If P4, P5, and P6 are active then the target mode is the 2WD position
         * P3 is not relavent here, and there are no other modes that use 4 5 and 6 so other pin checks
         * are irrelavent
         */
        motor_target_position = MODE_AWD;
    }
}

void gpio_c_pins_init()
{
    /*
     * PC0 input for MISC Sensor
     * PC1 input for PIN3
     * PC2 input for PIN5
     * PC3 input for PIN6
     * PC4 input for MISC Sensor
     * PC5 input for MISC Sensor
     * PC6 input for MISC Sensor
     * PC13 input for STM blue onboard-debug button
     */
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*
     * PC13 blue onboard debug button
     * Interrupt on rising and falling edge.
     */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*
     * PC13 is on EXTI line 13, which uses EXTI15_10_IRQn.
     */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}


void gpio_b_pins_init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*
     * Transfer case GPIO outputs:
     *
     * PB12 - INU_1
     * PB13 - INV_1
     * PB14 - INH_U
     * PB15 - INH_V
     */

    HAL_GPIO_WritePin(GPIOB,
                      GPIO_PIN_12 |
                          GPIO_PIN_13 |
                          GPIO_PIN_14 |
                          GPIO_PIN_15,
                      GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_12 |
                          GPIO_PIN_13 |
                          GPIO_PIN_14 |
                          GPIO_PIN_15;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void PC13_press_routine(void)
{
    uint8_t pc13_now;

    if (PC13_DEBOUNCE_PENDING)
    {
        if ((HAL_GetTick() - PC13_LAST_EDGE_MS) >= 50U)
        {
            PC13_DEBOUNCE_PENDING = 0;

            /*
             * PC13 active low:
             * 0 = pressed
             * 1 = released
             */
            if (GPIOC->IDR & GPIO_PIN_13)
            {
                pc13_now = 1;
            }
            else
            {
                pc13_now = 0;
            }

            /*
             * New stable press.
             * Only print if previous stable state was released.
             */
            if ((pc13_now == 0) && (PC13_STABLE_STATE == 1))
            {
                PC13_STABLE_STATE = 0;
                printf("Button Pressed\r\n");
            }

            /*
             * Stable release.
             * Unlock for the next press.
             */
            else if (pc13_now == 1)
            {
                PC13_STABLE_STATE = 1;
            }
        }
    }
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13)
    {
        /*
         * Every bounce edge restarts the debounce timer.
         * Do not print in here.
         */
        PC13_LAST_EDGE_MS = HAL_GetTick();
        PC13_DEBOUNCE_PENDING = 1;
    }
}
#include "main.h"
static volatile uint8_t motor_target_position = 255;
static volatile uint8_t user_target_position = 255;
/* PC13 debounce */
static volatile uint8_t PC13_DEBOUNCE_PENDING = 0;
static volatile uint32_t PC13_LAST_EDGE_MS = 0;

static volatile uint8_t PC13_STABLE_STATE = 1; /* 1 = released, 0 = pressed */
static volatile uint16_t DEBUG_BTN_COUNT = 2;  /* for debug modulo 3 state button select */
static volatile uint16_t last_debug_btn_count = 2;
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
        if (last_debug_btn_count != DEBUG_BTN_COUNT)
        {
            last_debug_btn_count = DEBUG_BTN_COUNT;
            switch (DEBUG_BTN_COUNT % 3)
            {
            case 0:
#ifdef DEBUG_MAIN
                printf("[SHIFT] Shifting from %d to %d\r\n", motor_target_position, MODE_2WD);
#endif
                shift_position(motor_target_position, MODE_2WD);
                break;

            case 1:
#ifdef DEBUG_MAIN
                printf("[SHIFT] Shifting from %d to %d\r\n", motor_target_position, MODE_4X4);
#endif
                shift_position(motor_target_position, MODE_4X4);
                break;

            case 2:
#ifdef DEBUG_MAIN
                printf("[SHIFT] Shifting from %d to %d\r\n", motor_target_position, MODE_AWD);
#endif
                shift_position(motor_target_position, MODE_AWD);
                break;

            default:
#ifdef DEBUG_MAIN
                printf("[ERROR] EMERGENCY SHIFT TO 2WD | %d to %d\r\n", motor_target_position, MODE_2WD);
#endif
                break;
            }
        }
    }
    return 0;
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    g_msticks++;

    uint8_t p3_active = ((GPIOC->IDR & GPIO_PIN_1) == 0);
    uint8_t p5_active = ((GPIOC->IDR & GPIO_PIN_2) == 0);
    uint8_t p6_active = ((GPIOC->IDR & GPIO_PIN_3) == 0);

    /*
     * Exact valid sensor patterns:
     *
     * P3 only      = AWD
     * P3 + P6 only = 4X4
     * P5 + P6 only = 2WD
     */
    if ((p3_active) && (!p5_active) && (!p6_active))
    {
        motor_target_position = MODE_AWD;
    }
    else if ((p3_active) && (!p5_active) && (p6_active))
    {
        motor_target_position = MODE_4X4;
    }
    else if ((!p3_active) && (p5_active) && (p6_active))
    {
        motor_target_position = MODE_2WD;
    }
    else
    {
        motor_target_position = UNKNOWN;
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
    __HAL_RCC_SYSCFG_CLK_ENABLE();

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
                DEBUG_BTN_COUNT++;
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

void shift_position(uint8_t current_target, uint8_t new_target)
{
    uint32_t start_time = HAL_GetTick();
    uint32_t timeout_ms = SHIFT_TIMEOUT_ADJACENT_MS;
    uint32_t overtravel_ms = 0U;
    uint16_t direction = 0U;

    if (current_target == new_target)
    {
        GPIOB->ODR &= ~(FORWARD | BACKWARD);
        GPIOB->ODR &= ~(ENABLE_U | ENABLE_V);
        return;
    }

    if ((current_target == MODE_AWD) && (new_target == MODE_4X4))
    {
        direction = BACKWARD;
        timeout_ms = SHIFT_TIMEOUT_ADJACENT_MS;
    }
    else if ((current_target == MODE_AWD) && (new_target == MODE_2WD))
    {
        direction = BACKWARD;
        timeout_ms = SHIFT_TIMEOUT_CROSS_MS;
    }
    else if ((current_target == MODE_4X4) && (new_target == MODE_AWD))
    {
        direction = FORWARD;
        timeout_ms = SHIFT_TIMEOUT_ADJACENT_MS;
    }
    else if ((current_target == MODE_4X4) && (new_target == MODE_2WD))
    {
        direction = BACKWARD;
        timeout_ms = SHIFT_TIMEOUT_ADJACENT_MS;
    }
    else if ((current_target == MODE_2WD) && (new_target == MODE_AWD))
    {
        direction = FORWARD;
        timeout_ms = SHIFT_TIMEOUT_CROSS_MS;
    }
    else if ((current_target == MODE_2WD) && (new_target == MODE_4X4))
    {
        direction = FORWARD;
        timeout_ms = SHIFT_TIMEOUT_ADJACENT_MS;
    }
    else if (current_target == UNKNOWN)
    {
        direction = BACKWARD;
        new_target = MODE_2WD;
        timeout_ms = SHIFT_TIMEOUT_ADJACENT_MS;
    }
    else
    {
        GPIOB->ODR &= ~(FORWARD | BACKWARD);
        GPIOB->ODR &= ~(ENABLE_U | ENABLE_V);
        return;
    }

    while ((motor_target_position != new_target) &&
           ((HAL_GetTick() - start_time) < timeout_ms))
    {
        GPIOB->ODR &= ~(FORWARD | BACKWARD);
        GPIOB->ODR |= (ENABLE_U | ENABLE_V);
        GPIOB->ODR |= direction;
    }

    if (motor_target_position == new_target)
    {
        if (new_target == MODE_AWD)
        {
            overtravel_ms = SHIFT_OVERTRAVEL_AWD_MS;
        }
        else if (new_target == MODE_2WD)
        {
            overtravel_ms = SHIFT_OVERTRAVEL_2WD_MS;
        }
        else
        {
            overtravel_ms = SHIFT_OVERTRAVEL_4X4_MS;
        }

        start_time = HAL_GetTick();

        while ((HAL_GetTick() - start_time) < overtravel_ms)
        {
            GPIOB->ODR &= ~(FORWARD | BACKWARD);
            GPIOB->ODR |= (ENABLE_U | ENABLE_V);
            GPIOB->ODR |= direction;
        }
    }

    GPIOB->ODR &= ~(FORWARD | BACKWARD);
    GPIOB->ODR &= ~(ENABLE_U | ENABLE_V);

#ifdef DEBUG_MAIN
    if (motor_target_position != new_target)
    {
        printf("[FAULT] Shift failed or timed out. Current=%d Target=%d\r\n",
               motor_target_position,
               new_target);
    }
    else
    {
        printf("[SHIFT] Complete. Current=%d Target=%d\r\n",
               motor_target_position,
               new_target);
    }
#endif
}
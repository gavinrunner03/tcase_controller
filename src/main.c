#include "main.h"
static volatile uint8_t motor_target_position = 255;
static volatile uint8_t user_target_position = 255;
/* PC13 debounce */
static volatile uint8_t PC13_DEBOUNCE_PENDING = 0;
static volatile uint32_t PC13_LAST_EDGE_MS = 0;

static volatile uint8_t PC13_STABLE_STATE = 1; /* 1 = released, 0 = pressed */

/* PC4 debounce */
static volatile uint8_t PC4_DEBOUNCE_PENDING = 0;
static volatile uint32_t PC4_LAST_EDGE_MS = 0;

static volatile uint8_t PC4_STABLE_STATE = 1; /* 1 = released, 0 = pressed */

static volatile uint16_t DEBUG_BTN_COUNT = 2; /* for debug modulo 3 state button select */
static volatile uint16_t last_debug_btn_count = 2;
static volatile uint16_t last_pc4_btn_count = 2;
static volatile uint16_t PC4_BTN_COUNT = 0;
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
        PC13_press_routine();
        PC4_press_routine();

        if (last_pc4_btn_count != PC4_BTN_COUNT)
        {
            last_pc4_btn_count = PC4_BTN_COUNT;

            printf("Stable 4WD Button state: %d\r\n", PC4_STABLE_STATE);

            if (PC4_STABLE_STATE == 0)
            {
                /*
                 * 4WD toggle/button turned ON.
                 * Go to AWD first.
                 */
                printf("Attempting AWD\r\n");
                shift_position(motor_target_position, MODE_AWD);
            }
            else
            {
                /*
                 * 4WD toggle/button turned OFF.
                 * Return to 2WD.
                 */
                printf("Attempting 2WD\r\n");
                shift_position(motor_target_position, MODE_2WD);
            }
        }

        if (last_debug_btn_count != DEBUG_BTN_COUNT)
        {
            last_debug_btn_count = DEBUG_BTN_COUNT;

            printf("Stable DEBUG Button state: %d\r\n", PC13_STABLE_STATE);

            if (PC13_STABLE_STATE == 0)
            {
                /*
                 * Diff lock button pressed.
                 * Only lock if 4WD is ON and current position is AWD.
                 */
                if ((PC4_STABLE_STATE == 0) && (motor_target_position == MODE_AWD))
                {
                    printf("Attempting Center Differential Lock\r\n");
                    shift_position(motor_target_position, MODE_4X4);
                }
                else
                {
                    printf("Center Diff Lock Button Press Ignored, AWD pre-state not met\r\n");
                }
            }
            else
            {
                /*
                 * Diff lock button released.
                 * If 4WD is still ON and we are locked, unlock back to AWD.
                 */
                if ((PC4_STABLE_STATE == 0) && (motor_target_position == MODE_4X4))
                {
                    printf("Attempting Center Differential Un-Lock\r\n");
                    shift_position(motor_target_position, MODE_AWD);
                }
                else
                {
                    printf("Center Diff Unlock Ignored\r\n");
                }
            }
        }
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    g_msticks++;

    uint8_t p3_active = ((GPIOC->IDR & GPIO_PIN_1) == 0);
    uint8_t p5_active = ((GPIOC->IDR & GPIO_PIN_2) == 0);
    uint8_t p6_active = ((GPIOC->IDR & GPIO_PIN_3) == 0);

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

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*
     * PC4 4WD button
     * Interrupt on rising and falling edge.
     */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
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
     * PC4 is on EXTI line 4.
     */
    HAL_NVIC_SetPriority(EXTI4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);

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

void PC4_press_routine(void)
{
    uint8_t pc4_now;

    if (PC4_DEBOUNCE_PENDING)
    {
        if ((HAL_GetTick() - PC4_LAST_EDGE_MS) >= 50U)
        {
            PC4_DEBOUNCE_PENDING = 0;

            /*
             * PC4 active low:
             * 0 = pressed
             * 1 = released
             */
            if (GPIOC->IDR & GPIO_PIN_4)
            {
                pc4_now = 1;
            }
            else
            {
                pc4_now = 0;
            }

            /*
             * Count every stable state change.
             * Press counts once.
             * Release counts once.
             */
            if (pc4_now != PC4_STABLE_STATE)
            {
                PC4_STABLE_STATE = pc4_now;
                PC4_BTN_COUNT++;

                if (PC4_STABLE_STATE == 0)
                {
                    printf("PC4 Button Pressed\r\n");
                }
                else
                {
                    printf("PC4 Button Released\r\n");
                }
            }
        }
    }
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
             * Count every stable state change.
             * Press counts once.
             * Release counts once.
             */
            if (pc13_now != PC13_STABLE_STATE)
            {
                PC13_STABLE_STATE = pc13_now;
                DEBUG_BTN_COUNT++;

                if (PC13_STABLE_STATE == 0)
                {
                    printf("PC13 Button Pressed\r\n");
                }
                else
                {
                    printf("PC13 Button Released\r\n");
                }
            }
        }
    }
}

void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
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
    else if (GPIO_Pin == GPIO_PIN_4)
    {
        /*
         * Every bounce edge restarts the debounce timer.
         * Do not print in here.
         */
        PC4_LAST_EDGE_MS = HAL_GetTick();
        PC4_DEBOUNCE_PENDING = 1;
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
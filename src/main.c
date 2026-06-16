#include "main.h"

extern volatile uint8_t machine_mode;
extern volatile uint8_t run_target;
extern volatile uint8_t *p_run_target;

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    /*
     * Optional.
     * HAL_Init() usually already does this,
     * but calling your own init makes it explicit.
     */
    SysTick_1ms_Init();

    MX_USART2_UART_Init();
    gpio_init();
    HAL_Delay(1000);

    machine_mode = 0;
    run_target = 0;
    inputs input_values = {0, 0, 0, 0, 0, 0, 0};
    outputs output_values = {0};
    state current_state = {0, 0};
    watchdog_setup();

    while(1){
        switch(machine_mode){
            case POLL:
                poll_inputs(&input_values);
                update_outputs(&input_values, &current_state, p_run_target);
                break;
            case RUN:
                run_motor(run_target, &input_values, &output_values);
        }
    }
    return 0;
}


void SysTick_Handler(void)
{
    HAL_IncTick();
}
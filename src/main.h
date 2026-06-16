#ifndef MAIN_H
#define MAIN_H

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include "peripherals.h"
#include <stdint.h>

#include <stdint.h>

/* Mode Definition */
#define HIGH_RANGE 0
#define LOW_RANGE 1
#define MODE_2WD 0
#define MODE_AWD 1
#define MODE_4WD 2
#define POLL 0
#define RUN 1
#define MODE_ERROR 99
/* Definitions of motor running states */
/* From 2WD -> Anything */
#define F2WDT2WD 0
#define F2WDTAWD 1
#define F2WDT4WD 2
/* From AWD -> Anything */
#define FAWDT2WD 3
#define FAWDTAWD 4
#define FAWDT4WD 5
/* From 4WD -> Anything */
#define F4WDT2WD 6
#define F4WDTAWD 7
#define F4WDT4WD 8

/* Definitions for default settings */
#define DEFAULT_SPEED 50
#define REVERSE 1
#define FORWARD 2
#define OFF 0
/* Begin Input/Output Structures */
typedef struct
{
    int position_2wd;          /*          PC7          |  This will be 1 when 2wd is engaged, 0 otherwise */
    int position_awd;          /*          PB4          |  This will be 1 when awd/4wd (4wd open) is engaged, 0 otherwise */
    int position_4wd;          /*          PA10         |  This will be 1 when 4wd (4wd locked) is engaged, 0 otherwise */
    int differential_lock;     /*     PB3          |  This will be 1 when the differential lock button is pressed (inside cabin) */
    int mode_select;           /*           PC4          |  This will be 1 when 4WD mode button on the shifter is pressed */
    int low_range;             /*             PB0          |  This is an input from the transfer to let the MCU know that the shifter is in the LOW position,
                                                           |  immediately put the vehicle in 4WD  (locked) mode to protect the gears. */
    int neutral_safety_switch; /* PA9          |  This is an input from the transmission to let the MCU know that the transmission
                                               |  is in the NEUTRAL position, does not serve a real purpose since lo-range is mechanically actuated anyways*/
} inputs;

typedef struct
{
    /*
     *  PB5  | Motor Phase 1 Control
     *  PA8  | Motor Phase 2 Control
     *  PC6  | Global Chip Wakeup
     */
    int motor_direction; // This will be 2 for forward, 1 for reverse, and 0 for stop
    int motor_speed;     // This will be a value from 0 to 100 representing the speed of the motor
} outputs;

typedef struct
{
    int current_mode; // This will be 0 for 2wd, 1 for awd, and 2 for 4wd
    int target_mode;  // This will be 0 for 2wd, 1 for awd, and 2 for 4wd
} state;

/* Globals */

volatile uint8_t machine_mode;
volatile uint8_t run_target;
volatile uint8_t *p_run_target = &run_target;
volatile uint8_t position_4WD_confirmed;        /*PC13*/
volatile uint8_t position_LO_confirmed;         /*PC14*/
/* Begin Function Prototypes */
void watchdog_setup();
void poll_inputs(inputs *input_values);
void update_outputs(inputs *input_values, state *current_states, volatile uint8_t *p_run_target);
int gpio_init(void);
void run_motor(volatile uint8_t run_target, inputs *input_values, outputs *output_values);
void motor_ctrl(outputs *output_values);
/* Begin Function Definitions */

int gpio_init(void)
{
    /* Enable GPIO Clocks */
    RCC->AHB1ENR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
    /* Configure GPIO Pins */
    /* INPUTS */
    // PC7, PB4, PA10 as inputs for position sensors
    GPIOC->MODER &= ~(3 << (7 * 2));  // Clear mode for PC7
    GPIOB->MODER &= ~(3 << (4 * 2));  // Clear mode for PB4
    GPIOA->MODER &= ~(3 << (10 * 2)); // Clear mode for PA10
    // PB3, PC4 as inputs for buttons
    GPIOB->MODER &= ~(3 << (3 * 2)); // Clear mode for PB3
    GPIOC->MODER &= ~(3 << (4 * 2)); // Clear mode for PC4
    // PB0 as input for low range
    GPIOB->MODER &= ~(3 << (0 * 2)); // Clear mode for PB0
    // PA9 as input for neutral safety switch
    GPIOA->MODER &= ~(3 << (9 * 2)); // Clear mode for PA9

    /* Resistor PUPDR */
    // PC7, PB4, PA10 with pull-down resistors for position sensors
    GPIOC->PUPDR &= ~(3 << (7 * 2));  // Clear PUPDR for PC7
    GPIOC->PUPDR |= (2 << (7 * 2));   // Set pull-down for PC7
    GPIOB->PUPDR &= ~(3 << (4 * 2));  // Clear PUPDR for PB4
    GPIOB->PUPDR |= (2 << (4 * 2));   // Set pull-down for PB4
    GPIOA->PUPDR &= ~(3 << (10 * 2)); // Clear PUPDR for PA10
    GPIOA->PUPDR |= (2 << (10 * 2));  // Set pull-down for PA10
    // PB3, PC4, PB0, PA9 with pull-up resistors for buttons
    GPIOB->PUPDR &= ~(3 << (3 * 2)); // Clear PUPDR for PB3
    GPIOB->PUPDR |= (1 << (3 * 2));  // Set pull-up for PB3
    GPIOC->PUPDR &= ~(3 << (4 * 2)); // Clear PUPDR for PC4
    GPIOC->PUPDR |= (1 << (4 * 2));  // Set pull-up for PC4
    GPIOB->PUPDR &= ~(3 << (0 * 2)); // Clear PUPDR for PB0
    GPIOB->PUPDR |= (1 << (0 * 2));  // Set pull-up for PB0
    GPIOA->PUPDR &= ~(3 << (9 * 2)); // Clear PUPDR for PA9
    GPIOA->PUPDR |= (1 << (9 * 2));  // Set pull-up for PA9

    /* OUTPUTS */
    // PB5, PA8 as outputs for motor control
    GPIOB->MODER &= ~(3 << (5 * 2)); // Clear mode for PB5
    GPIOB->MODER |= (1 << (5 * 2));  // Set PB5 as output
    GPIOA->MODER &= ~(3 << (8 * 2)); // Clear mode for PA8
    GPIOA->MODER |= (1 << (8 * 2));  // Set PA8 as output
    // PC6 as output for global chip wakeup
    GPIOC->MODER &= ~(3 << (6 * 2)); // Clear mode for PC6
    GPIOC->MODER |= (1 << (6 * 2));  // Set PC6 as output

    return 0; // Return 0 for success
}

void watchdog_setup()
{
    /* Watchdog timer so that the system resets if for some reaason the
    MCU gets stuck. Once reset, the default mode will be AWD */
}

void poll_inputs(inputs *input_values)
{
    /* Check Sensors */
    if (GPIOC->IDR & (1 << 7))
    { // Check PC7
        input_values->position_2wd = 1;
    }
    else
    {
        input_values->position_2wd = 0;
    }
    if (GPIOB->IDR & (1 << 4))
    { // Check PB4
        input_values->position_awd = 1;
    }
    else
    {
        input_values->position_awd = 0;
    }
    if (GPIOA->IDR & (1 << 10))
    { // Check PA10
        input_values->position_4wd = 1;
    }
    else
    {
        input_values->position_4wd = 0;
    }
    if ((GPIOB->IDR & (1 << 0)))
    { // Check PB0
        input_values->low_range = 1;
    }
    else
    {
        input_values->low_range = 0;
    }
    if ((GPIOA->IDR & (1 << 9)))
    { // Check PA9
        input_values->neutral_safety_switch = 1;
    }
    else
    {
        input_values->neutral_safety_switch = 0;
    }

    /* check buttons */
    if (!(GPIOB->IDR & (1 << 3)))
    { // Check PB3
        input_values->differential_lock = 1;
    }
    else
    {
        input_values->differential_lock = 0;
    }
    if (!(GPIOC->IDR & (1 << 4)))
    { // Check PC4
        input_values->mode_select = 1;
    }
    else
    {
        input_values->mode_select = 0;
    }
}

void update_outputs(inputs *input_values, state *current_states, volatile uint8_t *p_run_target)
{
    /* The combinations:  Mode |             Condition                  | Output
                          2WD  | 4WD button OFF, Center Diff button OFF | 2WD Sensor HIGH
                          AWD  | 4WD button ON, Center Diff button OFF  | AWD Sensor HIGH
                          4WD  | 4WD button ON, Center Diff button ON   | 4WD Sensor HIGH
                        4WD LO | Low range button ON                    | 4WD Sensor HIGH
                        */
    if (input_values->position_2wd)
    {
        current_states->current_mode = MODE_2WD;
    }
    else if (input_values->position_awd)
    {
        current_states->current_mode = MODE_AWD;
    }
    else if (input_values->position_4wd)
    {
        current_states->current_mode = MODE_4WD;
    }
    else
    {
        current_states->current_mode = MODE_ERROR; // ERROR
    }
    /*
     * Determine the target based on the inputs
     * Determines the target mode based on the button inputs and also the
     * LOW range sensor override. If the diff lock sensor is active, then
     * the system should immediately go to 4WD (locked) mode to protect the gears.
     */
    if (input_values->low_range)
    {
        current_states->target_mode = MODE_4WD;
    }
    else if (input_values->mode_select && input_values->differential_lock)
    {
        current_states->target_mode = MODE_4WD;
    }
    else if (input_values->mode_select)
    {
        current_states->target_mode = MODE_AWD;
    }
    else
    {
        current_states->target_mode = MODE_2WD;
    }

    switch (current_states->current_mode)
    {
    case MODE_2WD:
        /* Currently in 2WD */
        switch (current_states->target_mode)
        {
        case MODE_2WD:
            /* Do nothing */
            *p_run_target = F2WDT2WD;
            machine_mode = POLL;
            break;

        case MODE_AWD:
            /* Shift forward until position awd is high */
            *p_run_target = F2WDTAWD;
            machine_mode = RUN;
            break;

        case MODE_4WD:
            /* Shift Forward until position 4wd is high */
            *p_run_target = F2WDT4WD;
            machine_mode = RUN;
            break;
        }
        break;
    case MODE_AWD:
        /* Currently in AWD */
        switch (current_states->target_mode)
        {
        case MODE_2WD:
            /* Shift backward until position 2wd is high */
            *p_run_target = FAWDT2WD;
            machine_mode = RUN;
            break;

        case MODE_AWD:
            /* Do nothing */
            *p_run_target = FAWDTAWD;
            machine_mode = POLL;
            break;

        case MODE_4WD:
            /* Shift Forward until position 4wd is high */
            *p_run_target = FAWDT4WD;
            machine_mode = RUN;
        }
        break;
    case MODE_4WD:
        /* Currently in 4WD Locked*/
        switch (current_states->target_mode)
        {
        case MODE_2WD:
            /* Shift backward until position 2wd is high */
            *p_run_target = F4WDT2WD;
            machine_mode = RUN;
            break;

        case MODE_AWD:
            /* Shift backward until position awd is high */
            *p_run_target = F4WDTAWD;
            machine_mode = RUN;
            break;

        case MODE_4WD:
            /* Do nothing */
            *p_run_target = F4WDT4WD;
            machine_mode = POLL;
            break;
        }
        break;
    case MODE_ERROR:
        /* ERROR!! 2WD FORCED MOTOR UNTIL SENSORS VALID */
        /* Handle later */
        machine_mode = RUN;
        break;
    }
}

void run_motor(volatile uint8_t run_target, inputs *input_values, outputs *output_values)
{
    /* This function can be used to run the motor based on the current outputs */
    output_values->motor_speed = DEFAULT_SPEED; // default speed
    switch (run_target)
    {
    case F2WDT2WD:
        /* From 2WD to 2WD */
        /* From 2WD to 2WD */
        output_values->motor_direction = OFF; // stop
        while (!(input_values->position_2wd))
        {
            motor_ctrl(output_values);
            poll_inputs(input_values);
        }
        break;

    case F2WDTAWD:
        /*From 2WD to AWD */
        /* Run Forward, check for AWD Sensor */
        output_values->motor_direction = FORWARD; // forward
        while (!(input_values->position_awd))
        {
            motor_ctrl(output_values);
            poll_inputs(input_values);
        }
        break;

    case F2WDT4WD:
        /* From 2WD to 4WD */
        /* Run forward, check for 4WD sensor */
        output_values->motor_direction = FORWARD; // forward
        while (!(input_values->position_4wd))
        {
            motor_ctrl(output_values);
            poll_inputs(input_values);
        }
        break;

    case FAWDT2WD:
        /* From AWD to 2WD */
        /* Run backward, check for 2WD sensor */
        output_values->motor_direction = REVERSE; // reverse
        while (!(input_values->position_2wd))
        {
            motor_ctrl(output_values);
            poll_inputs(input_values);
        }
        break;

    case FAWDTAWD:
        /* From AWD to AWD */
        output_values->motor_direction = OFF; // stop
        while (!(input_values->position_awd))
        {
            motor_ctrl(output_values);
            poll_inputs(input_values);
        }
        break;

    case FAWDT4WD:
        /* From AWD to 4WD */
        output_values->motor_direction = FORWARD; // forward
        while (!(input_values->position_4wd))
        {
            motor_ctrl(output_values);
            poll_inputs(input_values);
        }
        break;

    case F4WDT2WD:
        /* From 4WD to 2WD */
        /* Run backward, check for 2wd sensor */
        output_values->motor_direction = REVERSE; // reverse
        while (!(input_values->position_2wd))
        {
            motor_ctrl(output_values);
            poll_inputs(input_values);
        }
        break;

    case F4WDTAWD:
        /* From 4WD to AWD */
        /* Run backward, check for awd sensor */
        output_values->motor_direction = REVERSE; // reverse
        while (!(input_values->position_awd))
        {   
            motor_ctrl(output_values);
            poll_inputs(input_values);
        }
        break;

    case F4WDT4WD:
        /* From 4WD to 4WD */
        output_values->motor_direction = OFF; // stop
        while (!(input_values->position_4wd))
        {
            motor_ctrl(output_values);
            poll_inputs(input_values);
        }
        
        break;
    }
    output_values->motor_direction = OFF;
    output_values->motor_speed = OFF;
    motor_ctrl(output_values);
    machine_mode = POLL;
}

void motor_ctrl(outputs *output_values){
    /* MOTOR GPIO CONTROL SET TO REFLECT THE OUTPUT VALUES STRUCT */
        /*
     *  PB5  | Motor Phase 1 Control
     *  PA8  | Motor Phase 2 Control
     *  PC6  | Global Chip Wakeup
     */
    if(output_values->motor_direction == OFF){
        /* ALL OFF */
        GPIOB->ODR &=~(0x1 << (5));
        GPIOA->ODR &=~(0x1 << (8));
        GPIOC->ODR &=~(0x1 << (6));
    }
    else if(output_values->motor_direction == REVERSE){
        /* ALL OFF */
        GPIOB->ODR &=~(0x1 << (5)); //P1
        GPIOA->ODR &=~(0x1 <<(8)); //P2
        GPIOC->ODR &=~(0x1 << (6)); //CHIP CTRL
        /* REVERSE */
        GPIOB->ODR |= (0x1 << (5)); //P1
        GPIOA->ODR |= (0x0 << (8)); //P2
        GPIOC->ODR |= (0x1 << (6)); //CTRL
    }
    else if(output_values->motor_direction == FORWARD){
        /* ALL OFF */
        GPIOB->ODR &=~(0x1 << (5)); //P1
        GPIOA->ODR &=~(0x1 <<(8)); //P2
        GPIOC->ODR &=~(0x1 << (6)); //CHIP CTRL
        /* FORWARD */
        GPIOB->ODR |= (0x0 << (5)); //P1
        GPIOA->ODR |= (0x1 << (8)); //P2
        GPIOC->ODR |= (0x1 << (6)); //CTRL
    }
    else{
        /* ALL OFF */
        GPIOB->ODR &=~(0x1 << (5)); //P1
        GPIOA->ODR &=~(0x1 <<(8)); //P2
        GPIOC->ODR &=~(0x1 << (6)); //CHIP CTRL
    }
}


#endif
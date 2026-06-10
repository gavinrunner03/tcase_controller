#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

// GPIO Peripheral Structure
typedef struct {
    volatile uint32_t MODER;    // 0x00
    volatile uint32_t OTYPER;   // 0x04
    volatile uint32_t OSPEEDR;  // 0x08
    volatile uint32_t PUPDR;    // 0x0C
    volatile uint32_t IDR;      // 0x10
    volatile uint32_t ODR;      // 0x14
    volatile uint32_t BSRR;     // 0x18
    volatile uint32_t LCKR;     // 0x1C
    volatile uint32_t AFR[2];   // 0x20 & 0x24 
} GPIO_TypeDef;

// RCC Peripheral Structure
typedef struct {
    volatile uint32_t CR;           // 0x00
    volatile uint32_t PLLCFGR;      // 0x04
    volatile uint32_t CFGR;         // 0x08
    volatile uint32_t CIR;          // 0x0C
    volatile uint32_t AHB1RSTR;     // 0x10
    volatile uint32_t AHB2RSTR;     // 0x14
    volatile uint32_t AHB3RSTR;     // 0x18
    uint32_t          RESERVED0;    // 0x1C (Boundary gap alignment)
    volatile uint32_t APB1RSTR;     // 0x20
    volatile uint32_t APB2RSTR;     // 0x24
    uint32_t          RESERVED1[2]; // 0x28 - 0x2C
    volatile uint32_t AHB1ENR;      // 0x30 <-- Clock enable register
    // Note: Remaining RCC registers are omitted for brevity
} RCC_TypeDef;

// Base Addresses
#define PERIPH_BASE       (0x40000000UL)
#define AHB1PERIPH_BASE   (PERIPH_BASE + 0x00020000UL) // 0x40020000
#define RCC_BASE          (AHB1PERIPH_BASE + 0x3800UL) // 0x40023800

#define GPIOA_BASE        (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE        (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE        (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE        (AHB1PERIPH_BASE + 0x0C00UL)

// Peripheral Handles
#define GPIOA             ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB             ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC             ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD             ((GPIO_TypeDef *) GPIOD_BASE)
#define RCC               ((RCC_TypeDef *) RCC_BASE)

/* End STM Register Definitions */
/* Begin Input/Output Structures */
typedef struct {
    int position_2wd; /*          PC7          |  This will be 1 when 2wd is engaged, 0 otherwise */
    int position_awd; /*          PB4          |  This will be 1 when awd/4wd (4wd open) is engaged, 0 otherwise */
    int position_4wd; /*          PA10         |  This will be 1 when 4wd (4wd locked) is engaged, 0 otherwise */
    int differential_lock; /*     PB3          |  This will be 1 when the differential lock button is pressed (inside cabin) */
    int mode_select; /*           PC4          |  This will be 1 when 4WD mode button on the shifter is pressed */
    int low_range; /*             PB0          |  This is an input from the transfer to let the MCU know that the shifter is in the LOW position, 
                                               |  immediately put the vehicle in 4WD  (locked) mode to protect the gears. */
    int neutral_safety_switch; /* PA9          |  This is an input from the transmission to let the MCU know that the transmission
                                               |  is in the NEUTRAL position, does not serve a real purpose since lo-range is mechanically actuated anyways*/
} inputs;

typedef struct {
    /* 
    *  PB5  | Motor Phase 1 Control
    *  PA8  | Motor Phase 2 Control
    *  PC6  | Global Chip Wakeup
    */
    int target_mode; // This will be 0 for 2wd, 1 for awd, and 2 for 4wd
    int motor_direction; // This will be 2 for forward, 1 for reverse, and 0 for stop
    int motor_speed; // This will be a value from 0 to 100 representing the speed of the motor
} outputs;


/* Begin Function Prototypes */
void watchdog_setup();
void poll_mode(inputs *input_values);
void poll_buttons(inputs *input_values);
void update_outputs(inputs *input_values, outputs *output_values);
int gpio_init(void);
/* Begin Function Definitions */

int gpio_init(void){
    /* Enable GPIO Clocks */
    RCC->AHB1ENR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
    /* Configure GPIO Pins */
    /* INPUTS */
    // PC7, PB4, PA10 as inputs for position sensors
    GPIOC->MODER &= ~(3 << (7 * 2)); // Clear mode for PC7
    GPIOB->MODER &= ~(3 << (4 * 2)); // Clear mode for PB4
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
    GPIOC->PUPDR &= ~(3 << (7 * 2)); // Clear PUPDR for PC7
    GPIOC->PUPDR |= (2 << (7 * 2)); // Set pull-down for PC7
    GPIOB->PUPDR &= ~(3 << (4 * 2)); // Clear PUPDR for PB4
    GPIOB->PUPDR |= (2 << (4 * 2)); // Set pull-down for PB4
    GPIOA->PUPDR &= ~(3 << (10 * 2)); // Clear PUPDR for PA10
    GPIOA->PUPDR |= (2 << (10 * 2)); // Set pull-down for PA10
    // PB3, PC4, PB0, PA9 with pull-up resistors for buttons
    GPIOB->PUPDR &= ~(3 << (3 * 2)); // Clear PUPDR for PB3
    GPIOB->PUPDR |= (1 << (3 * 2)); // Set pull-up for PB3
    GPIOC->PUPDR &= ~(3 << (4 * 2)); // Clear PUPDR for PC4
    GPIOC->PUPDR |= (1 << (4 * 2)); // Set pull-up for PC4
    GPIOB->PUPDR &= ~(3 << (0 * 2)); // Clear PUPDR for PB0
    GPIOB->PUPDR |= (1 << (0 * 2)); // Set pull-up for PB0
    GPIOA->PUPDR &= ~(3 << (9 * 2)); // Clear PUPDR for PA9
    GPIOA->PUPDR |= (1 << (9 * 2)); // Set pull-up for PA9


    /* OUTPUTS */
    // PB5, PA8 as outputs for motor control
    GPIOB->MODER &= ~(3 << (5 * 2)); // Clear mode for PB5
    GPIOB->MODER |= (1 << (5 * 2)); // Set PB5 as output
    GPIOA->MODER &= ~(3 << (8 * 2)); // Clear mode for PA8
    GPIOA->MODER |= (1 << (8 * 2)); // Set PA8 as output
    // PC6 as output for global chip wakeup
    GPIOC->MODER &= ~(3 << (6 * 2)); // Clear mode for PC6
    GPIOC->MODER |= (1 << (6 * 2)); // Set PC6 as output

    return 0;
}

void watchdog_setup(){
    /* Watchdog timer so that the system resets if for some reaason the 
    MCU gets stuck. Once reset, the default mode will be AWD */
}

void poll_mode(inputs *input_values){
    //mode select = button poll
    //diff lock = button poll
    if(input_values->mode_select == 0){
        input_values->mode_select = 0; // 2WD, ignore the diff lock button since it does not affect 2WD mode
    }
    else if(input_values->mode_select == 1){
        input_values->mode_select = input_values->mode_select + input_values->differential_lock; //diff lock can be either 0 or 1, the mode select is also 0 or 1, so range is 0->2
    }
    
}
void poll_buttons(inputs *input_values){
    
}

void update_outputs(inputs *input_values, outputs *output_values){
    /* The combinations:  Mode |             Condition                  | Output
                          2WD  | 4WD button OFF, Center Diff button OFF | 2WD Sensor HIGH
                          AWD  | 4WD button ON, Center Diff button OFF  | AWD Sensor HIGH
                          4WD  | 4WD button ON, Center Diff button ON   | 4WD Sensor HIGH
                        4WD LO | Low range button ON                    | 4WD Sensor HIGH
                        */
    switch(input_values->low_range){
        case 0:
        /* The normal case, do not force 4WD Locked*/
            switch(input_values->mode_select){
                case 0:
                /* 2WD Mode */
                    if(input_values->position_awd || input_values->position_4wd){
                        // If the system is currently in AWD or 4WD, we need to reverse the motor to get back to 2WD
                        if(!input_values->position_2wd){
                            output_values->motor_direction = 1; // Enable the motor
                        }
                        while(!input_values->position_2wd){
                            poll_buttons(input_values); // Keep polling the buttons to update the mode select and low range
                            poll_mode(input_values); // Keep polling the mode to update the position sensors
                        }
                        output_values->motor_direction = 0; // Disable the motor once we are in 2WD
                    }
                    else{
                        // If we are already in 2WD, do nothing
                        output_values->motor_direction = 0; // Ensure the motor is disabled
                    }
                break;

                case 1:
                /* AWD Mode */
                    if(input_values->position_4wd){
                        // If the system is currently in 4WD, we need to reverse the motor to get back to AWD
                        if(!input_values->position_awd){
                            output_values->motor_direction = 1; // reverse the motor if in 4wd
                        }
                        while(!input_values->position_awd){ // Wait until we are out of 4WD and in AWD
                            poll_buttons(input_values); // Keep polling the buttons to update the mode select and low range
                            poll_mode(input_values); // Keep polling the mode to update the position sensors
                        }
                        output_values->motor_direction = 0; // Disable the motor once we are in AWD
                    }
                    else if(input_values->position_2wd){
                        // If the system is currently in 2WD, we need to move the motor forward
                        if(!input_values->position_awd){
                        output_values->motor_direction = 2; // motor forward if in 2wd
                        }
                        while(!input_values->position_awd){
                            poll_buttons(input_values); // Keep polling the buttons to update the mode select and low range
                            poll_mode(input_values); // Keep polling the mode to update the position sensors
                        }
                        output_values->motor_direction = 0; // Disable the motor once the position is reached
                    }
                break;

                case 2:
                /* 4WD Mode */
                    output_values->target_mode = 2;
                    if(!input_values->position_4wd){
                        output_values->motor_direction = 2; // Move forward only if not already in 4WD
                    }
                    output_values->motor_speed = 50; // Set a moderate speed
                    while(!input_values->position_4wd){
                        poll_buttons(input_values); // Keep polling the buttons to update the mode select and low range
                        poll_mode(input_values); // Keep polling the mode to update the position sensors
                    }
                    output_values->motor_direction = 0; // Disable the motor once the position is reached
                break;

            }
        break;

        case 1:
        /* Force 4WD Locked due to Low Range */
            if(input_values->position_2wd){
                // If the system is currently in 2WD, we need to move the motor forward
                output_values->motor_direction = 2; // Enable the motor
            }
            else if(input_values->position_awd){
                // If the system is currently in AWD, we need to move the motor forward
                output_values->motor_direction = 2; // Enable the motor
            }
            output_values->target_mode = 2;
            if(!input_values->position_4wd){
                output_values->motor_direction = 2; // Move forward only if not already in 4WD
            }
            while(!input_values->position_4wd){
                poll_buttons(input_values); // Keep polling the buttons to update the mode select and low range
                poll_mode(input_values); // Keep polling the mode to update the position sensors
            }
            output_values->motor_direction = 0; // Disable the motor once the position is reached
        break;
    }

}
#endif

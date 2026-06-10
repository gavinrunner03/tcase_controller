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

/* Mode Definition */
#define HIGH_RANGE 0 
#define LOW_RANGE 1
#define MODE_2WD    0
#define MODE_AWD    1
#define MODE_4WD    2

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
    int motor_direction; // This will be 2 for forward, 1 for reverse, and 0 for stop
    int motor_speed; // This will be a value from 0 to 100 representing the speed of the motor
} outputs;

typedef struct {
    int current_mode; // This will be 0 for 2wd, 1 for awd, and 2 for 4wd
    int target_mode; // This will be 0 for 2wd, 1 for awd, and 2 for 4wd
} state;



/* Begin Function Prototypes */
void watchdog_setup();
void poll_inputs(inputs *input_values, state *current_states);
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

    return 0; // Return 0 for success
}


void watchdog_setup(){
    /* Watchdog timer so that the system resets if for some reaason the 
    MCU gets stuck. Once reset, the default mode will be AWD */
}

void poll_inputs(inputs *input_values, state *current_states){
    /* Check Sensors */
    if(GPIOC->IDR & (1 << 7)){ // Check PC7
        input_values->position_2wd = 1;
    } else {
        input_values->position_2wd = 0;
    }
    if(GPIOB->IDR & (1 << 4)){ // Check PB4
        input_values->position_awd = 1;
    } else {
        input_values->position_awd = 0;
    }
    if(GPIOA->IDR & (1 << 10)){ // Check PA10
        input_values->position_4wd = 1;
    } else {
        input_values->position_4wd = 0;
    }
    if((GPIOB->IDR & (1 << 0))){ // Check PB0
        input_values->low_range = 1;
    } else {
        input_values->low_range = 0;
    }
    if((GPIOA->IDR & (1 << 9))){ // Check PA9
        input_values->neutral_safety_switch = 1;
    } else {    
        input_values->neutral_safety_switch = 0;
    }

    /* check buttons */
        if(!(GPIOB->IDR & (1 << 3))){ // Check PB3
        input_values->differential_lock = 1;
    } else {
        input_values->differential_lock = 0;
    }
    if(!(GPIOC->IDR & (1 << 4))){ // Check PC4
        input_values->mode_select = 1;
    } else {
        input_values->mode_select = 0;
    }
    /* 
    * Determine the target based on the inputs 
    * Determines the target mode based on the button inputs and also the 
    * LOW range sensor override. If the diff lock sensor is active, then
    * the system should immediately go to 4WD (locked) mode to protect the gears.
    */
    if(input_values->differential_lock && input_values->mode_select){
        current_states->target_mode = MODE_4WD; //MODE 4WD is the LOCKED 4WD mode 
    } 
    else if (input_values->low_range){
        current_states->target_mode = MODE_4WD; // If low range is selected, we want to be in 4WD (locked) mode to protect the gears
    }
    else {
        if(input_values->mode_select){
            current_states->target_mode = MODE_AWD;
        } else {
            current_states->target_mode = MODE_2WD;
        }
    }
}

void update_outputs(inputs *input_values, outputs *output_values){
    /* The combinations:  Mode |             Condition                  | Output
                          2WD  | 4WD button OFF, Center Diff button OFF | 2WD Sensor HIGH
                          AWD  | 4WD button ON, Center Diff button OFF  | AWD Sensor HIGH
                          4WD  | 4WD button ON, Center Diff button ON   | 4WD Sensor HIGH
                        4WD LO | Low range button ON                    | 4WD Sensor HIGH
                        */
    switch(input_values->low_range){
        case LOW_RANGE:
            output_values->target_mode = MODE_4WD;
            break;
        case HIGH_RANGE:
            output_values->target_mode = input_values->mode_select; // This will be 0 for 2WD, 1 for AWD, and 2 for 4WD
            break;
    }

}

void set_motor_speed(int speed){
    /* This function can be used to set the motor speed using PWM if needed, for now we will just use full speed or no speed */

}

void set_motor_direction(int direction){
    /* This function can be used to set the motor direction, for now we will just use forward, reverse, or stop */
}

void run_motor(){
    /* This function can be used to run the motor based on the current outputs, for now we will just set the GPIO pins directly in the update_outputs function */
}
#endif

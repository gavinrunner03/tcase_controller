#ifndef MAIN_H
#define MAIN_H

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include "peripherals.h"
#include <stdint.h>

#include <stdint.h>

#define DEBUG_ALL
//#define DEBUG_MAIN
//#define DEBUG_PERIPHERALS

/* Mode definitions */
#define MODE_2WD 0
#define MODE_4X4 1
#define MODE_AWD 2


/* Function Prototypes */

void gpio_c_pins_init();
/* GPIO C is for the PC817 inputs (sensor inputs)*/

void gpio_b_pins_init();
/* GPIO B is for the INFINEON BLDC SHIELD outputs (motor controler)*/

void onboard_button_setup();
/* Onboard button for bench mode selection */

void watchdog_setup();
/* Watchdog timer to reset during hanging events */

void PC13_press_routine();
#ifdef DEBUG_ALL
#define DEBUG_MAIN
#define DEBUG_PERIPHERALS
#endif
#endif
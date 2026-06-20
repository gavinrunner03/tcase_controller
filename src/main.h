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
#define UNKNOWN 3
#define BACKWARD ((uint16_t) 0x1 << 12)  /* 1 << 12 Phase U Control*/
#define FORWARD  ((uint16_t) 0x1 << 13)  /* 1 << 13 Phase V Control */
#define ENABLE_U ((uint16_t) 0x1 << 14)  /* 1 << 14 INH_U */
#define ENABLE_V ((uint16_t) 0x1 << 15)  /* 1 << 15 INH_V */

/* Timeout Definition */
#define SHIFT_TIMEOUT_ADJACENT_MS  1500U /* Timout for any adjacent manuevre */
#define SHIFT_TIMEOUT_CROSS_MS     3000U /* Timout for any manuevre that goes across 4x4 locked (center, larger area)*/

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
/* Debug button routine for debouncing */

void shift_position(uint8_t current_target, uint8_t new_target);
/* Shift position/motor driver logic */
#ifdef DEBUG_ALL
#define DEBUG_MAIN
#define DEBUG_PERIPHERALS
#endif
#endif
cmake_minimum_required(VERSION 3.20)

project(tcase_controller C)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

set(TARGET main)

set(SRC_DIR ../src)
set(LD_SCRIPT ${CMAKE_BINARY_DIR}/STM32F446RETx_FLASH.ld)

set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=soft")

set(CMAKE_C_FLAGS "${CPU_FLAGS} -Wall -Wextra -O0 -g3 -ffreestanding -fdata-sections -ffunction-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CPU_FLAGS} -nostdlib -T ${LD_SCRIPT} -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET}.map -Wl,--gc-sections -Wl,-e,main")

file(WRITE ${LD_SCRIPT}
"ENTRY(main)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        *(.data*)
        . = ALIGN(4);
    } > RAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
    } > RAM
}
")

add_executable(${TARGET}.elf
    ${SRC_DIR}/main.c
)

target_include_directories(${TARGET}.elf PRIVATE
    ${SRC_DIR}
)

set_target_properties(${TARGET}.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_command(TARGET ${TARGET}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} ${CMAKE_BINARY_DIR}/${TARGET}.elf
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/${TARGET}.elf ${CMAKE_BINARY_DIR}/${TARGET}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${CMAKE_BINARY_DIR}/${TARGET}.elf ${CMAKE_BINARY_DIR}/${TARGET}.hex
)
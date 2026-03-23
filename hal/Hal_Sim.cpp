/*
 * Hal_Sim.cpp
 * Hardware Abstraction Layer — PC simulation backend
 *
 * All hardware functions return stub values.
 * Used when running tests on PC where no real hardware exists.
 * Replace with Hal_STM32.cpp when flashing to real hardware.
 */

#include "Hal.h"
#include <stdio.h>
#include <stdint.h>

void Hal_Init(void)
{
    printf("[Hal_Sim] Hardware simulation initialised\n");
}

uint32_t Hal_GetTickMs(void)
{
    /* Delegated to OsAl in sim — return 0 as stub */
    return 0U;
}

void Hal_GpioWrite(uint8_t pin, uint8_t value)
{
    (void)pin;
    (void)value;
}

uint8_t Hal_GpioRead(uint8_t pin)
{
    (void)pin;
    return 0U;
}

void Hal_UartWrite(uint8_t byte)
{
    (void)byte;
}

uint8_t Hal_UartRead(void)
{
    return 0U;
}

void Hal_WatchdogKick(void)
{
    /* No watchdog in sim */
}

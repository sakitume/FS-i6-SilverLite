/*
 * Copyright (c) 2013 - 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * This is template for main module created by New Kinetis SDK 2.x Project Wizard. Enjoy!
 **/

#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"

#include "timer.h"
#include "delay.h"
#include "lcd.h"
#include "sound.h"
#include "console.h"
#include "screen.h"
#include "debug.h"
#include "adc.h"
#include "backlight.h"
#include "buttons.h"
#include "drv_time.h"
#include "uart.h"
#include "flash.h"
#include "storage.h"

//------------------------------------------------------------------------------
//#define __USE_TRAINER_PORT_UART__
#define __USE_DEBUG_CONSOLE__

#if defined(__USE_TRAINER_PORT_UART__)
    #if defined(__USE_DEBUG_CONSOLE__)
        #error
    #endif
#endif

#if defined(__USE_DEBUG_CONSOLE__)    
    #if defined(__USE_TRAINER_PORT_UART__)
        #error
    #endif
    #include "fsl_debug_console.h"
#endif

//------------------------------------------------------------------------------
// Forward declarations
static void tests();

//------------------------------------------------------------------------------
void required_updates()
{
    // Call time_update() once at start of every loop
    time_update();

    // Query state of buttons and perform debounce
    buttons_update();

    // Retrieve ADC values for sticks and mult-position switches
    adc_update();

#if defined(__USE_TRAINER_PORT_UART__)
    uart_update();
#endif

    sound_update();

    // Check if lcd backlight should be turned on or off
    // If any of up/down/ok/cancel/bind buttons are active then light should be on;
    // if none of the buttons are active then set 10 second timer to turn off light
    static unsigned millisUntilBacklightOff = 10000;
    bool buttonActive = (button_active(kBtn_Up) || button_active(kBtn_Down) || button_active(kBtn_Ok)
        || button_active(kBtn_Cancel) || button_active(kBtn_Bind));
    if (buttonActive)
    {
        if (!millisUntilBacklightOff)
        {
            led_backlight_on();
        }
        millisUntilBacklightOff = millis_this_frame() + 10000;  // 10 seconds
    }
    else if (millisUntilBacklightOff)
    {
        if (millis_this_frame() >= millisUntilBacklightOff)
        {
            millisUntilBacklightOff = 0;
            led_backlight_off();
        }
    }
}

//------------------------------------------------------------------------------
int main(void)
{
    /* Init board hardware. */
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_SysTick();

#if defined(__USE_DEBUG_CONSOLE__)    
    BOARD_InitDebugConsole();
#endif    

    // Prevent A7105 module from being selected during SPI
    {
        // Set CS for the A7105 module to always be inactive (HIGH)
        gpio_pin_config_t pin_config_output_high = {
            kGPIO_DigitalOutput, 1
        };
        GPIO_PinInit(BOARD_INITPINS_SOFT_SPI_CS_A7105_GPIO, BOARD_INITPINS_SOFT_SPI_CS_A7105_PIN, &pin_config_output_high); // PORTC, 4
    }

#if defined(__USE_TRAINER_PORT_UART__)
    uart_init();
#endif
    timer_init();
    lcd_init();
    led_backlight_init();
    flash_init();
    storage_init();
    screen_init();
    console_init();
    debug_init();
    buttons_init();
    sound_init();
    int needStickCalibration = adc_init();
    if (needStickCalibration)
    {
        adc_calibrate_sticks();
    }

    //
    tests();
}

//------------------------------------------------------------------------------
static void tests()
{
    int testNumber = 0;
    while (1)
    {
        // Call the required update functions of various systems
        required_updates();

        // Advance to next test if down button was activated
        if (button_toggledActive(kBtn_Down))
        {
            testNumber++;
        }
        switch (testNumber)
        {
            case 0:
                buttons_test();
                break;

            case 1:
                adc_test();
                break;

            case 2:
                delay_test();
                break;

            case 3:
                millis_test();
                break;

            default:
                testNumber = 0;
                break;
        }
//        PRINTF("loop time: %d\n", micros_realtime() - micros_this_frame());
    }
}
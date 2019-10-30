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

#include "delay.h"
#include "lcd.h"
#include "console.h"
#include "screen.h"
#include "debug.h"
#include "adc.h"
#include "backlight.h"
#include "buttons.h"
#include "drv_time.h"

//------------------------------------------------------------------------------
//#define __USE_TRAINER_PORT_UART__
#define __USE_DEBUG_CONSOLE__

#if defined(__USE_TRAINER_PORT_UART__)
    #if defined(__USE_DEBUG_CONSOLE__)
        #error
    #endif
    #include "uart.h"
#endif

#if defined(__USE_DEBUG_CONSOLE__)    
    #if defined(__USE_TRAINER_PORT_UART__)
        #error
    #endif
    #include "fsl_debug_console.h"
#endif

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

    lcd_init();
    led_backlight_init();
    screen_init();
    console_init();
    debug_init();
    adc_init();
#if defined(__USE_TRAINER_PORT_UART__)
    uart_init();
#endif    
    buttons_init();

    while (1)
    {
        // Call time_update() once at start of every loop
        unsigned long totalMillis = time_update();
        buttons_update();
#if defined(__USE_TRAINER_PORT_UART__)
        uart_update();
#endif        

#if 0
    #if defined(__USE_TRAINER_PORT_UART__)
        uart_test();
    #endif
#endif

#if 0
        buttons_test();
#endif        

#if 0  
        extern void delay_test(void);
        delay_test();
#endif        
#if 0   
        extern void millis_test(void);
        millis_test();
#endif         

#if 0
        adc_test();
        adc_test2();
#endif

#if 1
        unsigned long us_now = micros_realtime();
        adc_test();
        unsigned long delta = micros_realtime() - us_now;
        console_clear();
        debug("ADC Update: ");
        debug_put_uint16(delta);
        debug_put_newline();
        debug_flush();
        PRINTF("ADC Update: %d\n", delta);
#endif

#if 0
        screen_test();        
#endif        
    }
}

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
#include "gui.h"

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
    led_backlight_update();
}

//------------------------------------------------------------------------------
#define __TEST_GEM__
#if defined(__TEST_GEM__)
#include "GEM.h"
static GLCD gGLCD;
static GEM gGEM(gGLCD);

// Create variables that will be editable through the menu and assign them initial values
int interval = 200;
boolean strobe = false;

// Create variable that will be editable through option select and create associated option select
byte tempo = 0;
SelectOptionByte selectTempoOptions[] = {{"Meh:(", 0}, {"Smooth", 1}, {"Hard", 2}, {"XTREME", 3}, {"Manual", 4}, {"Custom", 5}};
GEMSelect selectTempo(sizeof(selectTempoOptions)/sizeof(SelectOptionByte), selectTempoOptions);

// Values of interval variable associated with each select option
int tempoInterval[] = {400, 250, 120, 100, 0, 200};

// Create menu item objects of class GEMItem, linked to interval and strobe variables
// with validateInterval() callback function attached to interval menu item,
// that will make sure that interval variable is within allowable range (i.e. >= 0)
void validateInterval(); // Forward declaration
GEMItem menuItemInt("Interval:", interval, validateInterval);
GEMItem menuItemStrobe("Strobe:", strobe);

// Create menu item for option select with applyTempo() callback function
void applyTempo(); // Forward declaration
GEMItem menuItemTempo("Tempo:", tempo, selectTempo, applyTempo);

// Create menu button that will trigger rock() function. It will run animation sequence.
// We will write (define) this function later. However, we should
// forward-declare it in order to pass to GEMItem constructor
void rock(); // Forward declaration
GEMItem menuItemButton("Let's Rock!", rock);

// Create menu page object of class GEMPage. Menu page holds menu items (GEMItem) and represents menu level.
// Menu can have multiple menu pages (linked to each other) with multiple menu items each
GEMPage menuPageMain("Menu Title Goes Here");

// ---

// Validation routine of interval variable
void validateInterval() {
  // Check if interval variable is within allowable range (i.e. >= 0)
  if (interval < 0) {
    interval = 0;
  }
}

// Apply preset based on tempo variable value
void applyTempo() {
  if (tempo != 5) {
    // Set readonly mode for interval menu item
    menuItemInt.setReadonly();
    // Apply interval value based on preset selection
    interval = tempoInterval[tempo];
    // Turn on strobe effect for "XTREME" preset
    strobe = tempo == 3;
  } else {
    // Disable readonly mode of interval menu item for "Custom" preset
    menuItemInt.setReadonly(false);
  }
}

// Invoked once when the button is pressed
void rockContextEnter() {
    screen_fill(1);
}

// Invoked every loop iteration
void rockContextLoop() 
{
    if (button_toggledActive(kBtn_Cancel))
    {
        gGEM.context.exit();
        return;
    }

    // Do something here
}

// Invoked once when the GEM_KEY_CANCEL key is pressed
void rockContextExit() 
{
    // Draw menu back on screen and clear context
    gGEM.drawMenu();
    gGEM.clearContext();
}

// Setup context
void rock() 
{
    gGEM.context.loop = rockContextLoop;
    gGEM.context.enter = rockContextEnter;
    gGEM.context.exit = rockContextExit;
    gGEM.context.allowExit = false; // Setting to false will require manual exit from the loop
    gGEM.context.enter();
}

//------------------------------------------------------------------------------
static void gem_test()
{
    gGEM.init();

    // Add menu items to menu page
    menuPageMain.addMenuItem(menuItemTempo);
    menuPageMain.addMenuItem(menuItemInt);
    menuPageMain.addMenuItem(menuItemStrobe);
    menuPageMain.addMenuItem(menuItemButton);

    // Add menu page to menu and set it as current
    gGEM.setMenuPageCurrent(menuPageMain);

    gGEM.drawMenu();

    while (1)
    {
        if (button_toggledActive(kBtn_Bind))
        {
            return;
        }

        // Call the required update functions of various systems
        required_updates();
        if (gGEM.readyForKey())
        {
            uint8_t keyCode = GEM_KEY_NONE;
            if (button_toggledActive(kBtn_Cancel))
            {
                keyCode = GEM_KEY_CANCEL;
            }
            else if (button_toggledActive(kBtn_Ok))
            {
                keyCode = GEM_KEY_OK;
            }
            else if (button_toggledActive(kBtn_Up))
            {
                keyCode = GEM_KEY_UP;
            }
            else if (button_toggledActive(kBtn_Down))
            {
                keyCode = GEM_KEY_DOWN;
            }
            else if (button_toggledActive(kBtn_YawL))
            {
                keyCode = GEM_KEY_LEFT;
            }
            else if (button_toggledActive(kBtn_YawR))
            {
                keyCode = GEM_KEY_RIGHT;
            }
            gGEM.registerKeyPress(keyCode);
            screen_update();
        }
    }
}
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
    timer_init();   // Initializing this causes flash_write to reset the mcu!
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

    // Call buttons_update() once so we can use button_raw_state()
    buttons_update();
    if (button_raw_state(kBtn_ThrottleU))
    {
        tests();
    }

#if defined(__TEST_GEM__)
    gem_test();
#endif    

#if 1
    gui_init();
    gui_loop();
#endif    
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



#include "gui.h"
#include "debug.h"
#include "console.h"
#include "adc.h"
#include "delay.h"
#include "screen.h"
#include "assert.h"
#include "storage.h"
#include "sound.h"
#include "timer.h"
#include "buttons.h"
#include "backlight.h"
#include "drv_time.h"
#include "GEM.h"

static GLCD gGLCD;
GEM gGEM(gGLCD);

//------------------------------------------------------------------------------
volatile uint32_t gui_loop_100ms_counter;
static volatile uint8_t milli100 = 0xFF;

//------------------------------------------------------------------------------
// This is a millisecond timer callback that we use to manage the
// gui_loop_100ms_counter variable used by the original gui code
static void onEveryMillisecond(unsigned long millis)
{
    milli100++;
    if (milli100 >= 100)
    {
        milli100 = 0;
        gui_loop_100ms_counter++;
    }
}

//------------------------------------------------------------------------------
GEMPage menuPageMain("FS-i6 SilverLite");

extern void RunTXCtx();
GEMItem menuItem_RunTXCtx("Use Transmitter", RunTXCtx);
extern void RunSlidersCtx();
GEMItem menuItem_RunSlidersCtx("View Sliders", RunSlidersCtx);
extern void RunCalibrateCtx();
GEMItem menuItem_RunCalibrateCtx("Calibrate Sticks", RunCalibrateCtx);

extern void gui_init_models();
extern GEMPage menuPageModels;  // gui_models.cpp

GEMItem menuItem_Models("Models", menuPageModels);

//------------------------------------------------------------------------------
void gui_init() 
{
    if (milli100 == 0xFF)
    {
        milli100 = 0;
        timer_add_callback(onEveryMillisecond);
    }

    gGEM.init();
    gui_init_models();

    menuPageMain.addMenuItem(menuItem_Models);
    menuPageMain.addMenuItem(menuItem_RunTXCtx);
    menuPageMain.addMenuItem(menuItem_RunCalibrateCtx);
    menuPageMain.addMenuItem(menuItem_RunSlidersCtx);
    gGEM.setMenuPageCurrent(menuPageMain);
    
#if 1   // Launch TX context directly    
    RunTXCtx();
#else    
    gGEM.drawMenu();
#endif    
}

//------------------------------------------------------------------------------
void gui_loop()
{
    while (1)
    {
        // Call the required update functions of various systems
        extern void required_updates();
        required_updates();

        // Check for long press of up/down/ok/cancel buttons, if they've
        // been pressed for 3 seconds then we will exit this endless loop
        static uint32_t timeToReset;
        if (button_active(kBtn_Up) && button_active(kBtn_Down) && button_active(kBtn_Ok) && button_active(kBtn_Cancel))
        {
            if (!timeToReset)
            {
                timeToReset = millis_this_frame() + 3000;
            }
            else if (millis_this_frame() >= timeToReset)
            {
                timeToReset = 0;
                return;
            }
        }
        else
        {
            timeToReset = 0;
        }

        // Update the GEM system
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

//------------------------------------------------------------------------------
void gui_header_render(const char *str) {
    // border + header string
    screen_fill_rect(0, 0, LCD_WIDTH, 7, 1);
    screen_draw_round_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, 3, 1);

    screen_set_font(font_tomthumb3x5);
    screen_puts_centered(1 + font_tomthumb3x5[FONT_HEIGHT]/2, 0, str);
}


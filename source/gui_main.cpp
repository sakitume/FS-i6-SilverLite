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

#define __LOG_LOOPTIME__
#if defined(__LOG_LOOPTIME__)
#include "bayang_common.h"
#endif

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
GEMItem menuItem_RunTXCtx("Use Model", RunTXCtx);
extern void RunSlidersCtx();
GEMItem menuItem_RunSlidersCtx("View Sliders", RunSlidersCtx);
extern void RunCalibrateCtx();
GEMItem menuItem_RunCalibrateCtx("Calibrate Sticks", RunCalibrateCtx);

extern void gui_init_edit_model();
extern GEMPage menuPageEditModel;  // gui_models.cpp

GEMItem menuItem_Models("Edit current model", menuPageEditModel);

extern void gui_init_select_model();
extern GEMPage menuPageSelectModel;  // gui_select_model.cpp
GEMItem menuItem_SelectModel("Select model", menuPageSelectModel);


//------------------------------------------------------------------------------
void gui_init() 
{
    if (milli100 == 0xFF)
    {
        milli100 = 0;
        timer_add_callback(onEveryMillisecond);
    }

    gGEM.init();
    gui_init_edit_model();
    gui_init_select_model();
    menuPageMain.addMenuItem(menuItem_SelectModel);
    menuPageMain.addMenuItem(menuItem_Models);
    menuPageMain.addMenuItem(menuItem_RunTXCtx);
    menuPageMain.addMenuItem(menuItem_RunCalibrateCtx);
    menuPageMain.addMenuItem(menuItem_RunSlidersCtx);
    gGEM.setMenuPageCurrent(menuPageMain);

    menuPageMain.title = storage.model[storage.current_model].name;
    
#if 1   // Launch TX context directly    
    RunTXCtx();
#else    
    gGEM.drawMenu();
#endif    
}

#if defined(__LOG_LOOPTIME__)
//------------------------------------------------------------------------------
static void checkLoopTime()
{
    static uint32_t lastLoop;
    static uint32_t lastSec;
    uint32_t now = micros_this_frame();
    static uint32_t longest;
    if ((now - lastSec) > 1000000)
    {
        debug_put_uint16(longest);
        debug(", ");
        debug_put_uint16(gTXContext.irqTime);
        debug_put_newline();
        lastSec = now;
        longest = 0;
    }
    else
    {
        uint32_t delta = now - lastLoop;
        if (delta > longest)
        {
            longest = delta;
        }
    }
    lastLoop = now;
}
#endif

//------------------------------------------------------------------------------
void gui_loop()
{
    e_BtnIndex  autoRepeatButtonCode = kBtn_Cancel;
    uint8_t     autoRepeatKeyCode = GEM_KEY_NONE;
    uint32_t    autoRepeatTime = 0;
    while (1)
    {
        checkLoopTime();

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
            uint8_t     keyCode = GEM_KEY_NONE;
            e_BtnIndex  buttonCode;
            if (button_toggledActive(kBtn_Cancel))
            {
                keyCode = GEM_KEY_CANCEL;
                buttonCode = kBtn_Cancel;
            }
            else if (button_toggledActive(kBtn_Ok))
            {
                keyCode = GEM_KEY_OK;
                buttonCode = kBtn_Ok;
            }
            else if (button_toggledActive(kBtn_Up))
            {
                keyCode = GEM_KEY_UP;
                buttonCode = kBtn_Up;
            }
            else if (button_toggledActive(kBtn_Down))
            {
                keyCode = GEM_KEY_DOWN;
                buttonCode = kBtn_Down;
            }
            else if (button_toggledActive(kBtn_YawL))
            {
                keyCode = GEM_KEY_LEFT;
                buttonCode = kBtn_YawL;
            }
            else if (button_toggledActive(kBtn_YawR))
            {
                keyCode = GEM_KEY_RIGHT;
                buttonCode = kBtn_YawR;
            }

            // If up/down/left/right button was pushed
            if ((keyCode >= GEM_KEY_UP) && (keyCode <= GEM_KEY_LEFT))
            {
                autoRepeatKeyCode = keyCode;
                autoRepeatButtonCode = buttonCode;
                autoRepeatTime = millis_this_frame() + 500;
            }
            else if (keyCode == GEM_KEY_NONE)
            {
                // No button press, check if autorepeat should be performed
                if (autoRepeatKeyCode != GEM_KEY_NONE)
                {
                    if (!button_active(autoRepeatButtonCode))
                    {
                        autoRepeatKeyCode = GEM_KEY_NONE;
                    }
                    else if (millis_this_frame() >= autoRepeatTime)
                    {
                        keyCode = autoRepeatKeyCode;
                        autoRepeatTime = millis_this_frame() + 200;
                    }
                }
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


#include "fsl_debug_console.h"

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

//------------------------------------------------------------------------------
extern GEM gGEM;

//------------------------------------------------------------------------------
static void gui_config_stick_calibration_store_adc_values() 
{
    for (int i=0; i<4; i++)
    {
        uint16_t val = adc_get_channel_raw(i);
        uint16_t *minMidMax = storage.chanMinMidMax[i];
        if (i == 2) // throttle
        {
            minMidMax[1] = 2048;
        }
        else
        {
            minMidMax[1] = val;
        }
    }
}

//------------------------------------------------------------------------------
static void gui_config_stick_calibration_render() {
    uint32_t i;
    uint32_t a;

    const uint8_t *font = font_tomthumb3x5;
    uint32_t h = font[FONT_HEIGHT] + 1;
    uint32_t w = font[FONT_FIXED_WIDTH] + 1;

    // store adc values
    gui_config_stick_calibration_store_adc_values();

    // draw ui
    gui_header_render("STICK CALIBRATION");

    uint32_t y = 8;
    //                       |                             |
    screen_puts_xy(3, y, 1, "Please move all sticks to the"); y += h;
    screen_puts_xy(3, y, 1, "extreme positions."); y += h;
    screen_puts_xy(3, y, 1, "When done, move all sticks to"); y += h;
    screen_puts_xy(3, y, 1, "the center and press OK."); y += h;

    uint32_t x = 3;
    y = 33;

    screen_puts_xy(x+1*4*w+w, y, 1,       "min");
    screen_puts_xy(x+2*4*w+1*2*w+w, y, 1, "now");
    screen_puts_xy(x+3*4*w+2*2*w+w, y, 1, "max");
    y += h;

    for (i = 0; i < 4; i++) {
        screen_puts_xy(x, y, 1, adc_get_channel_name(i, false));
        for (a = 0; a < 3; a++) {
            screen_put_uint14(x+(a+1)*4*w+a*2*w, y, 1, storage.chanMinMidMax[i][a]);
        }
        y += h;
    }
}

//------------------------------------------------------------------------------
static void calibrationCtxEnter() 
{
    storage_take_snapshot();    // so we can discard our changes by restoring the snapshot

    // Set min/max to default values (smaller than the full range)
    // The calibration page will
    for (int chan=0; chan<4; chan++)
    {
        uint16_t *minMidMax = storage.chanMinMidMax[chan];
        minMidMax[0] = 400;
        minMidMax[2] = 4096 - 400;
    }

    screen_fill(0);
}

//------------------------------------------------------------------------------
static void calibrationCtxLoop() 
{
    led_backlight_tickle();
    if (button_toggledActive(kBtn_Cancel))
    {
        storage_restore_snapshot();
        gGEM.context.exit();
        return;
    }
    if (button_toggledActive(kBtn_Ok))
    {
        // Sticks should currently be centered, update storage with these "mid" values
        for (int i=0; i<4; i++)
        {
            uint16_t val = adc_get_channel_raw(i);
            uint16_t *minMidMax = storage.chanMinMidMax[i];
            if (i == 2) // throttle
            {
                minMidMax[1] = 2048;
            }
            else
            {
                minMidMax[1] = val;
            }
        }
        storage_save();
        gGEM.context.exit();
        return;
    }

//    screen_fill(0);
    gui_config_stick_calibration_render();
    screen_update();
}

//------------------------------------------------------------------------------
static void calibrationCtxExit() 
{
    gGEM.drawMenu();
    gGEM.clearContext();
}

//------------------------------------------------------------------------------
// Setup context
void RunCalibrateCtx() 
{
    gGEM.context.loop   = calibrationCtxLoop;
    gGEM.context.enter  = calibrationCtxEnter;
    gGEM.context.exit   = calibrationCtxExit;
    gGEM.context.allowExit = false; // Setting to false will require manual exit from the loop
    gGEM.context.enter();
}

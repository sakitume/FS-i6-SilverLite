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

extern GEM gGEM;

//------------------------------------------------------------------------------
static void gui_render_sliders() {
    uint32_t i;
    uint32_t y;

    led_backlight_tickle();

    // draw ui
    gui_header_render("SLIDER VIEW");

    screen_set_font(font_tomthumb3x5);

    for (i = 0; i < 8; i++) {
        // render channel names
        y = 10 + i*(font_tomthumb3x5[FONT_HEIGHT]+1);
        screen_puts_xy(1, y, 1, adc_get_channel_name(i, true));

        // render sliders
        uint32_t y2 = y + (font_tomthumb3x5[FONT_HEIGHT]+1)/2;
        screen_draw_hline(8, y2 - 1, 50-1, 1);
        screen_draw_hline(8, y2 + 1, 50-1, 1);
        screen_draw_hline(8 + 50 + 1, y2 - 1, 50-1, 1);
        screen_draw_hline(8 + 50 + 1, y2 + 1, 50-1, 1);

        // From 0..1023 (inclusive)
        // To -100..+100
        int32_t cal = (int32_t)adc_get_channel_calibrated(i);
        int val = ((cal * 200) / 1023) - 100;

        // render val as text
        screen_put_int8(8 + 100 + 2, y, 1, val);

        // rescale from +/-100 to 0..100
        val = 50 + val/2;
        screen_draw_vline(8 + val - 1, y+1, 5, 1);
        screen_draw_vline(8 + val    , y+1, 5, 1);
    }
}

//------------------------------------------------------------------------------
static void slidersCtxEnter() 
{
    screen_fill(0);
}

//------------------------------------------------------------------------------
static void slidersCtxLoop() 
{
    if (button_toggledActive(kBtn_Cancel))
    {
        gGEM.context.exit();
        return;
    }

    screen_fill(0);
    gui_render_sliders();
    screen_update();
}

//------------------------------------------------------------------------------
static void slidersCtxExit() 
{
    gGEM.drawMenu();
    gGEM.clearContext();
}

//------------------------------------------------------------------------------
// Setup context
void RunSlidersCtx() 
{
    gGEM.context.loop   = slidersCtxLoop;
    gGEM.context.enter  = slidersCtxEnter;
    gGEM.context.exit   = slidersCtxExit;
    gGEM.context.allowExit = false; // Setting to false will require manual exit from the loop
    gGEM.context.enter();
}

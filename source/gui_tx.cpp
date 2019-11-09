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
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) > (b) ? (a) : (b))

//------------------------------------------------------------------------------
// Used: https://javl.github.io/image2cpp/
const unsigned char sprite_alarm_enabled[] =
{
    16, 16,
    // 'alarm', 16x16px
    0xff, 0x1f, 0xdf, 0x1f, 0xef, 0xef, 0xf7, 0x07, 0xff, 0xef, 0x9b, 0xf5, 0x09, 0xf7, 0x0f, 0xff, 
    0xff, 0xf8, 0xfb, 0xf8, 0xf7, 0xf7, 0xef, 0xe0, 0xff, 0xf7, 0xd9, 0xaf, 0x90, 0xef, 0xf0, 0xff
};
const unsigned char sprite_alarm_disabled[] =
{
    16, 16,
    // 'alarm-off', 16x16px
    0xff, 0x1f, 0xdf, 0x1f, 0xef, 0xef, 0xf7, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xf8, 0xfb, 0xf8, 0xf7, 0xf7, 0xef, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

//------------------------------------------------------------------------------
#define GUI_STATUSBAR_FONT font_tomthumb3x5

//------------------------------------------------------------------------------
static int16_t modelTimer;
static uint8_t alarmBeepsDisabled;

//------------------------------------------------------------------------------
static void ReloadModelTimer() 
{
    modelTimer = (int16_t) storage.model[storage.current_model].timer;
}

//------------------------------------------------------------------------------
int gui_handle_buttons() {
    // TODO, check and respond to button presses here
    enum { kTimerID_BindButton = 1 };
    static bool bindActive;
    if (button_toggledActive(kBtn_Bind))
    {
        timer_set_timeout(kTimerID_BindButton, 3000);
        bindActive = true;
    }
    else if (bindActive)
    {
        if (0 == timer_get_timeout(kTimerID_BindButton))
        {
            bindActive = false;
            // TODO: Re-init TX here
            PRINTF("Bind button held down\n");
        }
    }

    // If user presses OK button
    {
        static uint32_t lastActivated;
        if (button_toggled(kBtn_Ok))
        {
            if (button_active(kBtn_Ok))
            {
                lastActivated = millis_this_frame();
            }
            else if (lastActivated)
            {
                uint32_t delta = millis_this_frame() - lastActivated;
                lastActivated = 0;

                // If it was a short button press
                if (delta < 800)
                {
                    // toggle alarm beep enabled switch
                    alarmBeepsDisabled = !alarmBeepsDisabled;
                }
            }
        }
        else if (lastActivated)
        {
            // If held down for 2.5 seconds
            uint32_t delta = millis_this_frame() - lastActivated;
            if (delta >= 2500)
            {
                lastActivated = 0;
                sound_play_bind();
                ReloadModelTimer();
            }
        }
    }

    // If user presses Cancel button
    {
        static uint32_t lastActivated;
        if (button_toggled(kBtn_Cancel))
        {
            if (button_active(kBtn_Cancel))
            {
                lastActivated = millis_this_frame();
            }
        }
        else if (lastActivated)
        {
            // If held down for 3 seconds
            uint32_t delta = millis_this_frame() - lastActivated;
            if (delta >= 3000)
            {
                lastActivated = 0;
                return 0;
            }
        }
    }
    return 1;
}

//------------------------------------------------------------------------------
static void gui_process_logic() {
    uint32_t second_elapsed = 0;

    enum { kTimerID_SecondElapsed = 0 };
    if (0 == timer_get_timeout(kTimerID_SecondElapsed)) {
        // one second has passed
        second_elapsed = 1;
        // next timeout in 1s
        timer_set_timeout(kTimerID_SecondElapsed, 1000);
    }

    // count down when throttle is past zero
    enum { kThrottleThreshold = 40 };   // TODO
    if (adc_get_channel_calibrated(ADC_ID_THROTTLE) >= kThrottleThreshold) {
        // do timer logic, handle countdown
        if (second_elapsed) {
            modelTimer--;
        }
    }
}

//------------------------------------------------------------------------------
static void gui_render_battery() {
    uint32_t x = 84;
    screen_set_font(GUI_STATUSBAR_FONT);

    // screen_fill_rect(x-1, 0, LCD_WIDTH-x+1, 7, 1);

    // fetch battery voltage
    uint16_t v_bat = adc_get_battery_voltage();

    // show voltage
    screen_put_fixed2(x, 1, 0, v_bat);
    x += (GUI_STATUSBAR_FONT[FONT_FIXED_WIDTH]+1)*4;
    screen_puts_xy(x, 1, 0, "V");
    x += GUI_STATUSBAR_FONT[FONT_FIXED_WIDTH]+1;

    // render battery symbol
    x += 2;
    // draw border
    screen_draw_round_rect(x, 1, 21, 5, 2, 0);
    screen_draw_vline(x+20, 1, 5, 0);

    // show fillgrade
    // assume nimh batteries with 1.2V > 90% / 1.0V = 5%
    //                         = 4.8V       / 4.0V
    // i know this is not 100% correct, better calc is tbd ;)
    int32_t fill_percent = ((17 * v_bat)/ 16) - 420;
    fill_percent = max(min(fill_percent, 100), 5);

    // 0% = 0px, 100% = 20px
    int32_t fill_px = max(0, min(20, fill_percent / 5));
    // draw fill grade
    screen_fill_rect(x+1, 1+1, fill_px, 3, 0);
}

//------------------------------------------------------------------------------
static void gui_render_rssi(uint8_t rssi_rx, uint8_t rssi_tx) {
    #define GUI_RSSI_BAR_W 25
    uint16_t x = 1;
    // render rx rssi bargraph at a given position
    screen_fill_rect(x, 1, GUI_RSSI_BAR_W+1, 5, 0);
    x+=GUI_RSSI_BAR_W+2;
    // show values
    screen_puts_xy(x, 1, 0, "120|105");

    // show RSSI
    uint8_t rssi = 25; // XXX, TODO
    uint8_t rssi_telemetry = 75; // XXX, TODO

    screen_put_uint8(x, 1, 0, rssi_telemetry);
    x += (GUI_STATUSBAR_FONT[FONT_FIXED_WIDTH]+1) * 3;
    screen_puts_xy(x, 1, 0, "|");
    x += (GUI_STATUSBAR_FONT[FONT_FIXED_WIDTH]+1) * 1;
    screen_put_uint8(x, 1, 0, rssi);
    x += (GUI_STATUSBAR_FONT[FONT_FIXED_WIDTH]+1) * 3;

    // render tx rssi bargraph at a given position
    screen_fill_rect(x, 1, GUI_RSSI_BAR_W+1, 5, 0);

    // fill bargraphs
    // rssi can be 0..100 (?)
    uint8_t bar_w = min(rssi_telemetry, 100)/4;
    if (bar_w > 1) bar_w--;
    if (bar_w > 0) screen_fill_rect(1+bar_w, 2, 25-bar_w, 3, 1);
    bar_w = min(rssi, 100)/4;
    if (bar_w > 1) bar_w--;
    if (bar_w > 0) screen_fill_rect(x+bar_w, 2, 25-bar_w, 3, 1);
}

//------------------------------------------------------------------------------
static void gui_render_statusbar() {
    // render rx/tx rssi and battery status:
    // draw divider
    screen_fill_rect(0, 0, LCD_WIDTH, 7, 1);

    // draw battery voltage
    gui_render_battery();

    gui_render_rssi(111, 120);

    if (alarmBeepsDisabled)
    {
        // LCD_WIDTH-16
        screen_put_sprite(0, 0, sprite_alarm_disabled, 0);
    }
    else
    {
        screen_put_sprite(0, 0, sprite_alarm_enabled, 0);
    }
}

//------------------------------------------------------------------------------
static void gui_render_bottombar() {
    // render modelname at bottom
    // draw black border
    screen_fill_rect(0, LCD_HEIGHT - 7, LCD_WIDTH, 7, 1);

    screen_set_font(font_tomthumb3x5);
    screen_puts_centered(LCD_HEIGHT - 6 + font_tomthumb3x5[FONT_HEIGHT]/2, 0,
                         storage.model[storage.current_model].name);
}

//------------------------------------------------------------------------------
static void txCtxRender() {
    uint32_t x;
    uint32_t y;

    // do statusbars
    gui_render_statusbar();
    gui_render_bottombar();

    // render voltage
    screen_set_font(font_metric7x12);
    x = 1;
    y = 10;
    uint32_t voltage = 123; // TODO, XXX telemetry_get_voltage()
    screen_put_fixed2_1digit(x, y, 1, voltage);
    x += (font_metric7x12[FONT_FIXED_WIDTH]+1)*3 + 3;
    screen_puts_xy(x, y, 1, "V");

    x = 1;
    y += font_metric7x12[FONT_HEIGHT]+1;
    uint32_t current = 123; // TODO, XXX telemetry_get_current()
    screen_put_fixed2_1digit(x, y, 1, current);
    x += (font_metric7x12[FONT_FIXED_WIDTH]+1)*3 + 3;
    screen_puts_xy(x, y, 1, "A");

    x = LCD_WIDTH - (font_metric7x12[FONT_FIXED_WIDTH]+1)*7 - 1;
    y += font_metric7x12[FONT_HEIGHT]+1;
    y += 5;
    uint16_t MAH = 123; // TODO, XXX telemetry_get_mah()
    screen_put_uint14(x, y, 1, MAH);
    x += (font_metric7x12[FONT_FIXED_WIDTH]+1)*4 + 1;
    screen_puts_xy(x, y, 1, "MAH");

    // screen_set_font(font_metric7x12);
    screen_set_font(font_metric15x26);

    // render time
    uint32_t color = 1;
    if (modelTimer < 0) {
        if ((gui_loop_100ms_counter % 4) == 0) {
            color = 1 - color;
        }
    }
    if ((modelTimer >= 0) && (modelTimer < 15)) {
        if ((gui_loop_100ms_counter % 10) == 0) {
            // beep!
            // TODO: Make this an option that can be easily toggled
            sound_play_low_time();
            led_backlight_tickle();
        }
    }

    // render background
    x = 51;
    y = 10;
    uint32_t w = (font_metric15x26[FONT_FIXED_WIDTH]/2 + 2) +
                 (font_metric15x26[FONT_FIXED_WIDTH] + 1) * 4 + 3;
    uint32_t h = font_metric15x26[FONT_HEIGHT] + 2;
    screen_fill_round_rect(x, y, w, h, 2, 1 - color);
    x++;
    y++;

    // render time
    screen_put_time(x, y, color, modelTimer);
}

//------------------------------------------------------------------------------
static void txCtxEnter() 
{
    // re init model timer
    ReloadModelTimer();
}

//------------------------------------------------------------------------------
static void txCtxLoop() 
{
    gui_process_logic();
    if (!gui_handle_buttons())
    {
        gGEM.context.exit();
        return;
    }

    screen_fill(0);
    txCtxRender();
    screen_update();
}

//------------------------------------------------------------------------------
static void txCtxExit() 
{
    gGEM.drawMenu();
    gGEM.clearContext();
}

//------------------------------------------------------------------------------
// Setup context
void RunTXCtx() 
{
    gGEM.context.loop   = txCtxLoop;
    gGEM.context.enter  = txCtxEnter;
    gGEM.context.exit   = txCtxExit;
    gGEM.context.allowExit = false; // Setting to false will require manual exit from the loop
    gGEM.context.enter();
}

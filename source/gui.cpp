/*
    Copyright 2016 fishpepper <AT> gmail.com

    This program is free software: you can redistribute it and/ or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http:// www.gnu.org/licenses/>.

    author: fishpepper <AT> gmail.com
*/

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

//------------------------------------------------------------------------------
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) > (b) ? (a) : (b))


typedef struct {
    uint8_t event_id;
    uint16_t x;
    uint16_t y;
} touch_event_t;
touch_event_t touch_get_last_event(void);
#define TOUCH_GESTURE_MOUSE_DOWN 1

touch_event_t touch_get_last_event(void)
{
    // TODO
    touch_event_t event;
    event.event_id = 0;
    return event;
}

//------------------------------------------------------------------------------










#define GUI_PREV_CLICK_X 10
#define GUI_NEXT_CLICK_X (LCD_WIDTH - GUI_PREV_CLICK_X)
#define GUI_PAGE_MAIN     0
#define GUI_PAGE_STICKS   1
#define GUI_PAGE_SETTINGS 2
#define GUI_MAX_PAGE GUI_PAGE_SETTINGS
#define GUI_STATUSBAR_FONT font_tomthumb3x5


#define GUI_LOOP_DELAY_MS 100
#define GUI_SHUTDOWN_PRESS_S 2.0
#define GUI_SHUTDOWN_PRESS_COUNT_FROM_MS(_ms) ((_ms)/GUI_LOOP_DELAY_MS)
#define GUI_SHUTDOWN_PRESS_COUNT (GUI_SHUTDOWN_PRESS_COUNT_FROM_MS(1000*GUI_SHUTDOWN_PRESS_S))

// touch function pointer
typedef void (*f_ptr_t)(void);
typedef void (*f_ptr_32_32_t)(uint32_t x, uint32_t y);

// touch callback
typedef struct {
    // defines the touch sensitive rect
    uint8_t xs;
    uint8_t xe;
    uint8_t ys;
    uint8_t ye;
    // and the callback to run
    f_ptr_t callback;
} touch_callback_entry_t;

#define GUI_TOUCH_CALLBACK_COUNT 10
static void gui_touch_callback_register(uint8_t xs, uint8_t xe, uint8_t ys, uint8_t ye, f_ptr_t cb);
static void gui_touch_callback_clear(void);


#define GUI_PAGE_CONFIG_FLAG        0x80
#define GUI_PAGE_CONFIG_OPTION_FLAG 0x40
#define GUI_PAGE_SETUP_FLAG         0x20
#define GUI_PAGE_NOFLAGS (~(GUI_PAGE_CONFIG_FLAG|GUI_PAGE_CONFIG_OPTION_FLAG|GUI_PAGE_SETUP_FLAG))


#define GUI_PAGE_SETUP_MAIN       (GUI_PAGE_SETUP_FLAG | 0)
#define GUI_PAGE_SETUP_BIND       (GUI_PAGE_SETUP_FLAG | 1)
#define GUI_PAGE_SETUP_BOOTLOADER (GUI_PAGE_SETUP_FLAG | 2)

#define GUI_PAGE_CONFIG_MAIN            (GUI_PAGE_CONFIG_FLAG | 0)
#define GUI_PAGE_CONFIG_STICK_CAL       (GUI_PAGE_CONFIG_FLAG | 1)
#define GUI_PAGE_CONFIG_MODEL_SETTINGS  (GUI_PAGE_CONFIG_FLAG | 2)


#define GUI_SUBPAGE_SETTING_MODEL_NAME  0
#define GUI_SUBPAGE_SETTING_MODEL_SCALE 1
#define GUI_SUBPAGE_SETTING_MODEL_TIMER 2

void gui_init(void);
void gui_loop(void);

uint32_t gui_running(void);

static void gui_process_touch(void);
static void gui_render_main_screen(void);
static void gui_process_logic(void);


static void gui_config_render(void);
static void gui_config_stick_calibration_store_adc_values(void);
static void gui_config_stick_calibration_render(void);

static void gui_setup_render(void);
static void gui_setup_main_render(void);


static void gui_render(void);
static void gui_render_sliders(void);
static void gui_render_battery(void);
static void gui_render_statusbar(void);
static void gui_render_bottombar(void);
static void gui_render_settings(void);

static void gui_touch_callback_execute(uint8_t i);
static void gui_add_button(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const char *str, f_ptr_t cb);
static void gui_add_button_smallfont(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                                     const char *str, f_ptr_t cb);
static void gui_cb_model_timer_reload(void);
static void gui_cb_model_prev(void);
static void gui_cb_model_next(void);
static void gui_cb_setting_model_stickscale(void);
static void gui_cb_setting_model_name(void);
static void gui_cb_setting_model_timer(void);
static void gui_cb_setting_option_leave(void);
static void gui_cb_previous_page(void);
static void gui_cb_next_page(void);
static void gui_cb_config_back(void);
static void gui_cb_config_save(void);
static void gui_cb_config_stick_save(void);

static void gui_cb_config_stick_cal(void);
static void gui_cb_config_model(void);
static void gui_cb_config_exit(void);

static void gui_cb_setup_bootloader(void);
static void gui_cb_setup_exit(void);

static void gui_config_main_render(void);
static void gui_config_model_render(void);

static void gui_setup_bindmode_render(void);
static void gui_setup_bootloader_render(void);











//------------------------------------------------------------------------------
static uint32_t gui_config_counter;
static uint32_t gui_shutdown_pressed;
static uint8_t gui_active = 0;
static uint8_t gui_page;
static uint8_t gui_sub_page;
static uint8_t gui_config_tap_detected;
static uint8_t gui_touch_callback_index;
static touch_callback_entry_t gui_touch_callback[GUI_TOUCH_CALLBACK_COUNT];
static int16_t gui_model_timer;

static uint8_t gui_button_cursor_enabled;
static uint8_t gui_button_cursor;

static volatile uint32_t gui_loop_100ms_counter;
static volatile uint8_t milli100;
static void onEveryMillisecond(unsigned long millis)
{
    milli100++;
    if (milli100 >= 100)
    {
        milli100 = 0;
        gui_loop_100ms_counter++;
    }
}

void gui_init(void) {
    debug("gui: init\n"); debug_flush();
    gui_page     = GUI_PAGE_MAIN;
    gui_sub_page = 0;
    gui_shutdown_pressed = 0;
    gui_config_tap_detected = 0;
    gui_touch_callback_index = 0;

    gui_touch_callback_clear();

    timer_add_callback(onEveryMillisecond);
}

static void gui_touch_callback_clear(void) {
    uint32_t i;
    // debug("gui: touch callback clear\n");
    // debug_flush();

    // clear all touch callbacks
    for (i = 0; i < GUI_TOUCH_CALLBACK_COUNT; i++) {
        gui_touch_callback[i].callback = 0;
    }
    gui_touch_callback_index = 0;
}

static void gui_touch_callback_register(uint8_t xs, uint8_t xe, uint8_t ys, uint8_t ye,
                                        f_ptr_t cb) {
    // debug("gui: touch cb register\n");

    // check if we have space to register that slot:
    if (gui_touch_callback_index >= GUI_TOUCH_CALLBACK_COUNT) {
        // debug("gui: ERROR: no free slot\n");
        // debug_flush();
        return;
    }

    // make sure to have valid ranges by ignoring bad ranges
    if (xs > xe) { xs = xe; }
    if (ys > ye) { ys = ye; }

    // fine, configure this slot:
    gui_touch_callback[gui_touch_callback_index].xs       = min(xs, LCD_WIDTH);
    gui_touch_callback[gui_touch_callback_index].xe       = min(xe, LCD_WIDTH);
    gui_touch_callback[gui_touch_callback_index].ys       = min(ys, LCD_HEIGHT);
    gui_touch_callback[gui_touch_callback_index].ye       = min(ye, LCD_HEIGHT);
    gui_touch_callback[gui_touch_callback_index].callback = cb;
    gui_touch_callback_index++;
}

uint32_t gui_running(void) {
    return gui_active;
}


static void gui_touch_callback_execute(uint8_t i) {
    assert(i < gui_touch_callback_index);

    /*debug("gui: exec cb 0x");
    debug_put_hex32((uint32_t) gui_touch_callback[i].callback);
    debug_put_newline();
    debug_flush();*/

    if (gui_touch_callback[i].callback != 0) {
        // execute this callback if valid
        gui_touch_callback[i].callback();
    }
}

static void gui_add_button_smallfont(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                                     const char *str, f_ptr_t cb) {
    screen_set_font(font_tomthumb3x5);
    gui_add_button(x, y, w, h, str, cb);
}

static void gui_add_button(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const char *str, f_ptr_t cb) {

    // If button cursor is enabled and the button being added is the one cursor is on
    uint8_t color = 1;
    bool hilite = false;
    if (gui_button_cursor_enabled && (gui_button_cursor == gui_touch_callback_index))
    {
        color ^= 1;
        hilite = true;
    }

    // render string
    screen_puts_xy_centered(x + w/2, y + h/2, 1, str);

    // render a rounded rect
    screen_draw_round_rect(x, y, w, h, 3, 1);
    if (hilite)
    {
        screen_draw_round_rect(x-1, y-1, w+2, h+2, 3, 1);
    }

    // register the callback
    gui_touch_callback_register(x, x + w, y, y + h, cb);
}

static void gui_process_touch(void) {
    uint32_t i;

    // fetch pending touch event:
    touch_event_t t = touch_get_last_event();

    if (t.event_id == TOUCH_GESTURE_MOUSE_DOWN) {
        // there was a mouse click!
        // check if we will have to execute a callback
        for (i = 0; i < gui_touch_callback_index; i++) {
            // the first one matching will be triggered first.
            // anyway we also allow multiple triggers
            if (gui_touch_callback[i].callback != 0) {
                // check if click was inside this region
                if ((t.x >= gui_touch_callback[i].xs) && (t.x <= gui_touch_callback[i].xe) &&
                    (t.y >= gui_touch_callback[i].ys) && (t.y <= gui_touch_callback[i].ye) ) {
                        // play sound
                        sound_play_click();

                        // reset sub pages
                        gui_config_counter = 0;

                        // execute callback!
                        gui_touch_callback_execute(i);
                }
            }
        }
    }
}

static void gui_cb_model_timer_reload(void) {
    gui_model_timer = (int16_t) storage.model[storage.current_model].timer;
}

static void gui_cb_model_prev(void) {
    if (storage.current_model > 0) {
        storage.current_model--;
    }
}

static void gui_cb_model_next(void) {
    if (storage.current_model < (STORAGE_MODEL_MAX_COUNT-1)) {
        storage.current_model++;
    }
}

static void gui_cb_setting_model_stickscale(void) {
    gui_page    |= GUI_PAGE_CONFIG_OPTION_FLAG;
    gui_sub_page = GUI_SUBPAGE_SETTING_MODEL_SCALE;
}

static void gui_cb_setting_model_name(void) {
    gui_page    |= GUI_PAGE_CONFIG_OPTION_FLAG;
    gui_sub_page = GUI_SUBPAGE_SETTING_MODEL_NAME;
}

static void gui_cb_setting_model_timer(void) {
    gui_page    |= GUI_PAGE_CONFIG_OPTION_FLAG;
    gui_sub_page = GUI_SUBPAGE_SETTING_MODEL_TIMER;
}

static void gui_cb_setting_option_leave(void) {
    gui_page &= ~GUI_PAGE_CONFIG_OPTION_FLAG;
}

static void gui_cb_model_stickscale_dec(void) {
    // TODO
}

static void gui_cb_model_stickscale_inc(void) {
    // TODO
}

static void gui_cb_model_timer_dec(void) {
    if (storage.model[storage.current_model].timer > 2) {
        storage.model[storage.current_model].timer--;
    }
}

static void gui_cb_model_timer_inc(void) {
    if (storage.model[storage.current_model].timer < 99*60) {
        storage.model[storage.current_model].timer++;
    }
}

static void gui_cb_previous_page(void) {
    if (gui_page > 0) {
        gui_page--;
    }
}

static void gui_cb_next_page(void) {
    if (gui_page < GUI_MAX_PAGE) {
        gui_page++;
    }
}

static void gui_cb_config_back(void) {
    gui_page = GUI_PAGE_CONFIG_MAIN;
}

static void gui_cb_config_stick_save()
{
    // Sticks should be centered, update storage with these "mid" values
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
    gui_cb_config_save();
}
static void gui_cb_config_save(void) 
{
    storage_save();
    gui_cb_config_back();
}

static void gui_cb_config_stick_cal(void) 
{
    // Set min/max to default values (smaller than the full range)
    // The calibration page will
    for (int chan=0; chan<4; chan++)
    {
        uint16_t *minMidMax = storage.chanMinMidMax[chan];
        minMidMax[0] = 400;
        minMidMax[2] = 4096 - 400;
    }


    gui_page = GUI_PAGE_CONFIG_STICK_CAL;
}

static void gui_cb_config_model(void) {
    gui_page = GUI_PAGE_CONFIG_MODEL_SETTINGS;
}

static void gui_cb_setup_bind(void) {
    gui_page = GUI_PAGE_SETUP_BIND;
}

static void gui_cb_setup_bootloader(void) {
    // disable tx code
    //frsky_tx_set_enabled(0);
    // TODO, XXX: Stop TX protocol
    gui_page = GUI_PAGE_SETUP_BOOTLOADER;
}

static void gui_cb_config_enter(void) {
    gui_page = GUI_PAGE_CONFIG_MAIN;
}

static void gui_cb_setup_exit(void) {
    // back to setup main menu
    gui_page = GUI_PAGE_SETTINGS;
}


static void gui_cb_config_exit(void) {
    // restore old settings
    // TODO, XXX: storage_load();
    storage_init();

    // back to config main menu
    gui_page = GUI_PAGE_CONFIG_MAIN;
}

static void gui_cb_setup_enter(void) {
    gui_page = GUI_PAGE_SETUP_MAIN;
}

void gui_handle_buttons(void) {
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

    // UI Button cursor handling
    {
        if (button_toggledActive(kBtn_Ok))
        {
            if (gui_button_cursor_enabled)
            {
                // UI Button has been activated
                gui_button_cursor_enabled = false;
                gui_touch_callback_execute(gui_button_cursor);
            }
            else
            {
                // If this gui page has buttons
                if  (gui_touch_callback_index > 0)
                {
                    gui_button_cursor_enabled = true;
                    gui_button_cursor = 0;
                }
            }
        }
        else if (button_toggledActive(kBtn_Cancel))
        {
            if  (gui_button_cursor_enabled)
            {
                gui_button_cursor_enabled = false;
            }
        }


        if (gui_button_cursor_enabled)
        {
            if (button_toggledActive(kBtn_Up))
            {
                if (gui_button_cursor)
                {
                    gui_button_cursor--;
                }
                else if (gui_touch_callback_index)
                {
                    gui_button_cursor = gui_touch_callback_index - 1;
                }
            }
            else if (button_toggledActive(kBtn_Down))
            {
                if ((gui_button_cursor + 1) < gui_touch_callback_index)
                {
                    gui_button_cursor++;
                }
                else
                {
                    gui_button_cursor = 0;
                }
            }
        }
    }
    switch (gui_page) 
    {
        case (GUI_PAGE_SETTINGS):
        case (GUI_PAGE_STICKS):
        case (GUI_PAGE_MAIN):
        {
            if (!gui_button_cursor_enabled)
            {
                if (button_toggledActive(kBtn_Up))
                {
                    gui_cb_previous_page();
                }
                else if (button_toggledActive(kBtn_Down))
                {
                    gui_cb_next_page();
                }
            }
        }
        break;

        default:
            break;
    }

}

static void gui_process_logic(void) {
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
            gui_model_timer--;
        }
    }
}



void gui_loop(void) {
    uint32_t gui_startup_counter = 0;

    debug("gui: entering main loop\n"); debug_flush();
    gui_active = 1;

    // start with main page
    gui_page = GUI_PAGE_MAIN;
    gui_loop_100ms_counter = 0;

    // re init model timer
    gui_cb_model_timer_reload();

    // this is the main GUI loop. rf stuff is done inside an ISR
    while (1)
    {
        extern void required_updates();
        required_updates();

        // handle buttons
        gui_handle_buttons();

        // handle touch gestures
        gui_process_touch();

        // clear old touch callbacks as the page rendering
        // will (re-)register callbacks
        gui_touch_callback_clear();

        // do some ui logic, like counting down timers,
        // doing warning beeps etc
        gui_process_logic();

        // render ui
#if 0        
        if (adc_get_channel_rescaled(CHANNEL_ID_CH3) < 0) 
        {
            // show console on switch down
            console_render();
            screen_update();
        } 
        else 
#endif        
        if (gui_page & GUI_PAGE_SETUP_FLAG) 
        {
            // render setup ui
            gui_setup_render();
        } 
        else if (gui_page & GUI_PAGE_CONFIG_FLAG) 
        {
            // render config gui
            gui_config_render();
        } 
        else if (gui_startup_counter < (2000/GUI_LOOP_DELAY_MS)) 
        {
            // show logo
            lcd_show_logo();
            gui_startup_counter++;
        } 
        else 
        {
            // render normal ui
            gui_render();
        }
    }
}

static void gui_render_battery(void) {
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

static void gui_render_statusbar(void) {
    // render rx/tx rssi and battery status:
    // draw divider
    screen_fill_rect(0, 0, LCD_WIDTH, 7, 1);

    // draw battery voltage
    gui_render_battery();

    gui_render_rssi(111, 120);
}


static void gui_render_bottombar(void) {
    // render modelname at bottom
    // draw black border
    screen_fill_rect(0, LCD_HEIGHT - 7, LCD_WIDTH, 7, 1);

    screen_set_font(font_tomthumb3x5);
    screen_puts_centered(LCD_HEIGHT - 6 + font_tomthumb3x5[FONT_HEIGHT]/2, 0,
                         storage.model[storage.current_model].name);
}



static void gui_render_sliders(void) {
    uint32_t i;
    uint32_t y;

    // add statusbar
    gui_render_statusbar();

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


static void gui_config_stick_calibration_store_adc_values(void) 
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



void gui_render(void) {
    // start with empty screen
    screen_fill(0);

#if 0 // This messes up button handling, plus it can't work without touch screen
    // register page inc/dec callbacks
    gui_touch_callback_register(0, GUI_PREV_CLICK_X, 0, LCD_HEIGHT,
                                &gui_cb_previous_page);
    gui_touch_callback_register(LCD_WIDTH-GUI_PREV_CLICK_X, LCD_WIDTH, 0, LCD_HEIGHT,
                                &gui_cb_next_page);
#endif

    switch (gui_page) {
        default  :
        case (GUI_PAGE_MAIN) :
            // main status screen
            gui_render_main_screen();
            break;

        case (GUI_PAGE_STICKS) :
            // slider screen
            gui_render_sliders();
            break;

        case (GUI_PAGE_SETTINGS) :
            // setup and config screen
            gui_render_settings();
            break;
    }
    screen_update();
}

static void gui_render_settings(void) {
    const uint8_t *font = font_tomthumb3x5;
    screen_set_font(font);

    // render buttons and set callback
    gui_add_button_smallfont(64-50/2, 10, 50, 15, "SETUP",  &gui_cb_setup_enter);
    gui_add_button_smallfont(64-50/2, 40, 50, 15, "CONFIG", &gui_cb_config_enter);
}


static void gui_config_render(void) {
    // start with an empty page
    screen_fill(0);

    // render config
    switch (gui_page & (~(GUI_PAGE_CONFIG_OPTION_FLAG))) {
        default  :
        case (GUI_PAGE_CONFIG_MAIN) :
            // main settings menu
            gui_config_main_render();
            break;

        case (GUI_PAGE_CONFIG_STICK_CAL) :
            // stick calibration
            gui_config_stick_calibration_render();
            break;

        case (GUI_PAGE_CONFIG_MODEL_SETTINGS) :
            // model config
            gui_config_model_render();
            break;
    }

    screen_update();
}



static void gui_setup_render(void) {
    // start with an empty page
    screen_fill(0);

    // show setup pages
    switch (gui_page) {
        case (GUI_PAGE_SETUP_MAIN) :
            gui_setup_main_render();
            break;

        case (GUI_PAGE_SETUP_BIND) :
            // bind mode
            gui_setup_bindmode_render();
            break;

        case (GUI_PAGE_SETUP_BOOTLOADER) :
            // bootloader
            gui_setup_bootloader_render();
            break;

        default:
            // invalid, go back
            gui_page = GUI_PAGE_SETTINGS;
            break;
    }

    screen_update();
}

static void gui_config_header_render(const char *str) {
    // border + header string
    screen_fill_rect(0, 0, LCD_WIDTH, 7, 1);
    screen_draw_round_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, 3, 1);

    screen_set_font(font_tomthumb3x5);
    screen_puts_centered(1 + font_tomthumb3x5[FONT_HEIGHT]/2, 0, str);
}

static void gui_render_main_screen(void) {
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
    if (gui_model_timer < 0) {
        if ((gui_loop_100ms_counter % 4) == 0) {
            color = 1 - color;
        }
    }
    if ((gui_model_timer >= 0) && (gui_model_timer < 15)) {
        if ((gui_loop_100ms_counter % 10) == 0) {
            // beep!
            sound_play_low_time();
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
    screen_put_time(x, y, color, gui_model_timer);

#if 0 // This messes up button handling, plus it can't work without touch screen
    // register the reset callback
    gui_touch_callback_register(x, x + w, y, y + h, &gui_cb_model_timer_reload);
#endif    
}


static void gui_config_main_render(void) {
    const uint8_t *font = font_tomthumb3x5;
    screen_set_font(font);

    // header
    gui_config_header_render("MAIN CONFIGURATION");

    // render buttons and set callback
    gui_add_button_smallfont(3, 10 + 0*17, 50, 15, "STICK CAL", &gui_cb_config_stick_cal);
    gui_add_button_smallfont(3, 10 + 1*17, 50, 15, "MODEL CFG", &gui_cb_config_model);

    // exit button
    gui_add_button_smallfont(74, 10 + 2*17, 50, 15, "EXIT", &gui_cb_setup_exit);
}


static void gui_setup_main_render(void) {
    const uint8_t *font = font_tomthumb3x5;
    screen_set_font(font);

    // header
    gui_config_header_render("SETUP");

    // render buttons and set callback
    gui_add_button_smallfont(3, 10 + 0*17, 50, 15, "BIND MODE", &gui_cb_setup_bind);
    gui_add_button_smallfont(74, 10 + 0*17, 50, 15, "FW UPDATE", &gui_cb_setup_bootloader);

    // exit button, go back to main
    gui_add_button_smallfont(74, 10 + 2*17, 50, 15, "EXIT", &gui_cb_setup_exit);
}

static void gui_setup_bootloader_render(void) {
    // header
    gui_config_header_render("BOOTLOADER MODE");
    screen_puts_xy(3, 9, 1, "WILL ENTER BOOTLOADER NOW!");
    screen_update();

    // Our STM32 F072 has:
    // 16k SRAM in address 0x2000 0000 - 0x2000 3FFF
    *((uint32_t *)0x20003FF0) = 0xDEADBEEF;

    // reset the processor
    NVIC_SystemReset();
}

static void gui_setup_bindmode_render(void) {
    const uint8_t *font = font_tomthumb3x5;
    uint32_t h = font[FONT_HEIGHT]+1;

    // header
    gui_config_header_render("BIND");
    screen_puts_xy(3, 9, 1, "Sending bind packets...");
    screen_puts_xy(3, 9 + 3*h, 1, "CAUTION: UNTESTED...");

    screen_puts_xy(3, 9 + 7*h, 1, "Switch off TX to leave...");

    if (gui_config_counter == 0) {
        // TODO
        //frsky_enter_bindmode();
    }
    gui_config_counter++;

    if (gui_config_counter > 500/GUI_LOOP_DELAY_MS) {
        // play a sound every 2 seconds
        sound_play_bind();

        // next iteration
        gui_config_counter = 1;
    }
}

static void gui_config_model_render_main(void) {
    const uint8_t *font = font_system5x7;
    screen_set_font(font);

    uint32_t y = 12;

    // add model name
    screen_puts_centered(y, 1, storage.model[storage.current_model].name);

#if 0 // This messes up button handling, plus it can't work without touch screen
    // register the callback
    gui_touch_callback_register(20, LCD_WIDTH - 20, y, y + font[FONT_HEIGHT] + 1,
                                &gui_cb_setting_model_name);
#endif                                

    // add < ... > buttons
    gui_add_button_smallfont(3, y - 3, 15, 10, "<", &gui_cb_model_prev);
    gui_add_button_smallfont(LCD_WIDTH - 3 - 15, y - 3, 15, 10, ">", &gui_cb_model_next);
    y += 1 + font[FONT_HEIGHT];

    // add line
    // screen_draw_hline(0, y, LCD_WIDTH, 1);
    y += 2;

    // now use smaller font
    // font = font_tomthumb3x5;
    // screen_set_font(font);

    // add stick scaling
    gui_add_button_smallfont(3, y, 40, 13, "SCALE", &gui_cb_setting_model_stickscale);
    y += 13 + 1;

    // time
    gui_add_button_smallfont(3, y, 40, 13, "TIMER", &gui_cb_setting_model_timer);

    // render buttons and set callback
    gui_add_button_smallfont(89, 34 + 0*15, 35, 13, "SAVE", &gui_cb_config_save);
    gui_add_button_smallfont(89, 34 + 1*15, 35, 13, "BACK", &gui_cb_config_exit);
}


static void gui_render_option_window(const char *opt_name, f_ptr_32_32_t func) {
    // render window
    // clear region for window
    uint32_t window_w = LCD_WIDTH - 20;
    uint32_t window_h = 55;
    uint32_t y = (LCD_HEIGHT - window_h) / 2;
    uint32_t x = (LCD_WIDTH - window_w) / 2;
    // clear
    screen_fill_round_rect(x, y , window_w, window_h, 4, 0);
    // render border
    screen_draw_round_rect(x, y, window_w, window_h, 4, 1);
    y += 5;

    // font selection
    const uint8_t *font = font_system5x7;
    screen_set_font(font);

    // render text
    screen_puts_centered(y, 1, opt_name);
    y += font[FONT_HEIGHT] + 1;
    uint32_t len = screen_strlen(opt_name);
    screen_draw_hline((LCD_WIDTH - len) / 2, y, len, 1);
    y += 5;

    // render change modifier
    if (func != 0) {
        func(x, y);
    }

    // add buttons
    screen_set_font(font_tomthumb3x5);
    y = LCD_HEIGHT - (LCD_HEIGHT - window_h) / 2 - 16;
    gui_add_button_smallfont((LCD_WIDTH - 40) / 2, y, 40, 13, "OK", &gui_cb_setting_option_leave);
}

static void gui_cb_render_option_stickscale(uint32_t x, uint32_t y) {
    screen_set_font(font_system5x7);

    // render +/- button
    gui_add_button(15, y, 15, 15, "-", &gui_cb_model_stickscale_dec);
    gui_add_button(LCD_WIDTH - 15 - 15, y, 15, 15, "+", &gui_cb_model_stickscale_inc);

    // render value
    uint8_t temp = 123; // TODO, XXX storage.model[storage.current_model].stick_scale
    screen_put_uint8(LCD_WIDTH / 2 - screen_strlen("123") / 2, y, 1, temp);
}

static void gui_cb_render_option_timer(uint32_t x, uint32_t y) {
    screen_set_font(font_system5x7);

    // render +/- button
    gui_add_button(15, y, 15, 15, "-", &gui_cb_model_timer_dec);
    gui_add_button(LCD_WIDTH - 15 - 15, y, 15, 15, "+", &gui_cb_model_timer_inc);

    // render timer value
    screen_put_time(LCD_WIDTH / 2 - screen_strlen("1234") / 2,
                     y, 1, storage.model[storage.current_model].timer);
}

static void gui_config_model_render(void) {
    // header
    gui_config_header_render("MODEL SETTINGS");

    // render normal options
    gui_config_model_render_main();

    // single item selected?
    if (gui_page & GUI_PAGE_CONFIG_OPTION_FLAG) {
        // render single settings above main!
        // but first: remove all button callbacks
        gui_touch_callback_clear();

        // which options have to be changed?
        switch (gui_sub_page) {
            default:
            case (GUI_SUBPAGE_SETTING_MODEL_NAME) :
                gui_render_option_window("MODEL NAME", 0);
                break;

            case (GUI_SUBPAGE_SETTING_MODEL_SCALE) :
                gui_render_option_window("STICK SCALE", &gui_cb_render_option_stickscale);
                break;

            case (GUI_SUBPAGE_SETTING_MODEL_TIMER) :
                gui_render_option_window("TIMER", &gui_cb_render_option_timer);
                break;
        }
    }
}


static void gui_config_stick_calibration_render(void) {
    uint32_t i;
    uint32_t a;

    const uint8_t *font = font_tomthumb3x5;
    uint32_t h = font[FONT_HEIGHT] + 1;
    uint32_t w = font[FONT_FIXED_WIDTH] + 1;

    // store adc values
    gui_config_stick_calibration_store_adc_values();

    // draw ui
    gui_config_header_render("STICK CALIBRATION");

    uint32_t y = 8;
    //                       |                             |
    screen_puts_xy(3, y, 1, "Please move all sticks to the"); y += h;
    screen_puts_xy(3, y, 1, "extreme positions."); y += h;
    screen_puts_xy(3, y, 1, "When done, move all sticks to"); y += h;
    screen_puts_xy(3, y, 1, "the center and press save."); y += h;

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

    // render buttons and set callback
    gui_add_button_smallfont(89, 34 + 0*15, 35, 13, "SAVE", &gui_cb_config_stick_save);
    gui_add_button_smallfont(89, 34 + 1*15, 35, 13, "BACK", &gui_cb_config_exit);
}

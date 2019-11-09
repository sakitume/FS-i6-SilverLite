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
#include "tx_interface.h"

//------------------------------------------------------------------------------
extern GEM gGEM;

//------------------------------------------------------------------------------
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) > (b) ? (a) : (b))

//------------------------------------------------------------------------------
// Used: https://javl.github.io/image2cpp/
const unsigned char sprite_beeps_enabled[] =
{
    12, 7,
    // 'beep-on', 12x7px
    0x77, 0x6b, 0x5d, 0x3e, 0x3e, 0x00, 0x7f, 0x55, 0x55, 0x55, 0x55, 0x7f
};
const unsigned char sprite_beeps_disabled[] =
{
    12, 7,
    // 'beep-off', 12x7px
    0x77, 0x6b, 0x5d, 0x3e, 0x3e, 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 
};

//------------------------------------------------------------------------------
#define GUI_STATUSBAR_FONT font_tomthumb3x5

//------------------------------------------------------------------------------
static bool guiEnabled;
static int16_t modelTimer;
static uint8_t alarmBeepsDisabled;
static bool txRunning;

// Forward declarations
static void startTX();
static void stopTX();
static void resetTX();

//------------------------------------------------------------------------------
class CShortLongPress
{
public:    
    CShortLongPress(uint8_t _buttonID, uint8_t _millis100 = 25) : buttonID(_buttonID), millis100(_millis100) {}
    int check();
    void reset();

private:    
    uint8_t buttonID;
    uint8_t millis100;
    uint32_t lastActivated;
};

//------------------------------------------------------------------------------
class CDialogBase
{
public:    
    virtual void drawMessage(int y, int h) = 0;
    virtual void checkOkCancel();
    virtual void close(bool wasOkay);
};

class CDialogLongPress : public CDialogBase
{
public:    
    CDialogLongPress() : exitDialogLongPress(kBtn_Ok, 10) {}
    virtual void checkOkCancel();

private:
    CShortLongPress exitDialogLongPress;    
};

class CDialogExit : public CDialogLongPress
{
public:
    virtual void drawMessage(int y, int h);
    virtual void close(bool wasOkay);
};

class CDialogBind : public CDialogLongPress
{
public:
    virtual void drawMessage(int y, int h);
    virtual void close(bool wasOkay);
};

class CDialogStart : public CDialogBase
{
public:    
    virtual void drawMessage(int y, int h);
    virtual void checkOkCancel();
};

static CDialogStart startDialog;
static CDialogBind bindDialog;
static CDialogExit exitDialog;
static CDialogBase *activeDialog;

//------------------------------------------------------------------------------
void CDialogBase::close(bool wasOkay)
{
    activeDialog = nullptr;
}

//------------------------------------------------------------------------------
void CDialogBase::checkOkCancel()
{
    if (button_toggledActive(kBtn_Cancel))
    {
        close(false);
    }
    if (button_toggledActive(kBtn_Ok))
    {
        close(true);
    }
}

//------------------------------------------------------------------------------
// This version requires that the OK button be long pressed
void CDialogLongPress::checkOkCancel()
{
    if (button_toggledActive(kBtn_Cancel))
    {
        exitDialogLongPress.reset();
        close(false);
    }
    if (exitDialogLongPress.check() > 0)
    {
        close(true);
    }
}

//------------------------------------------------------------------------------
void CDialogExit::drawMessage(int y, int h)
{
    screen_puts_centered(y, 1, "Long press OK to");
    y += h;
    screen_puts_centered(y, 1, "exit to main menu");
}

//------------------------------------------------------------------------------
void CDialogExit::close(bool wasOkay)
{
    if (wasOkay)
    {
        // Exit this gui app context
        guiEnabled = false;
    }
    CDialogLongPress::close(wasOkay);
}

//------------------------------------------------------------------------------
void CDialogBind::drawMessage(int y, int h)
{
    y -= h >> 1;
    screen_puts_centered(y, 1, "Long press OK to");
    y += h;
    screen_puts_centered(y, 1, "reset TX");
    y += h;
    screen_puts_centered(y, 1, "and rebind");
}

//------------------------------------------------------------------------------
void CDialogBind::close(bool wasOkay)
{
    if (wasOkay)
    {
        resetTX();
    }
    CDialogLongPress::close(wasOkay);
}

//------------------------------------------------------------------------------
void CDialogStart::drawMessage(int y, int h)
{
    y -= h >> 1;
    screen_puts_centered(y, 1, "Reset switches");
    y += h;
    screen_puts_centered(y, 1, "and set Throttle");
    y += h;
    screen_puts_centered(y, 1, "to zero");
}

//------------------------------------------------------------------------------
static bool SafeToStart()
{
    // Check switches and throttle, if any are active bring up the warning dialog
    enum { kThrottleThreshold = 20 };
    return ((adc_get_channel_calibrated(ADC_ID_THROTTLE) < kThrottleThreshold) && !button_active(kBtn_SwA) && !button_active(kBtn_SwD));
}

//------------------------------------------------------------------------------
void CDialogStart::checkOkCancel()
{
    if (SafeToStart())
    {
        startTX();
        close(true);
    }
}

//------------------------------------------------------------------------------
// Returns:
//  1   if button was long pressed
//  -1  if button was short pressed (released and it wasn't a long press)
//  0   None of the above
int CShortLongPress::check()
{
    if (button_toggled((e_BtnIndex)buttonID))
    {
        if (button_active((e_BtnIndex)buttonID))
        {
            lastActivated = millis_this_frame();
        }
        else if (lastActivated)
        {
            lastActivated = 0;
            return -1;
        }
    }
    else if (lastActivated)
    {
        // If held down for the requested number of 100millis
        uint32_t delta = millis_this_frame() - lastActivated;
        if (delta >= (millis100 * 100))
        {
            lastActivated = 0;
            return 1;
        }
    }
    return 0;
}
//------------------------------------------------------------------------------
void CShortLongPress::reset()
{
    lastActivated = 0;
}

//------------------------------------------------------------------------------
static void ReloadModelTimer() 
{
    modelTimer = (int16_t) storage.model[storage.current_model].timer;
}

//------------------------------------------------------------------------------
void gui_handle_buttons() 
{
    static CShortLongPress timerLongPress(kBtn_Ok, 15);

    if (activeDialog)
    {
        timerLongPress.reset();
        activeDialog->checkOkCancel();
        return;
    }
    else if (button_toggledActive(kBtn_Cancel))
    {
        activeDialog = &exitDialog;
        return;
    }
    else if (button_toggledActive(kBtn_Bind))
    {
        activeDialog = &bindDialog;
        return;
    }


    // If user short or long presses OK button
    int okButtonCheck = timerLongPress.check();
    if (okButtonCheck > 0)
    {
        sound_play_bind();
        ReloadModelTimer();
    }
    else if (okButtonCheck < 0)
    {
        // toggle alarm beep enabled switch
        alarmBeepsDisabled = !alarmBeepsDisabled;
    }
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

    // Count down when throttle is past zero and TX is running
    if (txRunning)
    {
        enum { kThrottleThreshold = 30 };   // TODO
        if (adc_get_channel_calibrated(ADC_ID_THROTTLE) >= kThrottleThreshold) {
            // do timer logic, handle countdown
            if (second_elapsed) {
                modelTimer--;
            }
        }

        if ((modelTimer >= 0) && (modelTimer < 15)) {
            if ((gui_loop_100ms_counter % 10) == 0) {
                if (!alarmBeepsDisabled) {
                        sound_play_low_time();
                    }
                led_backlight_tickle();
            }
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
static void gui_render_rssi() 
{
#if 0
    uint32_t rssi_fc = tx_get_rssi_fc();
    uint32_t rssi_tx = tx_get_rssi_tx();

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
#endif    
}

//------------------------------------------------------------------------------
static void gui_render_statusbar() {
    // render rx/tx rssi and battery status:
    // draw divider
    screen_fill_rect(0, 0, LCD_WIDTH, 7, 1);

    // draw battery voltage
    gui_render_battery();

    gui_render_rssi();

    if (alarmBeepsDisabled)
    {
        // LCD_WIDTH-16
            screen_put_sprite(0, 0, sprite_beeps_disabled, 0);
    }
    else
    {
            screen_put_sprite(0, 0, sprite_beeps_enabled, 0);
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
static void renderDialog() {

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
    screen_set_font(font_system5x7);
    uint32_t h = font_system5x7[FONT_HEIGHT]+1;

    // render text
    const char *msg = "Alert";
    screen_puts_centered(y, 1, msg);
    y += h;
    uint32_t len = screen_strlen(msg);
    screen_draw_hline((LCD_WIDTH - len) / 2, y, len, 1);
    y += 15;

    activeDialog->drawMessage(y, h);
}


//------------------------------------------------------------------------------
static void renderTX() {
    uint32_t x;
    uint32_t y;

    // do statusbars
    gui_render_statusbar();
    gui_render_bottombar();

    // render voltage
    screen_set_font(font_metric7x12);
    x = 1;
    y = 10;
    uint32_t rssi_tx = tx_get_rssi_tx();
    screen_put_uint8(x, y, 1, rssi_tx);
    x += (font_metric7x12[FONT_FIXED_WIDTH]+1)*3 + 3;
    screen_puts_xy(x, y, 1, "T");

    x = 1;
    y += font_metric7x12[FONT_HEIGHT]+1;
    uint32_t rssi_fc = tx_get_rssi_fc();
    screen_put_uint8(x, y, 1, rssi_fc);
    x += (font_metric7x12[FONT_FIXED_WIDTH]+1)*3 + 3;
    screen_puts_xy(x, y, 1, "R");

    x = LCD_WIDTH - (font_metric7x12[FONT_FIXED_WIDTH]+1)*7 - 1;
    y += font_metric7x12[FONT_HEIGHT]+1;
    y += 5;
    uint16_t fcVolts = tx_get_fc_voltage();
    screen_put_fixed2(x, y, 1, fcVolts);
    x += (font_metric7x12[FONT_FIXED_WIDTH]+1)*4 + 1;
    screen_puts_xy(x, y, 1, "V");

    // screen_set_font(font_metric7x12);
    screen_set_font(font_metric15x26);

    // render time
    uint32_t color = 1;
    if (modelTimer < 0) {
        if ((gui_loop_100ms_counter % 4) == 0) {
            color = 1 - color;
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
    guiEnabled = true;
    ReloadModelTimer();

    // Check switches and throttle, if any are active bring up the warning dialog
    if (!SafeToStart())
    {
        activeDialog = &startDialog;
    }
    else
    {
        startTX();
    }
}

//------------------------------------------------------------------------------
static void txCtxRender() 
{
    screen_fill(0);
    if (txRunning)
    {
        renderTX();
    }
    if (activeDialog)
    {
        renderDialog();   
    }
    screen_update();
}

//------------------------------------------------------------------------------
static void txCtxLoop() 
{
    gui_process_logic();
    gui_handle_buttons();

    // If process_logic or handle_buttons disabled the run flag
    if (!guiEnabled)
    {
        gGEM.context.exit();
        return;
    }

    txCtxRender();
}

//------------------------------------------------------------------------------
static void txCtxExit() 
{
    stopTX();

    // Exit back to GEM
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

//------------------------------------------------------------------------------
static void startTX()
{
    txRunning = true;
    tx_start();
}

//------------------------------------------------------------------------------
static void stopTX()
{
    txRunning = false;
    tx_stop();
}

//------------------------------------------------------------------------------
static void resetTX()
{
    tx_reset();
}

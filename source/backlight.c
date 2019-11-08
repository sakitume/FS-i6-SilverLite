#include "fsl_gpio.h"
#include "pin_mux.h"
#include "drv_time.h"
#include "buttons.h"
#include "backlight.h"

static unsigned millisUntilBacklightOff;

//------------------------------------------------------------------------------
void led_backlight_init(void)
{
	gpio_pin_config_t led_config = {
		kGPIO_DigitalOutput, 0,
	};
	/* Init output LED GPIO. */
	GPIO_PinInit(BOARD_INITPINS_LED_BACKLIGHT_GPIO, BOARD_INITPINS_LED_BACKLIGHT_PIN, &led_config);

	led_backlight_tickle();
}

//------------------------------------------------------------------------------
void led_backlight_on()
{
	GPIO_WritePinOutput(BOARD_INITPINS_LED_BACKLIGHT_GPIO, BOARD_INITPINS_LED_BACKLIGHT_PIN, 1);
}

//------------------------------------------------------------------------------
void led_backlight_off()
{
	GPIO_WritePinOutput(BOARD_INITPINS_LED_BACKLIGHT_GPIO, BOARD_INITPINS_LED_BACKLIGHT_PIN, 0);
}

//------------------------------------------------------------------------------
void led_backlight_update()
{
   	// Check if lcd backlight should be turned on or off
    // If any of up/down/ok/cancel/bind buttons are active then light should be on;
    bool buttonActive = (button_active(kBtn_Up) || button_active(kBtn_Down) || button_active(kBtn_Ok)
        || button_active(kBtn_Cancel) || button_active(kBtn_Bind));
    if (buttonActive)
    {
		led_backlight_tickle();
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
void led_backlight_tickle()
{
	if (!millisUntilBacklightOff)
	{
		led_backlight_on();
	}

	// TODO: The 30 second value could be an option we persist in FlashStorage_t
	// and editable with gui
	millisUntilBacklightOff = millis_this_frame() + 30000;  // 30 seconds
}
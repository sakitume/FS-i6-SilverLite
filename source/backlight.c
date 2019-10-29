/*
 * backlight.c
 *
 *  Created on: 15 lut 2017
 *      Author: Kuba
 */


#include "backlight.h"
#include "MKL16Z4.h"
#include "fsl_gpio.h"
#include "pin_mux.h"

void led_backlight_init(void)
{
	gpio_pin_config_t led_config = {
		kGPIO_DigitalOutput, 1,
	};
	/* Init output LED GPIO. */
	GPIO_PinInit(BOARD_INITPINS_LED_BACKLIGHT_GPIO, BOARD_INITPINS_LED_BACKLIGHT_PIN, &led_config);
}

void led_backlight_on()
{
	GPIO_WritePinOutput(BOARD_INITPINS_LED_BACKLIGHT_GPIO, BOARD_INITPINS_LED_BACKLIGHT_PIN, 1);
}

void led_backlight_off()
{
	GPIO_WritePinOutput(BOARD_INITPINS_LED_BACKLIGHT_GPIO, BOARD_INITPINS_LED_BACKLIGHT_PIN, 0);
}
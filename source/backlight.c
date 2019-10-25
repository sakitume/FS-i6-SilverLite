/*
 * backlight.c
 *
 *  Created on: 15 lut 2017
 *      Author: Kuba
 */


#include "backlight.h"
#include "MKL16Z4.h"
#include "fsl_gpio.h"

static gpio_pin_config_t led_config = {
	kGPIO_DigitalOutput, 0,
};

void backlightInit(void)
{
	/* Init output LED GPIO. */
	GPIO_PinInit(BACKLIGHT_LED_GPIO, BACKLIGHT_LED_PIN, &led_config);
	GPIO_SetPinsOutput(BACKLIGHT_LED_GPIO, (1U << BACKLIGHT_LED_PIN));
}

void led_backlight_on()
{
	GPIO_SetPinsOutput(BACKLIGHT_LED_GPIO, (1U << BACKLIGHT_LED_PIN));
}

void led_backlight_off()
{
	GPIO_ClearPinsOutput(BACKLIGHT_LED_GPIO, (1U << BACKLIGHT_LED_PIN));
}
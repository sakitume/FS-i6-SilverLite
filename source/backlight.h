/*
 * backlight.h
 *
 *  Created on: 15 lut 2017
 *      Author: Kuba
 */

#ifndef SOURCE_BACKLIGHT_H_
#define SOURCE_BACKLIGHT_H_

#define BACKLIGHT_LED_GPIO GPIOB
#define BACKLIGHT_LED_PIN  0

void __attribute__((section (".TestCODE"))) backlightInit(void);

void led_backlight_on();
void led_backlight_off();

#endif /* SOURCE_BACKLIGHT_H_ */

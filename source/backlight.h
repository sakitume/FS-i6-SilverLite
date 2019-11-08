/*
 * backlight.h
 *
 *  Created on: 15 lut 2017
 *      Author: Kuba
 */

#ifndef SOURCE_BACKLIGHT_H_
#define SOURCE_BACKLIGHT_H_
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

void __attribute__((section (".TestCODE"))) led_backlight_init(void);

void led_backlight_on();
void led_backlight_off();

void led_backlight_update();
void led_backlight_tickle();


#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* SOURCE_BACKLIGHT_H_ */

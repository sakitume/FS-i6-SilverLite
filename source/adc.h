/*
    Copyright 2016 fishpepper <AT> gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    author: fishpepper <AT> gmail.com
*/

#ifndef ADC_H_
#define ADC_H_
#include <stdbool.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef enum 
{
    ADC_ID_AILERON = 0,
    ADC_ID_ELEVATION,
    ADC_ID_THROTTLE,
    ADC_ID_RUDDER,
    ADC_ID_CH0,
    ADC_ID_CH1,
    ADC_ID_SwB,
    ADC_ID_SwC,
    ADC_ID_BATTERY,
    ADC_CHANNEL_COUNT
} e_ADCChannel;

int adc_init(void);
void adc_update(void);  // currently takes 183 microseconds
void adc_test(void);

uint16_t adc_get_channel_raw(int id);
uint16_t adc_get_channel_calibrated(int id);
uint16_t adc_get_channel_calibrated_unscaled(int id);

// Return a fixed point number of the battery voltage: 1230 = 12.3 V
uint32_t adc_get_battery_voltage(void);

const char *adc_get_channel_name(int id, bool short_descr);


void adc_calibrate_sticks();


#if defined(__cplusplus)
}
#endif /* __cplusplus */


#endif  // ADC_H_

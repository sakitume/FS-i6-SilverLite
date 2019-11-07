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

#include "adc.h"
#include "debug.h"
#include "console.h"
#include "fsl_adc16.h"
#include "fsl_debug_console.h"
#include "fsl_port.h"
#include "delay.h"
#include "storage.h"

static uint16_t adc_data[ADC_CHANNEL_COUNT];

static uint16_t adc_battery_voltage_raw_filtered;

static const uint8_t adac_channels[] = {
	7,  // ADC0_SE7b, Ch1 Roll (Aileron)
	6,  // ADC0_SE6b, Ch2 Pitch (Elevator)
	5,  // ADC0_SE5b, Ch3 Throttle (Throttle)
	11, // ADC0_SE11, Ch4 Yaw (Rudder)
	3,  // ADC0_SE3,  Ch5 VrA
	7,  // ADC0_SE7a, Ch6 VrB
	4,  // ADC0_SE4b, Battery Voltage
	14, // ADC0_SE14, SwC
	15, // ADC0_SE15, SwB
};

static bool bNeedRecalibrate;

//------------------------------------------------------------------------------
// Forward declarations
static void adc_init_internal(void);
static void LoadCalibrationData();

//------------------------------------------------------------------------------
int adc_init(void)
{
	debug("adc: init\n"); 
    debug_flush();

	adc_battery_voltage_raw_filtered = 0;
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) 
    {
        adc_data[i] = 0;
    }
    adc_init_internal();

    LoadCalibrationData();
    return bNeedRecalibrate;
}

static void adc_init_internal(void)
{
	CLOCK_EnableClock(kCLOCK_Adc0);
	CLOCK_EnableClock(kCLOCK_PortC);
	CLOCK_EnableClock(kCLOCK_PortD);
	CLOCK_EnableClock(kCLOCK_PortE);
	PORT_SetPinMux(PORTC, 0, kPORT_PinDisabledOrAnalog); /* PORTC0 (pin 43) is configured as ADC0_SE14 */
	PORT_SetPinMux(PORTC, 1, kPORT_PinDisabledOrAnalog); /* PORTC1 (pin 44) is configured as ADC0_SE15 */
	PORT_SetPinMux(PORTC, 2, kPORT_PinDisabledOrAnalog); /* PORTC2 (pin 45) is configured as ADC0_SE11 */
	PORT_SetPinMux(PORTD, 1, kPORT_PinDisabledOrAnalog); /* PORTD1 (pin 58) is configured as ADC0_SE5b */
	PORT_SetPinMux(PORTD, 5, kPORT_PinDisabledOrAnalog); /* PORTD5 (pin 62) is configured as ADC0_SE6b */
	PORT_SetPinMux(PORTD, 6, kPORT_PinDisabledOrAnalog); /* PORTD6 (pin 63) is configured as ADC0_SE7b */
	PORT_SetPinMux(PORTE, 22, kPORT_PinDisabledOrAnalog); /* PORTE22 (pin 11) is configured as ADC0_SE3 */
	PORT_SetPinMux(PORTE, 23, kPORT_PinDisabledOrAnalog); /* PORTE23 (pin 12) is configured as ADC0_SE7a */
	PORT_SetPinMux(PORTE, 29, kPORT_PinDisabledOrAnalog); /* PORTE29 (pin 17) is configured as ADC0_SE4b */

	adc16_config_t config = {
		kADC16_ReferenceVoltageSourceVref,
		kADC16_ClockSourceAlt0,
		false,
		kADC16_ClockDivider2,
		kADC16_Resolution12or13Bit,
		kADC16_LongSampleCycle6,
		true,
		false,
		true //not sure
	};
	//ADC16_Init(ADC0, &config);
	//ADC16_SetChannelMuxMode(ADC0, kADC16_ChannelMuxB);
	ADC0->CFG1 = 52;
	ADC0->CFG2 = 22;
	ADC0->SC2 = 0;
	//ADC0->SC3 = 133;
	ADC0->SC3 = 5;
	ADC16_DoAutoCalibration(ADC0);
}

void adc_update(void){
	for(int i =0; i < ADC_CHANNEL_COUNT; i++){
		if(i == 5) ADC0->CFG2 &= 0xFFFFFFEF; //select ADxxa
		else ADC0->CFG2 |= 0x10; //select ADxxb
		ADC0->SC1[0] = (adac_channels[i] | 0x40) & ADC_SC1_ADCH_MASK;
		//Write to SC1A to start conversion
		while (ADC0->SC2 & ADC_SC2_ADACT_MASK);
		while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK));
		adc_data[i] = ADC0->R[0];
	}

    adc_battery_voltage_raw_filtered = adc_data[6];
}

//------------------------------------------------------------------------------
void adc_test(void) {
        console_clear();
        debug("ADC TEST");
        debug_put_newline();
        debug_put_fixed2(adc_get_battery_voltage());
        debug(" V\n");
        uint32_t i;
        for (i = 0; i < ADC_CHANNEL_COUNT; i++) {
            debug_put_uint8(i+0); debug_putc('=');
//            debug_put_uint16(adc_data[i]);
            debug_put_uint16(adc_get_channel_calibrated(i));
            if (i&1) {
                debug_put_newline();
            } else {
                debug(" ");
            }
        }
        debug_flush();
}


//------------------------------------------------------------------------------
const char *adc_get_channel_name(int id, bool short_descr) 
{
    switch (id) 
    {
        default                 : return ((short_descr) ? "?" : "???");
        case (ADC_ID_AILERON)   : return ((short_descr) ? "A" : "AIL");
        case (ADC_ID_ELEVATION) : return ((short_descr) ? "E" : "ELE");
        case (ADC_ID_THROTTLE)  : return ((short_descr) ? "T" : "THR");
        case (ADC_ID_RUDDER)    : return ((short_descr) ? "R" : "RUD");
        case (ADC_ID_CH0)       : return ((short_descr) ? "0" : "VRA");
        case (ADC_ID_CH1)       : return ((short_descr) ? "1" : "VRB");
        case (ADC_ID_SwB)       : return ((short_descr) ? "2" : "SwB");
        case (ADC_ID_SwC)       : return ((short_descr) ? "3" : "SwC");
        case (ADC_ID_BATTERY)   : return ((short_descr) ? "V" : "VLT");
    }
}

//------------------------------------------------------------------------------
uint32_t adc_get_battery_voltage(void) {
    // return a fixed point number of the battery voltage
    // 1230 = 12.3 V
    // raw data is 0 .. 4095 ~ 0 .. 3300mV
    // Vadc = raw * 3300 / 4095
    uint32_t raw = adc_battery_voltage_raw_filtered;
    // the voltage divider is 5.1k / 10k
    // Vadc = Vbat * R2 / (R1+R2) = Vbat * 51/ 151
    // -> Vbat = Vadc * (R1-R2) / R2
    // -> Vout = raw * 3300 * (151 / 51) / 4095
    //         = (raw * (3300 * 151) ) / (4095 * 51)
    uint32_t mv = (raw * (3300 * 151) ) / (4095 * 51);
    return mv / 10;
}

//------------------------------------------------------------------------------
static void SetupSaneDefaults()
{
    // Provide default calibration data should previously stored calibration
    // data be unavailable
    //
    // Set min/max to default values (smaller than the full range)
    // Set mid to absolute mid
    for (int chan=0; chan<4; chan++)
    {
        uint16_t *minMidMax = storage.chanMinMidMax[chan];
        minMidMax[0] = 400;
        minMidMax[1] = 2048;
        minMidMax[2] = 4096 - 400;
    }
}

//------------------------------------------------------------------------------
static void LoadCalibrationData()
{
    if (!storage_is_valid())
    {
        PRINTF("Unable to load stick calibration data from flash");
        SetupSaneDefaults();
    }
    else
    {
        // Check if previously stored min/max values are missing or seem invalid
        bNeedRecalibrate = false;
        for (int chan=0; chan<4; chan++)
        {
            const uint16_t *minMidMax = storage.chanMinMidMax[chan];
            uint16_t cmin = minMidMax[0];
            uint16_t cmid = minMidMax[1];
            uint16_t cmax = minMidMax[2];

            if (cmin >= cmax)
            {
                PRINTF("Stick calibration is required: bad min/max for channel: %d\n", chan);
                bNeedRecalibrate = true;
                break;
            }
            if ((cmax - cmin) > 4096)
            {
                PRINTF("Stick calibration is required: bad range for channel: %d\n", chan);
                bNeedRecalibrate = true;
                break;
            }
            int diff = (int)cmid - 2048;
            if (diff < 0) diff = -diff;
            if (diff > 300)
            {
                PRINTF("Stick calibration is required: bad mid value for channel: %d\n", chan);
                bNeedRecalibrate = true;
                break;
            }
        }

        if (bNeedRecalibrate)
        {
            SetupSaneDefaults();
        }
    }
}

//------------------------------------------------------------------------------
uint16_t adc_get_channel_raw(int id) 
{
    switch (id) 
    {
        case (ADC_ID_AILERON)  : return adc_data[0];
        case (ADC_ID_ELEVATION): return adc_data[1];
        case (ADC_ID_THROTTLE) : return adc_data[2];
        case (ADC_ID_RUDDER)   : return adc_data[3];
        case (ADC_ID_CH0)      : return adc_data[4];
        case (ADC_ID_CH1)      : return adc_data[5];
        case (ADC_ID_SwB)      : return adc_data[8];
        case (ADC_ID_SwC)      : return adc_data[7];
        case (ADC_ID_BATTERY)  : return adc_data[6];
        default:                 return 0;
    }
}

//------------------------------------------------------------------------------
uint16_t adc_get_channel_calibrated(int id) 
{
    uint16_t rawVal = adc_get_channel_raw(id);
    int32_t calVal;

    // If the channel is a stick value (AETR)
    if (id <= ADC_ID_RUDDER)
    {
        uint16_t *minMidMax = storage.chanMinMidMax[id];
        if (rawVal < minMidMax[0])
        {
            minMidMax[0] = rawVal;
        }
        if (rawVal > minMidMax[2])
        {
            minMidMax[2] = rawVal;
        }

        const int mid = (int)minMidMax[1];

        int32_t divider;
        calVal = (int)rawVal - mid;
        if (calVal < 0)
        {
            divider = mid - (int)minMidMax[0];
        }
        else
        {
            divider = minMidMax[2] - (int)mid;
        }
        calVal = ((calVal * 2048) / divider) + 2048;
    }
    else
    {
        calVal = rawVal;
    }

    // Scale the value from range 4096 to range 1024
    calVal >>= 2;
    
    // Clamp to 0..1023 inclusive
    if (calVal >= 1024)
    {
        calVal = 1023;
    }
    else if (calVal < 0)
    {
        calVal = 0;
    }
    return calVal;
}

// TODO: I don't like that adc.cpp has to know about all of these other
// systems. Maybe move into an adc_ui.cpp
#include "drv_time.h"
#include "buttons.h"
#include "console.h"
#include "debug.h"

//------------------------------------------------------------------------------
extern void required_updates();

//------------------------------------------------------------------------------
void adc_calibrate_sticks()
{
    // 0 = Prompt to center sticks
    // 1 = Wait until OK or Cancel
    // 2 = Prompt to move sticks
    // 3 = Wait until OK or Cancel
    int state = 0;  
    while (state >= 0)
    {
        required_updates();

        switch (state)
        {
            case 0:
            {
                // Set min/max to default values (smaller than the full range)
                for (int chan=0; chan<4; chan++)
                {
                    uint16_t *minMidMax = storage.chanMinMidMax[chan];
                    minMidMax[0] = 400;
                    minMidMax[2] = 4096 - 400;
                }

                console_clear();
                debug("Place sticks in their center positions");
                debug_put_newline();
                debug_put_newline();
                debug("Then press OK to continue.");
                debug_flush();

                state++;
            }
            break;

            case 1:
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
                if (button_toggledActive(kBtn_Ok))
                {
                    console_clear();
                    for (int i=0; i<4; i++)
                    {
                        uint16_t *minMidMax = storage.chanMinMidMax[i];
                        debug_put_uint16(minMidMax[0]);
                        debug_putc(' ');
                        debug_put_uint16(minMidMax[1] >> 2);
                        debug_putc(' ');
                        debug_put_uint16(minMidMax[2]);
                        debug_put_newline();
                    }
                    debug_put_newline();
                    debug("Press OK to continue.");
                    debug_flush();
                    state++;
                }
            }
            break;

            case 2:
            {
                if (button_toggledActive(kBtn_Ok))
                {
                    console_clear();
                    debug("Move sticks to their extremes");
                    debug_put_newline();
                    debug_put_newline();
                    debug("Press OK to continue.");
                    debug_flush();

                    state++;
                }
            }
            break;

            case 3:
            {
                // Call adc_get_channel_calibrated() so that
                // min/max values get updated
                for (int i=0; i<4; i++)
                {
                    adc_get_channel_calibrated(i);
                }

                if (button_toggledActive(kBtn_Ok))
                {
                    console_clear();
                    for (int i=0; i<4; i++)
                    {
                        uint16_t *minMidMax = storage.chanMinMidMax[i];
                        debug_put_uint16(minMidMax[0] >> 2);
                        debug_putc(' ');
                        debug_put_uint16(minMidMax[1] >> 2);
                        debug_putc(' ');
                        debug_put_uint16(minMidMax[2] >> 2);
                        debug_put_newline();
                    }
                    debug_put_newline();
                    debug("Press OK to continue.");
                    debug_flush();
                    state++;
                }
            }
            break;

            case 4:
            {
                if (button_toggledActive(kBtn_Cancel))
                {
                    state = 0;
                }
                else if (button_toggledActive(kBtn_Ok))
                {
                    state++;
                }
            }              
            break;      


            case 5:
            {
                adc_test();

                if (button_toggledActive(kBtn_Cancel))
                {
                    state = 0;
                }
                else if (button_toggledActive(kBtn_Ok))
                {
                    storage_save();
                    state++;
                }
            }              
            break;      


            default:
                state = -1;
                break;
        }
    }
}
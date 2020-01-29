/*
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License.
 If not, see <http://www.gnu.org/licenses/>.
 */
//#include <string.h>
//#include "fsl_pit.h"
#include "fsl_debug_console.h"
//#include "fsl_common.h"

#include "drv_nRF24L01.h"
#include "drv_time.h"

#include "bayang_common.h"
#include "bayang.h"

#include "console.h"
#include "screen.h"
#include "debug.h"
#include "buttons.h"
#include "adc.h"

//------------------------------------------------------------------------------
void Bayang_tx_reset(enum EProtocol protocol, uint8_t options)
{
    // Our ISR will notice the protocol value on the next timer interrupt
    // and perform the necessary reset sequence
    gTXContext.resetToProtocol  = protocol;
    gTXContext.resetToOptions   = options;
}

//------------------------------------------------------------------------------
enum
{
    // flags going to packet[2]
    BAYANG_FLAG_RTH      = 0x01,
    BAYANG_FLAG_HEADLESS = 0x02,
    BAYANG_FLAG_FLIP     = 0x08,
    BAYANG_FLAG_VIDEO    = 0x10,
    BAYANG_FLAG_SNAPSHOT = 0x20,
};

enum
{
    // flags going to packet[3]
    BAYANG_FLAG_EMG_STOP = 0x04,
    BAYANG_FLAG_INVERT   = 0x80,
    BAYANG_FLAG_TAKE_OFF = 0x20
};

// From Silverware (NFE fork). When "USE_DEVO" is defined:
// devo tx channel mapping
// also for nr24multipro
//#define CHAN_5 CH_INV           // BAYANG_FLAG_INVERT
//#define CHAN_6 CH_FLIP          // BAYANG_FLAG_FLIP
//#define CHAN_7 CH_PIC           // BAYANG_FLAG_SNAPSHOT
//#define CHAN_8 CH_VID           // BAYANG_FLAG_VIDEO
//
//#define CHAN_9 CH_HEADFREE      // BAYANG_FLAG_HEADLESS
//#define CHAN_10 CH_RTH          // BAYANG_FLAG_RTH
//#define CHAN_11 CH_TO           // BAYANG_FLAG_TAKE_OFF
//#define CHAN_12 CH_EMG          // BAYANG_FLAG_EMG_STOP

// These two are only available if USE_ANALOG_AUX was defined for Silverware
//#define CHAN_13 CH_ANA_AUX1
//#define CHAN_14 CH_ANA_AUX2

// Also, I'm using these settings when I build SilverWare NFE
//
//#define ARMING CHAN_5     BAYANG_FLAG_INVERT (packet[3]), SwA
//#define IDLE_UP CHAN_9    BAYANG_FLAG_HEADLESS (packet[2]), SwD
//#define LEVELMODE CHAN_6  BAYANG_FLAG_FLIP (packet[2]), SwB: Position 1
//#define RACEMODE  CHAN_7  BAYANG_FLAG_SNAPSHOT (packet[2]), SwC: Position 2
//#define HORIZON   CHAN_8  BAYANG_FLAG_VIDEO (packet[2]), SwC: Position 3

//#define RATES CHAN_ON
//#define LEDS_ON CHAN_OFF	// Yes! CHAN_OFF will turn on the LEDs

//------------------------------------------------------------------------------
void Bayang_tx_update()
{
    // Also, I'm using these settings when I build SilverWare NFE
    //
    //#define ARMING CHAN_5     BAYANG_FLAG_INVERT (packet[3]), SwA
    //#define IDLE_UP CHAN_9    BAYANG_FLAG_HEADLESS (packet[2]), SwD
    //#define LEVELMODE CHAN_6  BAYANG_FLAG_FLIP (packet[2]), SwB: Position 1
    //#define RACEMODE  CHAN_7  BAYANG_FLAG_SNAPSHOT (packet[2]), SwC: Position 2
    //#define HORIZON   CHAN_8  BAYANG_FLAG_VIDEO (packet[2]), SwC: Position 3
    
    // ADC_ID_SwB == SwB: Position 1 == <100(46), Position 2 == >923(1023)
    // ADC_ID_SwC == SwC: Position 1 == 0, Position 2 == 512, Position 3 == 1023

    txPkt_Flags2 = 0;
    if (button_active(kBtn_SwD))
    {
        txPkt_Flags2 |= BAYANG_FLAG_HEADLESS;
    }
    if (adc_get_channel_calibrated(ADC_ID_SwB) < 100)   // SwB: Position 1
    {
        txPkt_Flags2 |= BAYANG_FLAG_FLIP;       // CHAN_6, LEVELMODE
    }
    uint16_t SwC = adc_get_channel_calibrated(ADC_ID_SwC);
    if (SwC >= 923)         // SwC: Position 3
    {
        txPkt_Flags2 |= BAYANG_FLAG_VIDEO;      // CHAN_8, HORIZON
    }
    else if (SwC >= 412)         // SwC: Position 2
    {
        txPkt_Flags2 |= BAYANG_FLAG_SNAPSHOT;   // CHAN_7, RACEMODE
    }
    if (button_active(kBtn_SwD))
    {
        txPkt_Flags2 |= BAYANG_FLAG_HEADLESS;   // CHAN_9, IDLE_UP
    }


    txPkt_Flags3 = button_active(kBtn_SwA) ? BAYANG_FLAG_INVERT : 0;

    // AETR channel values
    for (int i=0; i<4; i++)
    {
        txPkt_Sticks[i] = adc_get_channel_calibrated(i);
    }

    // Auxliary analog channels
    // Note: We convert from range 0..1023 to 0..255
    txPkt_AuxAnalog[0] = adc_get_channel_calibrated(ADC_ID_CH0) >> 2;
    txPkt_AuxAnalog[1] = adc_get_channel_calibrated(ADC_ID_CH1) >> 2;
}

//------------------------------------------------------------------------------
static unsigned msTimeToReset;
void Bayang_tx_ui()
{
    console_clear();
    debug("TLMx: ");
    debug_put_uint16((gTXContext.tlmCount * 100) / gTXContext.tlmExpected);
    debug_put_newline();

    debug("tx: ");
    debug_put_uint16(gTXContext.txSendTime);
    debug(" rx: ");
    debug_put_uint16(gTXContext.telemetryRxTime);
    debug_put_newline();
    
    debug("bat: ");
    debug_put_uint16(gSilverLiteData.vbattComp);
    debug(" pps: ");
    debug_put_uint16(gSilverLiteData.pktsPerSec);
    debug_put_newline();

    debug("reset: ");
    debug_put_uint16(gTXContext.resetTime);
    debug_put_newline();

    debug_flush();

    // Holding bind button down for 3 seconds will reset the TX
    if (button_toggled(kBtn_Bind))
    {
        // If toggled active then set reset time, otherwise reset it
        msTimeToReset = button_active(kBtn_Bind) ? millis_this_frame() + 3000 : 0;
        PRINTF("Bayang msTimeToReset: %d", msTimeToReset);
    }
    else if (msTimeToReset)
    {
        if (millis_this_frame() >= msTimeToReset)
        {
            msTimeToReset = 0;
            Bayang_tx_reset(kBayangSilverware, BAYANG_OPTION_FLAG_TELEMETRY);
            PRINTF("Bayang reset");
        }
    }
}

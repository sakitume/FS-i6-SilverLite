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
#include "fsl_debug_console.h"

#include <string.h>
#include "drv_nRF24L01.h"
#include "drv_softspi.h"
#include "drv_time.h"
#include "timer.h"
#include "bayang_common.h"
#include "bayang.h"

//------------------------------------------------------------------------------
// Compatibility
#define     _BV(a)  (1 << (a))

//------------------------------------------------------------------------------
// Forward declarations

//------------------------------------------------------------------------------
uint8_t Bayang_rf_chan;
uint8_t Bayang_rf_channels[BAYANG_RF_NUM_CHANNELS] = {0,};
uint8_t Bayang_rx_tx_addr[BAYANG_ADDRESS_LENGTH];

uint8_t packet[BAYANG_PACKET_SIZE];
uint8_t txPkt_Flags2;       // Used for packet[2]
uint8_t txPkt_Flags3;       // Used for packet[3]
uint16_t txPkt_Sticks[4];   // in AETR order
uint16_t txPkt_AuxAnalog[2];    // auxiliary analog channels

//------------------------------------------------------------------------------
uint8_t Bayang_checksum()
{
    uint8_t sum = packet[0];
    for (uint8_t i = 1; i < BAYANG_PACKET_SIZE - 1; i++)
        sum += packet[i];
    return sum;
}

//------------------------------------------------------------------------------
static bool initOnce;
void Bayang_init()
{
    if (initOnce)
    {
        return;
    }
    initOnce = true;

    // Ensure GPIO pins to/from nRF24L01 module are configured
    NRF24L01_InitGPIO();

    // Ensure soft SPI GPIO pins are configured
    spi_init();

    // Add our ISR handler that expects to be called every millisecond
    extern void Bayang_tx_isr(unsigned long millis);
    timer_add_callback(Bayang_tx_isr);
}

//------------------------------------------------------------------------------
void Bayang_disable()
{
    // Ensure ISR stops running
    gTXContext.protocol = kDisabled;

    // Turn off nRF24L01
    NRF24L01_SetTxRxMode(TXRX_OFF);
}

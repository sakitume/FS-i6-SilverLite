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
#include "drv_XN297_emu.h"
#include "drv_time.h"
#include "bayang_common.h"
#include "bayang.h"

//------------------------------------------------------------------------------
// Compatibility
#define     _BV(a)  (1 << (a))

//------------------------------------------------------------------------------
static uint8_t bind_count;
static uint8_t bind_packet[BAYANG_PACKET_SIZE] = {0};

//------------------------------------------------------------------------------
static void Bayang_rx_bind_init()
{
    // Be sure to flush rx fifo
    NRF24L01_FlushRx();
    bind_count = 0;
}

//------------------------------------------------------------------------------
static bool Bayang_rx_bind()
{
    // If data received from tx
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
    {
        XN297_ReadPayload(packet, BAYANG_PACKET_SIZE);

        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
        NRF24L01_FlushRx();

        PRINTF("Bayang_bind_rx(): packet[0]: 0x%0X\n", packet[0]);

        if (packet[14] != Bayang_checksum())
        {
            PRINTF("bad checksum\n");
        }
        else if ((packet[0] == 0xA4) || (packet[0] == 0xA3)) // A3==PROTO_BAYANG_SILVERWARE
        {
            if (0 == bind_count)
            {
                PRINTF("Bayang_bind_rx(): Received first bind packet\n");
                memcpy(bind_packet, packet, BAYANG_PACKET_SIZE);
                bind_count++;
            }
            else
            {
                if (0 == memcmp(bind_packet, packet, BAYANG_PACKET_SIZE))
                {
                    bind_count++;
                    if (bind_count >= 10)
                    {
                        PRINTF("Bayang_rx_bind(): Successfully bound");
                        memcpy(Bayang_rx_tx_addr, &packet[1], 5);
                        memcpy(Bayang_rf_channels, &packet[6], 4);

                        XN297_SetTXAddr(Bayang_rx_tx_addr, BAYANG_ADDRESS_LENGTH);
                        XN297_SetRXAddr(Bayang_rx_tx_addr, BAYANG_ADDRESS_LENGTH);

                        NRF24L01_WriteReg(NRF24L01_05_RF_CH, Bayang_rf_channels[Bayang_rf_chan++]);
                        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
                        NRF24L01_FlushRx();

                        return true;
                    }
                }
            }
        }
    }

    // Not yet bound
    return false;
}

//------------------------------------------------------------------------------
static void Bayang_rx_packet()
{
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
    {
        XN297_ReadPayload(packet, BAYANG_PACKET_SIZE);

        if ((packet[0] == 0xA4) || (packet[0] == 0xA3)) // A3==PROTO_BAYANG_SILVERWARE
        {
            // bind packet, ignore it
        }
        else if (packet[0] == 0xA5)
        {
            // data packet
            if (packet[14] != Bayang_checksum())
            {
                //checksum FAIL
            }
            else
            {
                uint16_t roll, pitch, yaw, throttle;
                roll    = ((uint16_t)(packet[4] & 0x0003) << 8) | packet[5];
                pitch   = ((uint16_t)(packet[6] & 0x0003) << 8) | packet[7];
                yaw     = ((uint16_t)(packet[10] & 0x0003) << 8) | packet[11];
                throttle= ((uint16_t)(packet[8] & 0x0003) << 8) | packet[9];
                static uint32_t lastRX;
                uint32_t period = micros_this_frame() - lastRX;
                lastRX = micros_this_frame();
                PRINTF("%02x %02x : %03x %03x %03x %03x - %02x %02x %02x : %d\n", packet[1], packet[2], roll, pitch, yaw, throttle, packet[4] & ~3, packet[6] & ~3, packet[10] & ~3, period);
            }
        }
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, Bayang_rf_channels[Bayang_rf_chan++]);
        Bayang_rf_chan %= sizeof(Bayang_rf_channels);
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
        NRF24L01_FlushRx();
    }
}

//------------------------------------------------------------------------------
enum e_RXState
{
    kRXInitNeeded,
    kRXWaitForBind,
    kRXReady
};
static e_RXState  rxState = kRXInitNeeded;
static uint32_t lastSend;

//------------------------------------------------------------------------------
static void Bayang_rx_test_init()
{
    Bayang_init(0);
    Bayang_rx_bind_init();
    rxState = kRXWaitForBind;
    PRINTF("Waiting to bind to TX\n");
}

//------------------------------------------------------------------------------
// Call this last on every loop update.
void Bayang_rx_test()
{
    if (rxState == kRXInitNeeded)
    {
        Bayang_rx_test_init();
    }

    // How much longer until we should send out another packet
    enum { period = 200 };
    long wait = period - (long)(micros_realtime() - lastSend);

    // If enough time to perform another loop update then return
    enum { maxLoopUpdateTime = 2000 };  // TODO: Need to figure this out
    if (wait > maxLoopUpdateTime)
    {
        return;
    }

    // Otherwise we'll wait here until its time to send out another packet
    // (or receive a bind packet)
    if (wait > 0)
    {
        delay_us(wait);
    }

    // 
    lastSend = micros_realtime();
    if (rxState == kRXWaitForBind)
    {
        if (Bayang_rx_bind())
        {
            rxState = kRXReady;
        }
    }
    else
    {
        Bayang_rx_packet();
    }
}

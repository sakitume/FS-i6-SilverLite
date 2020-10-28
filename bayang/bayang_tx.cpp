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
#include <string.h>
#include "fsl_pit.h"

#include "drv_nRF24L01.h"
#include "drv_XN297_emu.h"
#include "drv_time.h"
#include "bayang_common.h"
#include "bayang.h"
#include "silverlite_data.h"

//------------------------------------------------------------------------------
struct TXContext_t gTXContext;

//------------------------------------------------------------------------------
// Forward declarations
static void SendBindPacket();
static bool CheckForTelemetry();
static void SendTXPacket();
static void ResetProtocol();
static void HandleBayangPacket(const uint8_t* packet);

//------------------------------------------------------------------------------
// Compatibility
#define     _BV(a)  (1 << (a))

//------------------------------------------------------------------------------
static unsigned long gTotalMicros;
//------------------------------------------------------------------------------
// This must be called at least every 2 seconds to avoid underflow of SysTick
// counter
static unsigned long micros(void)
{
    unsigned long maxticks = SysTick->LOAD;
    unsigned long ticks = SysTick->VAL;
    unsigned long quotient;
    unsigned long elapsedticks;
    static unsigned long remainder = 0; // carry forward the remainder ticks;
    static unsigned long lastticks;

    if (ticks < lastticks)
    {
        elapsedticks = lastticks - ticks;
    }
    else
    {
        // overflow ( underflow really)
        elapsedticks = lastticks + (maxticks - ticks);
    }

    lastticks = ticks;
    elapsedticks += remainder;

#if 0
    quotient = elapsedticks / 3;
    remainder = elapsedticks - quotient * 3;
    gTotalMicros = gTotalMicros + quotient;
#else
    // faster divide by 3
    quotient = elapsedticks * (43691*2) >> 18;
    remainder = elapsedticks - quotient * 3;
    gTotalMicros = gTotalMicros + quotient;
#endif
    return gTotalMicros;
}

//------------------------------------------------------------------------------
//
void Bayang_tx_isr(unsigned long /*millis*/)
{
    uint32_t us_now = micros();

    if (gTXContext.resetToProtocol != kBayangDisabled)
    {
        ResetProtocol();    // takes about 727us
        gTXContext.resetTime = micros() - us_now;
    }
    else if (gTXContext.protocol == kBayangDisabled)
    {
        return;
    }
    else
    {
        // If still in the binding phase
        if (gTXContext.bindCounter)
        {
            SendBindPacket();
        }
        else
        {
            if (gTXContext.telemetryTimeout)
            {
                gTXContext.telemetryTimeout--;
            }

            // If still waiting for telemetry and we're not scheduled
            // to send a TX packet at this time
            if (gTXContext.awaitingTelemetry && (gTXContext.intervalCounter > 1))
            {
                uint32_t now = micros();
                if (CheckForTelemetry())
                {
                    gTXContext.telemetryRxTime = micros() - now;    // Usually around 200us
                    gTXContext.awaitingTelemetry = false;
                    gTXContext.tlmCount++;

                    // Reload telemetry timout
                    gTXContext.telemetryTimeout = gTXContext.telemetryTimeoutLoad;
                }
            }

            // If time to send next packet
            if (--gTXContext.intervalCounter <= 0)
            {
                uint32_t now = micros();
                SendTXPacket();
                gTXContext.txSendTime = micros() - now; // Usually around 892us

                // Every half second
                if (gTXContext.tlmExpected >= 100)
                {
                    gTXContext.tlmRatio = (200 * gTXContext.tlmCount) / gTXContext.tlmExpected;
                    gTXContext.tlmCount = 0;
                    gTXContext.tlmExpected = 0;
                }
                gTXContext.txSendCount++;
                gTXContext.tlmExpected++;
                gTXContext.awaitingTelemetry = gTXContext.expectTelemetry;
                gTXContext.intervalCounter = gTXContext.sendInterval;
            }
        }

        uint32_t deltaTime = micros() - us_now;
        if (deltaTime > gTXContext.irqTime)
        {
            gTXContext.irqTime = deltaTime;
        }
        gTXContext.irqHits++;
    }
}

//------------------------------------------------------------------------------
static void SendBindPacket()
{
    // 0xA4 = Stock Bayang or Silverware with no telemetry
    // 0xA3 = Silverware with telemetry
    // 0xA1 = Silverware with telemetry, with 2 aux analog channels
    // 0xA2 = Silverware no telemetry, with 2 aux analog channels
    if (gTXContext.protocol == kBayangStock)
    {
        packet[0]= 0xA4;	// Stock Bayang
    }
    else //..protocol is kBayangSilverware
    {
        const uint8_t option = gTXContext.options;
        if (option & (BAYANG_OPTION_FLAG_TELEMETRY | BAYANG_OPTION_FLAG_SILVERLITE))
        {
            if (option & BAYANG_OPTION_FLAG_ANALOGAUX)
                packet[0]= 0xA1;	// Silverware with telemetry, with 2 aux analog channels
            else
                packet[0]= 0xA3;    // Silverware with telemetry
        }
        else if(option & BAYANG_OPTION_FLAG_ANALOGAUX)
        {
            packet[0]= 0xA2;    // Silverware no telemetry, with 2 aux analog channels
        }
        else
        {
            packet[0]= 0xA4;	// Silverware with no telemetry
        }
    }

    memcpy(&packet[1], Bayang_rx_tx_addr, 5);
    memcpy(&packet[6], Bayang_rf_channels, 4);

    if ((gTXContext.options & BAYANG_OPTION_FLAG_SILVERLITE) && (gTXContext.protocol == kBayangSilverware))
    {
        // Use special magic bytes to indicate to flight controller that this
        // TX supports SilverLite extension.
        //
        // SilverLite feature will not be enabled until receiver replies
        // back (via telemetry) to enable SilverLite
        //
        // Note: We can still successfully bind and operate a stock BWhoop B03 Pro with this change
        //
        packet[10] = packet[1] ^ 0xAA;
        packet[11] = packet[2] ^ 0xAA;
    }
    else
    {
        packet[10] = Bayang_rx_tx_addr[0];
        packet[11] = Bayang_rx_tx_addr[1];
    }

    packet[12] = Bayang_rx_tx_addr[2];
    packet[13] = 0x0A;
    packet[14] = Bayang_checksum();
    
    NRF24L01_SetTxRxMode(TX_EN);
    XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, BAYANG_RF_BIND_CHANNEL);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet, BAYANG_PACKET_SIZE);

    // If this is the last bind packet to send out
    if (--gTXContext.bindCounter <= 0)
    {
        XN297_SetTXAddr(Bayang_rx_tx_addr, BAYANG_ADDRESS_LENGTH);
        XN297_SetRXAddr(Bayang_rx_tx_addr, BAYANG_ADDRESS_LENGTH);
    }
}

//------------------------------------------------------------------------------
// Based on: https://github.com/goebish/nrf24_multipro/blob/master/nRF24_multipro/Bayang.ino
// And: https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/blob/master/Multiprotocol/Bayang_nrf24l01.ino
static void SendTXPacket()
{
    packet[0] = 0xA5;   // 0xA5 means normal TX packet (not bind packet)

    const uint8_t bSendAuxAnalog = (gTXContext.protocol == kBayangSilverware) && (gTXContext.options & BAYANG_OPTION_FLAG_ANALOGAUX);
    if (bSendAuxAnalog)
    {
        packet[1] = txPkt_AuxAnalog[0];
    }
    else
    {
        packet[1] = 0xFA;   // For BWhoop B03 Pro: slow == 0xF4, normal = 0xF7, fast = 0xFA
    }

    packet[2] = txPkt_Flags2;
    packet[3] = txPkt_Flags3;

    const uint8_t *bytes = (uint8_t*)txPkt_Sticks;
    packet[4] = bytes[1] | 0x7C;    // 2 most signicant bits, and rest of the bits in this byte are set
    packet[5] = bytes[0];           // 8 least significant bits

    packet[6] = bytes[3] | 0x7C;
    packet[7] = bytes[2];

    packet[8] = bytes[5] | 0x7C;
    packet[9] = bytes[4];

    packet[10] = bytes[7] | 0x7C;
    packet[11] = bytes[6];

    // The values for bytes[12], bytes[13] depends on the sub protocol
    // If silverLite is desired then we use byte 12 to indicate we are silverLite capable
    // Note: The magic bytes we sent during the bind phase are never seen if the flight controller
    // was using auto-bind, that is why we still do this here
    const uint8_t bSendSilverLiteFlag = (gTXContext.protocol == kBayangSilverware) && (gTXContext.options & BAYANG_OPTION_FLAG_SILVERLITE);
    if (bSendAuxAnalog)
    {
        packet[12] = bSendSilverLiteFlag ? Bayang_rx_tx_addr[2] ^ 0xAA : Bayang_rx_tx_addr[2];
        packet[13] = txPkt_AuxAnalog[1];
    }
    else
    {
        packet[12] = bSendSilverLiteFlag ? Bayang_rx_tx_addr[2] ^ 0xAA : Bayang_rx_tx_addr[2];
        packet[13] = 0x0A;
    }


    // Last in the packet is the checksum
    packet[14] = Bayang_checksum();
    
    NRF24L01_SetTxRxMode(TX_EN);
    XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, Bayang_rf_channels[Bayang_rf_chan++]);
    Bayang_rf_chan %= sizeof(Bayang_rf_channels);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet, BAYANG_PACKET_SIZE);

    if (gTXContext.expectTelemetry)
    {
        // Wait until TX completes so we can switch radio to RX
		while (!(NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_TX_DS)))
            ;
		NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x03);
    }
}

//------------------------------------------------------------------------------
static bool CheckForTelemetry()
{
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR)) 
    {
        XN297_ReadPayload(packet, BAYANG_PACKET_SIZE);
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0xFF);
        NRF24L01_FlushRx();

        if (packet[14] == Bayang_checksum())
        {
            if (packet[0] == 0x85)
            {
                HandleBayangPacket(packet);
                return true;
            }

            // SilverLite packets are in the range of 0xA0 to 0xAF inclusive
            if ((packet[0] >= 0xA0) && (packet[0] <= 0xAF))
            {
                gTXContext.silverLiteEnabled = true;
                HandleSilverLitePacket(packet);
                return true;
            }
        }
    }
    return false;
}

//------------------------------------------------------------------------------
static void HandleBayangPacket(const uint8_t* packet)
{
    gSilverLiteData.lastUpdate = millis_this_frame();
    gSilverLiteData.tlmCount++;

    gSilverLiteData.vbattFilt   = ((packet[3] & 0x7) << 8) | packet[4];
    gSilverLiteData.vbattComp   = ((packet[5] & 0x7) << 8) | packet[6];
    gSilverLiteData.pktsPerSec  = packet[7];

    // bit 3 of packet[3] will be set if low battery. 
    gSilverLiteData.flags =  (packet[3] & (1 << 3)) ? SilverLiteData_t::kLowBattery : 0;
}


#if 0
//------------------------------------------------------------------------------
static void StopTimer()
{
    PIT_StopTimer(PIT, kPIT_Chnl_0);
}
#endif

//------------------------------------------------------------------------------
// TODO: Extend this so caller can specify subprotocol
static void ResetProtocol()
{
    gTXContext.protocol = gTXContext.resetToProtocol;
    gTXContext.options  = gTXContext.resetToOptions;
    gTXContext.resetToProtocol  = kBayangDisabled;
    gTXContext.resetToOptions   = 0;

    // We must first send out 1000 bind packets before sending any TX packets
    gTXContext.bindCounter = 1000;

    gTXContext.expectTelemetry = false;
    gTXContext.awaitingTelemetry = false;
    gTXContext.intervalCounter = 0;

    gTXContext.txSendCount = 0;
    gTXContext.tlmRatio = 0;
    gTXContext.tlmCount = 0;
    gTXContext.tlmExpected = 0;
    gTXContext.txSendTime = 0;
    gTXContext.telemetryRxTime = 0;
    gTXContext.irqTime = 0;
    gTXContext.irqHits = 0;

    gTXContext.telemetryTimeout = 0;
    gTXContext.telemetryTimeoutLoad = 500;

    gTXContext.silverLiteEnabled = false;

    // Different subprotocols expect to send packets at a different period
    // and may also send back telemetry
    switch (gTXContext.protocol)
    {
        case kBayangStock:
            gTXContext.sendInterval = 2;
            break;
        case kBayangSilverware:
            if (gTXContext.options & (BAYANG_OPTION_FLAG_TELEMETRY | BAYANG_OPTION_FLAG_SILVERLITE))
            {
                gTXContext.sendInterval = 5;
                gTXContext.expectTelemetry = true;
            }
            else
            {
                gTXContext.sendInterval = 3;
            }
            break;
    }

    uint8_t txID[4] = { 0xDC, 0xCB, 0x07, 0x0B };
    Bayang_init(txID);
}

//------------------------------------------------------------------------------
// NOTE: This will be called by a timer interrupt so care must be taken to
// ensure this function (and anything it may call) is thread safe
void Bayang_init(const uint8_t *transmitterID = nullptr)
{
    NRF24L01_Reset();

    uint8_t i;
    const uint8_t bind_address[] = {0,0,0,0,0};
    if (transmitterID)
    {
        memcpy(Bayang_rx_tx_addr, transmitterID, 4);
    }
    Bayang_rx_tx_addr[4] = Bayang_rx_tx_addr[0] ^ 0xff;

    Bayang_rf_channels[0] = 0x00;
    for(i=1; i<BAYANG_RF_NUM_CHANNELS; i++) 
    {
        Bayang_rf_channels[i] = Bayang_rx_tx_addr[i] % 0x42;
    }

    NRF24L01_Initialize();

    if (transmitterID)
    {
        NRF24L01_SetTxRxMode(TX_EN);
    }
    else
    {
        NRF24L01_SetTxRxMode(RX_EN);
    }

    XN297_SetTXAddr(bind_address, BAYANG_ADDRESS_LENGTH);
    XN297_SetRXAddr(bind_address, BAYANG_ADDRESS_LENGTH);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, BAYANG_PACKET_SIZE);
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps

//XXX, TODO: At higher power levels I'm having great difficulty sending out a clean signal
// My two receivers (one a silverware flight controller the other an nrf24l01 board) aren't 
// receiving, or are getting bad checksums.
//
// Placing my hand near the antenna or the wire leads going to the nrf24l01 board make
// the issue go away, or get worse depending on where I do this
//
// Where I currently have the board installed I seem to have the best luck (freestanding, not
// touching or being near the antenna or board) with TX_POWER_5mW
//
// UPDATE: Shielding the module by simply wrapping it up in an anti-static bag made a huge
// improvement, mostly removing the dropouts (and bad crc).
//

//    NRF24L01_SetPower(TX_POWER_5mW);       // TODO: Should we use TX_POWER_80mW? Or a different TX_Power value?
//    NRF24L01_SetPower(TX_POWER_20mW);       // TODO: Should we use TX_POWER_80mW? Or a different TX_Power value?
    NRF24L01_SetPower(TX_POWER_80mW);       // TODO: Should we use TX_POWER_80mW? Or a different TX_Power value?
//    NRF24L01_SetPower(TX_POWER_158mW);      // TODO: Should we use TX_POWER_80mW? Or a different TX_Power value?

    NRF24L01_Activate(0x73);                         // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);      // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
    NRF24L01_Activate(0x73);
    delay_us(150);

    if (!transmitterID)
    {
        XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP) | _BV(NRF24L01_00_PRIM_RX));
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
        NRF24L01_FlushRx();

        NRF24L01_WriteReg(NRF24L01_05_RF_CH, BAYANG_RF_BIND_CHANNEL);
    }
}


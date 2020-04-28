#include "MKL16Z4.h"
//#include "fsl_common.h"
//#include "fsl_dma.h"
//#include "fsl_dmamux.h"
#include "fsl_uart.h"
#include "multiprotocol.h"
#include "multiprotocol_enums.h"
#include "drv_time.h"

#include <stdio.h>  // printf
#include "console.h"
#include "debug.h"
#include "adc.h"
#include "buttons.h"
#include "storage.h"
#include "silverlite_data.h"
#include "switchID.h"

#define MPM_UART UART2
#define MPM_UART_CLK_FREQ CLOCK_GetFreq(BUS_CLK)

// Note: Due to how UART_TransferStartRingBuffer() is implemented, the
// number of bytes actually available/stored into the ring buffer is one
// less than the value of RX_RING_BUFFER_SIZE
#define RX_RING_BUFFER_SIZE (64+1)

static uart_handle_t g_uartHandle;
static uint8_t g_rxRingBuffer[RX_RING_BUFFER_SIZE] = {0}; /* RX ring buffer. */


#define RX_BUFFER_SIZE  256
static uint8_t g_rxBuffer[RX_BUFFER_SIZE] = {0}; // Buffer for telemetry receive data
static uart_transfer_t receiveXfer;

#define TX_BUFFER_SIZE  36  // Total of 26 bytes for protocol V1, variable length 27..36 for protocol V2
static uint8_t g_txBuffer[TX_BUFFER_SIZE] = {0};
static uart_transfer_t sendXfer;

static volatile bool rxBufferEmpty = true;
static volatile bool txOnGoing = false;
static volatile bool rxOnGoing = false;

static bool bWaitingForTLMHeader;
static uint8_t telemetryType;
static uint8_t expectedTLMDataLength;

static uint8_t multi4in1_bind = 0;
static uint8_t multi4in1_range_check = 0;

static int telemetryFrame[17];

static bool bTXEnabled = false;

#define MULTI_CHANS 16
#define MULTI_CHAN_BITS 11

extern void HandleSilverLitePacket(const uint8_t* packet);

//--[ Forward Declarations] ----------------------------------------------------
static void ProcessTelemetry(uint8_t telemetryType, const uint8_t* data, uint8_t dataLen);
static void mapSwitchesToMPMChans(uint16_t adc_data[], const uint8_t *mapping);


//------------------------------------------------------------------------------
static void UART_UserCallback(UART_Type *base, uart_handle_t *handle, status_t status, void *userData)
{
    userData = userData;

    if (kStatus_UART_TxIdle == status)
    {
        txOnGoing = false;
    }

    if (kStatus_UART_RxIdle == status)
    {
        rxBufferEmpty = false;
        rxOnGoing = false;
    }
}

//------------------------------------------------------------------------------
static inline int constrain(int amt, int low, int high) 
{
    if (amt < low)
        return low;
    else if (amt > high)
        return high;
    else
        return amt;
}

//------------------------------------------------------------------------------
void multiprotocol_update(void)
{
    if (!bTXEnabled)
    {
        return;
    }
    for (;;)
    {
        // If RX is idle and g_rxBuffer is empty, start to read data to g_rxBuffer
        if ((!rxOnGoing) && rxBufferEmpty)
        {
            rxOnGoing = true;

            // Header is 4 bytes
            const size_t kNumBytesToReceive = bWaitingForTLMHeader ? 4 : expectedTLMDataLength;
            receiveXfer.dataSize = kNumBytesToReceive;

            size_t receivedBytes;
            UART_TransferReceiveNonBlocking(MPM_UART, &g_uartHandle, &receiveXfer, &receivedBytes);
            if (kNumBytesToReceive == receivedBytes)
            {
                rxBufferEmpty = false;
                rxOnGoing = false;
            }
        }

        if (!rxBufferEmpty)
        {
            rxBufferEmpty = true;

            //  Format: header (4 byte) + data (variable)
            //     [0] = 'M' (0x4d)
            //     [1] = 'P' (0x50)
            //  
            //     The first byte is deliberatly chosen to be different from other telemetry protocols
            //     (e.g. 0xAA for DSM/Multi, 0xAA for FlySky and 0x7e for Frsky) to allow a TX to detect
            //     the telemetry format of older versions
            //  
            //     [2] Type (see below)
            //     [3] Length (excluding the 4 header bytes)

            // If we were waiting for telemetry header
            if (bWaitingForTLMHeader)
            {
                if ((g_rxBuffer[0] != 'M') || (g_rxBuffer[1] != 'P'))
                {
                    // This is bad
//                    printf("!\r\n");
                    debug("Bad TLM");
                    debug_put_newline();

                    telemetryType = 0;
                    expectedTLMDataLength = 0;
                }
                else
                {
                    telemetryType = g_rxBuffer[2];
                    expectedTLMDataLength = g_rxBuffer[3];

                    // If telemetry data is of zero length, then we'll wait for next
                    // header and can immediately process the data
                    if (expectedTLMDataLength == 0)
                    {
                        ProcessTelemetry(telemetryType, &g_rxBuffer[4], 0);
                        telemetryType = 0;
                        expectedTLMDataLength = 0;

                        // Prepare for next RX
                        bWaitingForTLMHeader = true;
                    }
                    else    // ...otherwise we'll wait for the data
                    {
                        // Prepare for next RX
                        bWaitingForTLMHeader = false;
                    }
                }
            }
            else // ..we were waiting for data, which we now have
            {
                ProcessTelemetry(telemetryType, &g_rxBuffer[0], expectedTLMDataLength);
                telemetryType = 0;
                expectedTLMDataLength = 0;

                // Prepare for next RX
                bWaitingForTLMHeader = true;
            }
        }
        else
        {
            break;
        }
    }

    // If TX is idle
    if (!txOnGoing)
    {
        // If we have data ready for transmission
        if (sendXfer.dataSize)
        {
            // Send the data
            txOnGoing = true;
            UART_TransferSendNonBlocking(MPM_UART, &g_uartHandle, &sendXfer);
            sendXfer.dataSize = 0;
        }
    }

    // If TX is idle
    if (!txOnGoing)
    {
#if 1        
        bool bTimeToSend = false;
        uint32_t    now = micros_this_frame();
        static uint32_t lastTX = 0;
        if ((now - lastTX) >= 2000)
        {
            bTimeToSend = true;
        }

        if (bTimeToSend)
        {
            lastTX = now;
#else
        if (bTXEnabled)
        {
#endif        

            uint8_t *p = sendXfer.data;
            uint8_t data;

            // Get current model so we can use its multiprotocol config
            const ModelDesc_t &model = storage.model[storage.current_model];

#if 0   

From Protocol_Details.md (Multiprotocol repo):

```
## BAYANG - *14*
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|-|-|CH14|CH15
---|---|---|---|---|---|---|---|---|----|----|---|---|----|----
A|E|T|R|FLIP|RTH|PICTURE|VIDEO|HEADLESS|INVERTED|RATES|-|-|ANAAUX1|ANAAUX2

RATES: -100%(default)=>higher rates by enabling dynamic trims (except for Headless), 100%=>disable dynamic trims

Channels 14 and 15 (ANAAUX1 and ANAAUX2) only available with analog aux channel option, indicated below.

### Sub_protocol BAYANG - *0*
Models: Eachine H8(C) mini, BayangToys X6/X7/X9, JJRC JJ850, Floureon H101 ...

Option=0 -> normal Bayang protocol

Option=1 -> enable telemetry with [Silverxxx firmware](https://github.com/silver13/H101-acro/tree/master). Value returned to the TX using FrSkyD Hub are RX RSSI, TX RSSI, A1=uncompensated battery voltage, A2=compensated battery voltage

Option=2 -> enable analog aux channels with [NFE Silverware firmware](https://github.com/NotFastEnuf/NFE_Silverware). Two otherwise static bytes in the protocol overridden to add two 'analog' (non-binary) auxiliary channels.

Option=3 -> both Silverware telemetry and analog aux channels enabled.

```




Bayang send packet code from multiprotocol module source code
CH5_SW thru CH13_SW is used. These are defined as:
#define CH5_SW	(Channel_AUX & _BV(0))      -> BAYANG_FLAG_FLIP
#define CH6_SW	(Channel_AUX & _BV(1))      -> BAYANG_FLAG_RTH
#define CH7_SW	(Channel_AUX & _BV(2))      -> BAYANG_FLAG_PICTURE
#define CH8_SW	(Channel_AUX & _BV(3))      -> BAYANG_FLAG_VIDEO
#define CH9_SW	(Channel_AUX & _BV(4))      -> BAYANG_FLAG_HEADLESS
#define CH10_SW	(Channel_AUX & _BV(5))      -> BAYANG_FLAG_INVERTED
#define CH11_SW	(Channel_AUX & _BV(6))      -> dyntrim = 0
#define CH12_SW	(Channel_AUX & _BV(7))      -> BAYANG_FLAG_TAKE_OFF
#define CH13_SW	(Channel_data[CH13]>CHANNEL_SWITCH) -> BAYANG_FLAG_EMG_STOP


Channel_AUX is initialized from channel data as follows:

	//Calc AUX flags
	Channel_AUX=0;
	for(uint8_t i=0;i<8;i++)
		if(Channel_data[CH5+i]>CHANNEL_SWITCH)
			Channel_AUX|=1<<i;

> Note: CHANNEL_SWITCH is 1104

// Also, I'm using these settings when I build SilverWare NFE
//
//#define ARMING CHAN_5     BAYANG_FLAG_INVERT (packet[3]), SwA
//#define IDLE_UP CHAN_9    BAYANG_FLAG_HEADLESS (packet[2]), SwD
//#define LEVELMODE CHAN_6  BAYANG_FLAG_FLIP (packet[2]), SwB: Position 1
//#define RACEMODE  CHAN_7  BAYANG_FLAG_SNAPSHOT (packet[2]), SwC: Position 2
//#define HORIZON   CHAN_8  BAYANG_FLAG_VIDEO (packet[2]), SwC: Position 3

// ADC_ID_SwB == SwB: Position 1 == <100(46), Position 2 == >923(1023)
// ADC_ID_SwC == SwC: Position 1 == 0, Position 2 == 512, Position 3 == 1023


		//Flags packet[2]
		packet[2] = 0x00;
		if(CH5_SW)
			packet[2] = BAYANG_FLAG_FLIP;
		if(CH6_SW)
			packet[2] |= BAYANG_FLAG_RTH;
		if(CH7_SW)
			packet[2] |= BAYANG_FLAG_PICTURE;
		if(CH8_SW)
			packet[2] |= BAYANG_FLAG_VIDEO;
		if(CH9_SW)
		{
			packet[2] |= BAYANG_FLAG_HEADLESS;
			dyntrim = 0;
		}
		//Flags packet[3]
		packet[3] = 0x00;
		if(CH10_SW)
			packet[3] = BAYANG_FLAG_INVERTED;
		if(CH11_SW)
			dyntrim = 0;
		if(CH12_SW)
		  packet[3] |= BAYANG_FLAG_TAKE_OFF;
		if(CH13_SW)
			packet[3] |= BAYANG_FLAG_EMG_STOP;


So something like this will be needed

    //#define ARMING CHAN_5     BAYANG_FLAG_INVERT (packet[3]), SwA
    const int CH10_SW = 9;
    adc_data[CH10_SW]= button_active(kBtn_SwA) ? 4095 : 0;

    //#define IDLE_UP CHAN_9    BAYANG_FLAG_HEADLESS (packet[2]), SwD
    const int CH9_SW = 8;
    adc_data[CH9_SW]= button_active(kBtn_SwD) ? 4095 : 0;

    //#define LEVELMODE CHAN_6  BAYANG_FLAG_FLIP (packet[2]), SwB: Position 1
    const int CH5_SW = 4;
    adc_data[CH5_SW]= (adc_get_channel_calibrated(ADC_ID_SwB) < 100) ? 4095 : 0;

    //#define RACEMODE  CHAN_7  BAYANG_FLAG_SNAPSHOT (packet[2]), SwC: Position 2
    const uint16_t SwC = adc_get_channel_calibrated(ADC_ID_SwC);
    const int CH7_SW = 6;
    adc_data[CH7_SW]= (SwC >= 412) ? 4095 : 0;

    //#define HORIZON   CHAN_8  BAYANG_FLAG_VIDEO (packet[2]), SwC: Position 3
    const int CH8_SW = 7;
    adc_data[CH8_SW]= (SwC >= 923) ? 4095 : 0;

    // Disable dyntrim (because I don't know what it does exactly and it seems wrong to have it on based on code in Silverware)
    const int CH11_SW = 10;
    adc_data[CH11_SW] = 4095;

#endif            

            // Header
            *p++ = model.mpm_protocol < 32 ? 0x55 : 0x54;

            // Stream[1] = sub_protocol|RangeCheckBit|AutoBindBit|BindBit;
            data = model.mpm_protocol & 0x1f;
            data |= multi4in1_range_check << 5;
            data |= model.mpm_auto_bind << 6;
            if (multi4in1_bind)
            {
                multi4in1_bind--;
                data |= 1 << 7;
            }
            *p++ = data;

            // Stream[2] = RxNum | Type | Power;
            data = model.mpm_rx_num & 0x0f;
            data |= (model.mpm_sub_protocol & 0x07) << 4;
            data |= model.mpm_low_power << 7;
            *p++ = data;

            // Stream[3] = option_protocol;
            *p++ = model.mpm_option;

            // Stream[4] to [25] = Channels
            uint32_t bits = 0;
            uint8_t bitsavailable = 0;

            // FlySky-i6 can support 10 channels, 8 are true analog, 2 are digital (on/off):
            //  4 adc channels (sticks): ADC_ID_AILERON, ADC_ID_ELEVATION, ADC_ID_THROTTLE, ADC_ID_RUDDER
            //  2 adc channels (VrA, VrB): ADC_ID_CH0, ADC_ID_CH1
            //  2 adc channels (SwB, SwC): ADC_ID_SwB, ADC_ID_SwC
            //  2 digital channels (SwA, SwD)
            //
            // Multiprotocol module expects 16 channels of input data
            uint16_t adc_data[MULTI_CHANS];

            // Populate first 4 mpm channels with AETR data
            int ch;
            for (ch = ADC_ID_AILERON; ch <= ADC_ID_RUDDER; ch++) 
            {
                // ADC value will be between 0x0 to 0xFFF inclusive
                int value = adc_get_channel_calibrated_unscaled(ch);
                adc_data[ch] = value;
            }

            if (model.mpm_protocol == kBayangProtocol)
            {
                // Mapping switches to Bayang channels, default MPM channels to 0
                // then call mapSwitchesToMPMChans() to honor the model's switch mapping
                for (; ch < MULTI_CHANS; ch++)
                {
                    adc_data[ch] = 0;
                }
                mapSwitchesToMPMChans(adc_data, model.bayangChans);
            }
            else
            {
                // Copy rest of our ADC channels to MPM channels
                //  2 adc channels (VrA, VrB): ADC_ID_CH0, ADC_ID_CH1
                //  2 adc channels (SwB, SwC): ADC_ID_SwB, ADC_ID_SwC
                for (; ch <= ADC_ID_SwC; ch++) 
                {
                    // ADC value will be between 0x0 to 0xFFF inclusive
                    int value = adc_get_channel_calibrated_unscaled(ch);
                    adc_data[ch] = value;
                }

                // Then copy SwA and SwB states
                adc_data[ch++] = button_active(kBtn_SwA) ? 4095 : 0;
                adc_data[ch++] = button_active(kBtn_SwD) ? 4095 : 0;

                // Zero out the remaining
                for (; ch < MULTI_CHANS; ch++)
                {
                    adc_data[ch] = 0;
                }
            }

            // Multiprotocol module expects 16 channels to be provided
            const uint16_t SwC = adc_get_channel_calibrated(ADC_ID_SwC);
            for (int i = 0; i < MULTI_CHANS; i++) 
            {
                // adc_data[] values:
            	// 0	-100%
            	// 2048    0%
            	// 4095	+100%
                int value = adc_data[i];

                // Channel value constrained to 11 bits (0 to 2047 inclusive)
                //	0		-125%
                //  204		-100%
                //	1024	   0%
                //	1843	+100%
                //	2047	+125%

                // (4095 - 2048) * 820 / 2048 + 1024 => 1843
                value -= 2048;
                // Scale to 82% (1024-204 == 820)
                value = value * 820 / 2048 + 1024;
                bits |= constrain(value, 0, 2047) << bitsavailable;
                bitsavailable += MULTI_CHAN_BITS;
                while (bitsavailable >= 8) 
                {
                    *p++ = (uint8_t)(bits & 0xff);
                    bits >>= 8;
                    bitsavailable -= 8;
                }
            }

            sendXfer.dataSize = p - sendXfer.data;
        }
    }
}

// Maps EBayangChan to Multiprotocol module channels
static uint8_t BayangChanToMPMChan[] =
{
    9,      // CH_INV
    7,      // CH_VID
    6,      // CH_PIC
    11,     // CH_TO
    12,     // CH_EMG
    4,      // CH_FLIP
    8,      // CH_HEADFREE
    5       // CH_RTH
};
static void mapSwitchesToMPMChans(uint16_t adc_data[], const uint8_t *mapping)
{
    // Disable dyntrim (because I don't know what it does exactly and it seems wrong 
    // to have it on based on code in Silverware)
    adc_data[10]    = 4095;

    // MPM expects analog values for CH_14, CH_15 so we'll use VrA and VrB
    // Note: They're only used if the bayang analog aux option (flag 2) is set
    adc_data[13] = adc_get_channel_calibrated_unscaled(ADC_ID_CH0);
    adc_data[14] = adc_get_channel_calibrated_unscaled(ADC_ID_CH1);

    for (int i=0; i<_CH_Max; i++)
    {
        uint8_t sw = mapping[i];
        if (sw != kSw_None)
        {
            uint8_t mpmChan = BayangChanToMPMChan[i];
            adc_data[mpmChan] = switchIsActive(sw) ? 4095 : 0;
        }
    }
}

void multiprotocol_test(void)
{
    if (button_toggledActive(kBtn_Up))
    {
        multiprotocol_enable();
        debug("MPM on");
        debug_put_newline();
    }
    else if (button_toggledActive(kBtn_Down))
    {
        multiprotocol_disable();
        debug("MPM off");
        debug_put_newline();
    }
    else if (button_active(kBtn_Bind))
    {
        multiprotocol_rebind();
    }

//       debug_put_fixed2(adc_get_battery_voltage());
//        debug(" V\n");
//            debug_put_uint8(i+0); debug_putc('=');
//            debug_put_uint16(adc_data[i]);
//            debug_put_uint16(adc_get_channel_calibrated(i));
//                debug(" ");
    debug_flush();
}

// telemetryType parameter to ProcessTelemetry() should be one of the
// following (Bayang protocol will be MULTI_TELEMETRY_HUB)
enum MultiPacketTypes
{
	MULTI_TELEMETRY_STATUS			= 1,
	MULTI_TELEMETRY_SPORT			= 2,
	MULTI_TELEMETRY_HUB				= 3,
	MULTI_TELEMETRY_DSM				= 4,
	MULTI_TELEMETRY_DSMBIND			= 5,
	MULTI_TELEMETRY_AFHDS2A			= 6,
	MULTI_TELEMETRY_REUSE_1			= 7,
	MULTI_TELEMETRY_SYNC			= 8,
	MULTI_TELEMETRY_REUSE_2			= 9,
	MULTI_TELEMETRY_HITEC			= 10,
	MULTI_TELEMETRY_SCANNER			= 11,
	MULTI_TELEMETRY_AFHDS2A_AC		= 12,
	MULTI_TELEMETRY_RX_CHANNELS		= 13,
	MULTI_TELEMETRY_HOTT			= 14,
	MULTI_TELEMETRY_SILVERLITE		= 15,
};

static void ProcessTelemetry(uint8_t telemetryType, const uint8_t* data, uint8_t dataLen)
{
#if 0


	frame[0] = 0xFE;			// Link frame
	if (protocol==PROTO_FRSKYD)
	{		
		frame[1] = telemetry_in_buffer[3];		// A1
		frame[2] = telemetry_in_buffer[4];		// A2
		frame[3] = telemetry_in_buffer[5];		// RX_RSSI
		telemetry_link &= ~1 ;		// Sent
		telemetry_link |= 2 ;		// Send hub if available
	}
	else
	{//PROTO_HUBSAN, PROTO_AFHDS2A, PROTO_BAYANG, PROTO_NCC1701, PROTO_CABELL, PROTO_HITEC, PROTO_BUGS, PROTO_BUGSMINI, PROTO_FRSKYX
		frame[1] = v_lipo1;
		frame[2] = v_lipo2;
		frame[3] = RX_RSSI;
		telemetry_link=0;
	}
	frame[4] = TX_RSSI;
	frame[5] = RX_LQI;
	frame[6] = TX_LQI;
	frame[7] = frame[8] = 0;

    static void multi_send_header(uint8_t type, uint8_t len)
    {
        Serial_write('M');
        Serial_write('P');
        Serial_write(type);
        Serial_write(len);
    }
	multi_send_header(MULTI_TELEMETRY_HUB, 9);

	for (uint8_t i = 0; i < 9; i++)
		Serial_write(frame[i]);
#endif

    // Every 500ms the MPM sends "STATUS" info
    if (telemetryType == MULTI_TELEMETRY_STATUS)
    {
        // TODO: Review multi_send_status() in Telemetry.ino
#if 0
0	Serial_write(flags);

	// Version number example: 1.1.6.1
1	Serial_write(VERSION_MAJOR);
2	Serial_write(VERSION_MINOR);
3	Serial_write(VERSION_REVISION);
4	Serial_write(VERSION_PATCH_LEVEL);
	// Channel order
5	Serial_write(RUDDER<<6|THROTTLE<<4|ELEVATOR<<2|AILERON);
    // Protocol next/prev
    if(multi_protocols[multi_protocols_index+1].protocol != 0)
6       Serial_write(multi_protocols[multi_protocols_index+1].protocol);		// next protocol number
    else
6       Serial_write(protocol);													// end of list
    if(multi_protocols_index>0)
7       Serial_write(multi_protocols[multi_protocols_index-1].protocol);		// prev protocol number
    else
7       Serial_write(protocol);													// begining of list
    // Protocol
    for(uint8_t i=0;i<7;i++)
8-14     Serial_write(multi_protocols[multi_protocols_index].ProtoString[i]);	// protocol name
    // Sub-protocol
    uint8_t nbr=multi_protocols[multi_protocols_index].nbrSubProto;
    Serial_write(nbr | (multi_protocols[multi_protocols_index].optionType<<4));	// number of sub protocols && option type
    uint8_t j=0;
    if(nbr && (sub_protocol&0x07)<nbr)
    {
        uint8_t len=multi_protocols[multi_protocols_index].SubProtoString[0];
        uint8_t offset=len*(sub_protocol&0x07)+1;
        for(;j<len;j++)
            Serial_write(multi_protocols[multi_protocols_index].SubProtoString[j+offset]);	// current sub protocol name
    }
    for(;j<8;j++)
        Serial_write(0x00);
#endif
        debug("Flags: ");
        uint8_t flags = data[0];
        debug_put_hex8(flags);
        debug(", ");

        char name[12];
        uint8_t i;
        for(i=0;i<7;i++)
        {
            name[i] = data[i+8];
        }
        name[i] = 0;
        debug(name);
        debug_put_newline();
    }
    else if (telemetryType == MULTI_TELEMETRY_HUB)
    {
        if (data[0] == 0xFE)    // 0xFE == link frame
        {
#if 0            
            data[1] = v_lipo1;  // uncompensated battery volts*100/2
            data[2] = v_lipo2;  // compensated battery volts*100/2
            data[3] = RX_RSSI;  // reception in packets / sec
            data[4] = TX_RSSI;
            data[5] = RX_LQI;
            data[6] = TX_LQI;
            data[7] = 0;
            data[8] = 0;
#endif

#if 0
            debug("Batt: ");
            debug_put_hex8(data[1]);
            debug(", ");
            debug_put_hex8(data[2]);
            debug_put_newline();

            debug("RSSI: ");
            debug_put_hex8(data[3]);
            debug(", ");
            debug_put_hex8(data[4]);
            debug_put_newline();


            debug("LQI: ");
            debug_put_hex8(data[5]);
            debug(", ");
            debug_put_hex8(data[6]);
            debug_put_newline();

// For FlySky protocol, above yields:
//          Batt: 86, 00
//			RSSI: CC, F8
//			LQI: 00, 00

// For Bayang w/ Telemetry protocol, above yields:
//          Batt: 15, 9B
//			RSSI: AA, 00
//			LQI: AA, 55

#endif

            for (int i=0; i<8; i++)
            {
                telemetryFrame[i] = data[i];
            }

//------------------------------------------------------------------------------
// Multiprotocol telemetry data
//
// data[1] = v_lipo1;  // uncompensated battery volts*100/2
// data[2] = v_lipo2;  // compensated battery volts*100/2
// data[3] = RX_RSSI;  // reception in packets / sec
// data[4] = TX_RSSI;
// data[5] = RX_LQI;
// data[6] = TX_LQI;
//
// For FlySky protocol, above yields:
// data[1], data[2] (Batt): 86, 00
// data[3], data[4] (RSSI): CC, F8
// data[5], data[6] (LQI):  00, 00
//
// For Bayang protocol, above yields:
// data[1], data[2] (Batt): 15, 9B
// data[3], data[4] (RSSI): AA, 00
// data[5], data[6] (LQI):  AA, 55

            // Get current model so we can use its multiprotocol config
            const ModelDesc_t &model = storage.model[storage.current_model];
            if (model.mpm_protocol == PROTO_BAYANG)
            {
                gSilverLiteData.vbattComp = gSilverLiteData.vbattFilt = telemetryFrame[2] * 2;
                gSilverLiteData.pktsPerSec = telemetryFrame[5] * 2;
            }
            else
            {
                gSilverLiteData.vbattComp = gSilverLiteData.vbattFilt = telemetryFrame[1];
                gSilverLiteData.pktsPerSec = telemetryFrame[3];
            }
            gSilverLiteData.lastUpdate = millis_this_frame();
            gSilverLiteData.tlmCount++;
        }
    }
    else if (telemetryType == MULTI_TELEMETRY_SILVERLITE)
    {
        HandleSilverLitePacket(data);
    }
}

void multiprotocol_enable(void)
{
    if (!bTXEnabled)
    {
        bTXEnabled = true;

        sendXfer.data = g_txBuffer;
        receiveXfer.data = g_rxBuffer;
        bWaitingForTLMHeader = true;

        rxBufferEmpty = true;
        txOnGoing = false;
        rxOnGoing = false;

        for (unsigned i=0; i < (sizeof(telemetryFrame)/sizeof(telemetryFrame[0])); i++)
        {
            telemetryFrame[i] = 0;
        }

        /*
        * config.baudRate_Bps = 115200U;
        * config.parityMode = kUART_ParityDisabled;
        * config.stopBitCount = kUART_OneStopBit;
        * config.txFifoWatermark = 0;
        * config.rxFifoWatermark = 1;
        * config.enableTx = false;
        * config.enableRx = false;
        */
        uart_config_t config;
        UART_GetDefaultConfig(&config);
        config.baudRate_Bps = 100000;
        config.parityMode = kUART_ParityEven;
        config.stopBitCount = kUART_TwoStopBit;
        config.enableTx = true;
        config.enableRx = true;
        UART_Init(MPM_UART, &config, MPM_UART_CLK_FREQ);
        UART_TransferCreateHandle(MPM_UART, &g_uartHandle, UART_UserCallback, NULL);
        UART_TransferStartRingBuffer(MPM_UART, &g_uartHandle, g_rxRingBuffer, RX_RING_BUFFER_SIZE);
    }
}

void multiprotocol_disable(void)
{
    if (bTXEnabled)
    {
        bTXEnabled = false;
        UART_Deinit(MPM_UART);
    }
}

void multiprotocol_rebind(void)
{
    if (bTXEnabled)
    {
        for (unsigned i=0; i < (sizeof(telemetryFrame)/sizeof(telemetryFrame[0])); i++)
        {
            telemetryFrame[i] = 0;
        }
        multi4in1_bind = 100;
    }
}

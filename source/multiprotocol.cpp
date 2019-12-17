#include "MKL16Z4.h"
//#include "fsl_common.h"
//#include "fsl_dma.h"
//#include "fsl_dmamux.h"
#include "fsl_uart.h"
#include "multiprotocol.h"
#include "drv_time.h"

#include <stdio.h>  // printf
#include "console.h"
#include "debug.h"
#include "adc.h"
#include "buttons.h"

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

volatile bool rxBufferEmpty = true;
volatile bool txOnGoing = false;
volatile bool rxOnGoing = false;

static bool bWaitingForTLMHeader;
static uint8_t telemetryType;
static uint8_t expectedTLMDataLength;

static uint8_t multi4in1_bind = 0;
static uint8_t multi4in1_range_check = 0;

#define MULTI_CHANS 16
#define MULTI_CHAN_BITS 11

//--[ Forward Declarations] ----------------------------------------------------
static void ProcessTelemetry(uint8_t telemetryType, const uint8_t* data, uint8_t dataLen);


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
void multiprotocol_init()
{
    printf("multiprotocol_init\r\n");
    sendXfer.data = g_txBuffer;
    receiveXfer.data = g_rxBuffer;
    bWaitingForTLMHeader = true;

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


//------------------------------------------------------------------------------
static bool bTXEnabled = false;

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
        uint32_t    now = micros_this_frame();
        bool bTimeToSend = false;
        static uint32_t lastTX = 0;
        if ((now - lastTX) >= 1000)
        {
            bTimeToSend = true;
        }

        if (bTimeToSend && bTXEnabled)
        {
            lastTX = now;

            uint8_t *p = sendXfer.data;
            uint8_t data;

            // TODO: Put these into a model definition structure
            const uint8_t kBayangProtocol = 14;
            const uint8_t settings_protocol = kBayangProtocol;
            const uint8_t settings_sub_protocol = 0;    // 0 == Bayang
            const uint8_t settings_option = 1;          // 1 == Bayang telemetry. Note: When using test RX board, don't use telemetry (you get lots of bad CRC packets)

            const uint8_t settings_auto_bind = 0;
            const uint8_t settings_rx_num = 0;
            const uint8_t settings_low_power = 0;

#if 0   
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
            *p++ = settings_protocol < 32 ? 0x55 : 0x54;

            // Stream[1] = sub_protocol|RangeCheckBit|AutoBindBit|BindBit;
            data = settings_protocol & 0x1f;
            data |= multi4in1_range_check << 5;
            data |= settings_auto_bind << 6;
            if (multi4in1_bind)
            {
                data |= 1 << 7;
                multi4in1_bind = 0;
            }
            *p++ = data;

            // Stream[2] = RxNum | Type | Power;
            data = settings_rx_num & 0x0f;
            data |= (settings_sub_protocol & 0x07) << 4;
            data |= settings_low_power << 7;
            *p++ = data;

            // Stream[3] = option_protocol;
            *p++ = settings_option;

            // Stream[4] to [25] = Channels
            uint32_t bits = 0;
            uint8_t bitsavailable = 0;

            // fetch adc channel data for the sticks
            uint16_t adc_data[8];
            for (int i = ADC_ID_AILERON; i <= ADC_ID_SwC; i++) 
            {
                // ADC value will be between 0x0 to 0xFFF inclusive
                int value = adc_get_channel_calibrated_unscaled(i);
                adc_data[i] = value;
            }

            // Multiprotocol module expects 16 channels to be provided
            const uint16_t SwC = adc_get_channel_calibrated(ADC_ID_SwC);
            for (int i = 0; i < MULTI_CHANS; i++) 
            {
                // Stick adc values:
            	// 0	-100%
            	// 2048    0%
            	// 4095	+100%
                int value = (i <= ADC_ID_SwC) ? adc_data[i] : 0;

                if (settings_protocol == kBayangProtocol)
                {
                    // The following channels correspond to various
                    // Bayang flags that we care about
                    enum
                    {
                        CH5_SW  = 4,
                        CH7_SW  = 6,
                        CH8_SW  = 7,
                        CH9_SW  = 8,
                        CH10_SW = 9,
                        CH11_SW = 10
                    };

                    // On my NFE Silverware quads I setup ARMING, IDLE_UP, etc
                    // to virtual channels (CHAN_5, etc) which in turn are initialized
                    // from Bayang flags. The comments below (above each case
                    // statement) describe how I expect things to be mapped.
                    switch (i)
                    {
                        //#define ARMING CHAN_5     BAYANG_FLAG_INVERT, SwA
                        case CH10_SW:
                            value = button_active(kBtn_SwA) ? 4095 : 0;
                            break;

                        //#define IDLE_UP CHAN_9    BAYANG_FLAG_HEADLESS, SwD
                        case CH9_SW:
                            value = button_active(kBtn_SwD) ? 4095 : 0;
                            break;

                        //#define LEVELMODE CHAN_6  BAYANG_FLAG_FLIP, SwB: Position 1
                        case CH5_SW:
                            value = (adc_get_channel_calibrated(ADC_ID_SwB) < 100) ? 4095 : 0;
                            break;

                        //#define RACEMODE  CHAN_7  BAYANG_FLAG_SNAPSHOT, SwC: Position 2
                        case CH7_SW:
                            value = (SwC >= 412) ? 4095 : 0;
                            break;

                        //#define HORIZON   CHAN_8  BAYANG_FLAG_VIDEO, SwC: Position 3
                        case CH8_SW:
                            value = (SwC >= 923) ? 4095 : 0;
                            break;

                        // Disable dyntrim (because I don't know what it does exactly and it seems wrong to have it on based on code in Silverware)
                        case CH11_SW:
                            value = 4095;
                            break;
                    }
                }

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

void multiprotocol_test(void)
{
    if (button_toggledActive(kBtn_Up))
    {
        bTXEnabled = true;
        debug("MPM on");
        debug_put_newline();
    }
    else if (button_toggledActive(kBtn_Down))
    {
        bTXEnabled = false;
        debug("MPM off");
        debug_put_newline();
    }
    else if (button_active(kBtn_Bind))
    {
        if (bTXEnabled)
        {
            multi4in1_bind = 1;
        }
    }

//       debug_put_fixed2(adc_get_battery_voltage());
//        debug(" V\n");
//            debug_put_uint8(i+0); debug_putc('=');
//            debug_put_uint16(adc_data[i]);
//            debug_put_uint16(adc_get_channel_calibrated(i));
//                debug(" ");
    debug_flush();
}

static void ProcessTelemetry(uint8_t telemetryType, const uint8_t* data, uint8_t dataLen)
{
    debug("TLM: ");
    debug_put_uint8(dataLen);
    debug_put_newline();
}

void multiprotocol_enable(void)
{
    bTXEnabled = true;
}

void multiprotocol_disable(void)
{
    bTXEnabled = false;
}

void multiprotocol_rebind(void)
{
    multi4in1_bind = 1;
}

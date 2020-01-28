#ifndef __BAYANG_COMMON_H__
#define __BAYANG_COMMON_H__

#include <inttypes.h>

//------------------------------------------------------------------------------
#define BAYANG_PACKET_PERIOD    1000
#define BAYANG_PACKET_SIZE      15
#define BAYANG_RF_NUM_CHANNELS  4
#define BAYANG_RF_BIND_CHANNEL  0
#define BAYANG_ADDRESS_LENGTH   5

//------------------------------------------------------------------------------
extern uint8_t Bayang_rf_chan;
extern uint8_t Bayang_rf_channels[BAYANG_RF_NUM_CHANNELS];
extern uint8_t Bayang_rx_tx_addr[BAYANG_ADDRESS_LENGTH];
extern uint16_t Bayang_telemetry_count;
extern uint16_t Bayang_last_telemetry_count;
extern uint16_t Bayang_loopcount;
extern uint16_t Bayang_count;


//------------------------------------------------------------------------------
extern uint8_t packet[BAYANG_PACKET_SIZE];

extern uint8_t txPkt_Flags2;    // Used for packet[2]
extern uint8_t txPkt_Flags3;    // Used for packet[3]
extern uint16_t txPkt_Sticks[4];// in AETR order
extern uint16_t txPkt_AuxAnalog[2]; // auxiliary analog channels

//------------------------------------------------------------------------------
uint8_t Bayang_checksum();
void Bayang_init(const uint8_t *transmitterID);

//------------------------------------------------------------------------------
#if defined(__cplusplus)
struct SilverLiteData_t
{
    // Bit flag constants
    enum eFlags
    {
        kRates      =   (1 << 0),
        kHorizon    =   (1 << 1),
        kRaceMode   =   (1 << 2),
        kLevelMode  =   (1 << 3),
        kOnGround   =   (1 << 4),
        kLowBattery =   (1 << 5)
    };

    // Obtained from FC
    uint16_t P[3];              // The P constants. In order of roll, pitch, yaw. Scaled by 1000 (11 bit resolution)
    uint16_t I[3];              // The I constants.
    uint16_t D[3];              // The D constants.

    uint16_t vbattFilt;         // Battery voltage, scaled by 100 (11 bits resolution)
    uint16_t vbattComp;         // Battery voltage compensated, scaled by 100 (11 bit resolution)
    uint16_t pktsPerSec;        // Packets per second that FC has processed (clamped to 1023) (10 bit resolution)
    uint16_t flags;             // See eFlags

    // Note: There will likely be a padding of 2 bytes here (between 'flags' and 'count') which can inflate
    // the number of bytes computed (and) sent to a client if we used:
    //  offsetof(SilverLiteData_t, count);
    // so instead we use:
    //  offsetof(SilverLiteData_t, flags) + sizeof(((SilverLiteData_t*)0)->flags)

    // Used internally (did not come from telemetry packet)
    bool    telemetryChanged;
};
extern struct SilverLiteData_t gSilverLiteData;

//------------------------------------------------------------------------------
struct TXContext_t
{
    uint16_t    bindCounter;        // number of bind packets remaining to be sent
    uint8_t     protocol;           // EProtocol
    uint8_t     options;            // Bit flags: 0x01 == use telemetry, 0x02 use aux analog channels
    uint8_t     resetToProtocol;    // If non-zero, this is the protocol we should reset to
    uint8_t     resetToOptions;     // ..and these are the options that go along with it
    uint8_t     expectTelemetry;    // true if expecting telemetry
    uint8_t     awaitingTelemetry;  // true if awaiting telemetry response for last packet
    uint8_t     _unused;            //
    uint8_t     silverLiteEnabled;  // true if flight controller reported back that it has enabled SilverLite extension
    int8_t      sendInterval;       // Interval in milliseconds between sending TX packets
    int8_t      intervalCounter;    // Countdown timer used by ISR
    uint32_t    txSendCount;        // number of TX packets sent
    uint32_t    txSendTime;         // time in micros it takes to send packet
    uint32_t    telemetryRxTime;    // time in micros it takes to receive telemetry packet
    uint32_t    irqTime;            // time it took for most recent irq handler to complete
    uint32_t    irqHits;            // incremented every time ISR his executed
    uint32_t    resetTime;          // How long it took to perform a protocol reset
    uint16_t    telemetryTimeout;   // if non-zero, decremented every millisecond. Reloaded if telemetry recevied
    uint16_t    telemetryTimeoutLoad;   // Used to reload telemetryTimeout

    uint16_t	tlmRatio;			// (tlmCount * 100) / tlmExpected
    uint16_t    tlmCount;           // number of telemetry packets received
    uint16_t    tlmExpected;        // expected number of telemetry packets
    uint16_t    tlmBattVoltage;     // Telemetry reported battery voltage (uncompensated)
};
extern struct TXContext_t gTXContext;

#endif

#endif // #ifndef __BAYANG_COMMON_H__

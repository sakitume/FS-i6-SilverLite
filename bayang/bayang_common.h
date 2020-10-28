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

// These are written to by foreground task and read by ISR handler
extern volatile uint8_t txPkt_Flags2;       // Used for packet[2]
extern volatile uint8_t txPkt_Flags3;       // Used for packet[3]
extern volatile uint16_t txPkt_Sticks[4];   // in AETR order
extern volatile uint16_t txPkt_AuxAnalog[2];// auxiliary analog channels

//------------------------------------------------------------------------------
uint8_t Bayang_checksum();
void Bayang_init(const uint8_t *transmitterID);

//------------------------------------------------------------------------------
#if defined(__cplusplus)

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
    uint32_t    txSendTime;         // time in micros it takes to send packet (893 to 894 microseconds)
    uint32_t    telemetryRxTime;    // time in micros it takes to receive telemetry packet
    uint32_t    irqTime;            // longest time it took for irq handler to complete (898 to 899 microseconds)
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

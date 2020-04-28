#ifndef __SILVERLITE_DATA_H__
#define __SILVERLITE_DATA_H__

#include <stdint.h>

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

void HandleSilverLitePacket(const uint8_t* packet);

#endif

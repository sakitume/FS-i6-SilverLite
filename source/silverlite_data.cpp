#include "silverlite_data.h"

//------------------------------------------------------------------------------
struct SilverLiteData_t gSilverLiteData;

//------------------------------------------------------------------------------
void HandleSilverLitePacket(const uint8_t* packet)
{
    // Byte 0 of packet:
    // 0xA0 = Normal telemetry
    // 0xA1 = PIDs
    const int packetType = packet[0];
    if (packetType == 0xA0)
    {
        const uint16_t bits = packet[4];

        uint16_t temp;
        #define UPDATE_TEST(lVal, rVal)     temp = rVal;  if (lVal != temp) { lVal = temp; gSilverLiteData.telemetryChanged = true; }
        UPDATE_TEST(gSilverLiteData.vbattFilt, packet[1] | ((bits << 8) & 0x700))
        UPDATE_TEST(gSilverLiteData.vbattComp, packet[2] | ((bits << 5) & 0x700))
        UPDATE_TEST(gSilverLiteData.pktsPerSec, packet[3] | ((bits << 2) & 0x300))
        UPDATE_TEST(gSilverLiteData.flags, packet[5])
    }
    else if (packetType == 0xA1)   // pids
    {
        const uint32_t bits = 
            (((uint32_t)packet[10]) << 0)  |
            (((uint32_t)packet[11]) << 8)  |
            (((uint32_t)packet[12]) << 16) |
            (((uint32_t)packet[13]) << 24);

        gSilverLiteData.P[0] = packet[1] | ((bits << 8) & 0x700);
        gSilverLiteData.P[1] = packet[2] | ((bits << 5) & 0x700);
        gSilverLiteData.P[2] = packet[3] | ((bits << 2) & 0x700);

        gSilverLiteData.I[0] = packet[4] | ((bits >> 1) & 0x700);
        gSilverLiteData.I[1] = packet[5] | ((bits >> 4) & 0x700);
        gSilverLiteData.I[2] = packet[6] | ((bits >> 7) & 0x700);

        gSilverLiteData.D[0] = packet[7] | ((bits >> 10) & 0x700);
        gSilverLiteData.D[1] = packet[8] | ((bits >> 13) & 0x700);
        gSilverLiteData.D[2] = packet[9] | ((bits >> 16) & 0x700);
    }
}


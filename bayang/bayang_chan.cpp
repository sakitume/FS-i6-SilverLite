#include "bayang_chan.h"
#include "switchID.h"

const char *gBayangChanNames[_CH_Max] =
{
    "CH_INV",
    "CH_VID",
    "CH_PIC",
    "CH_TO",
    "CH_EMG",
    "CH_FLIP",
    "CH_HEADFREE",
    "CH_RTH"
};

void setupDefaultMapping(uint8_t bayangChansToSwitchID[])
{
    for (int i=0; i<_CH_Max; i++)
    {
        bayangChansToSwitchID[i] = kSw_None;
    }

                                                    // NFE
                                                    // ---
    bayangChansToSwitchID[CH_INV]       = kSwA_2;   // ARMED
    bayangChansToSwitchID[CH_FLIP]      = kSwB_1;   // LEVELMODE
    bayangChansToSwitchID[CH_PIC]       = kSwC_2;   // RACEMODE
    bayangChansToSwitchID[CH_VID]       = kSwC_3;   // HORIZON
    bayangChansToSwitchID[CH_HEADFREE]  = kSwD_2;   // IDLE_UP
}

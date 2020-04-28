#ifndef __BAYANG_CHAN_H__
#define __BAYANG_CHAN_H__

#include <stdint.h>

// These are the auxiliary bayang channels originally defined in no particular order
// *BUT* their order is important as mapping tables like BayangChanToMPMChan[] and
// gBayangChanNames[] depend on it
//
// So if you reorder or redefine this be sure to update BayangChanToMPMChan[], gBayangChanNames[]
//
enum EBayangChan
{
    CH_INV,
    CH_VID,
    CH_PIC,
    CH_TO,
    CH_EMG,
    CH_FLIP,
    CH_HEADFREE,
    CH_RTH,
    _CH_Max
};

extern const char *gBayangChanNames[_CH_Max];

void setupDefaultMapping(uint8_t bayangChansToSwitchID[]);

#endif // __BAYANG_CHAN_H__
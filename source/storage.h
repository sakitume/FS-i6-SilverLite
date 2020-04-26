#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdint.h>

#if defined(__cplusplus)

#include "bayang_chan.h"    // for _CH_Max

#define STORAGE_VERSION_ID 0x03
#define STORAGE_MODEL_NAME_LEN 11
#define STORAGE_MODEL_MAX_COUNT 10

struct ModelDesc_t 
{
    enum        {   kModelNameLen = 10  };
    char        name[kModelNameLen];
    uint16_t    timer;

    // Bayang channels
    uint8_t bayangChans[_CH_Max];

    uint8_t armSwitch;

    // Multiprotocol params
    uint8_t mpm_protocol;
    uint8_t mpm_sub_protocol;
    uint8_t mpm_option;
    uint8_t mpm_auto_bind;
    uint8_t mpm_rx_num;
    uint8_t mpm_low_power;
};

struct FlashStorage_t 
{
    uint32_t    fixes_alignment;    // ensures storage is 32bit aligned
    uint8_t     bayang_txid[4];

    // stick calibration data
    uint16_t    chanMinMidMax[4][3];

    // model settings
    uint8_t     current_model;
    ModelDesc_t model[STORAGE_MODEL_MAX_COUNT];

    // new data should be placed BEFORE the checksum...
    //
    // checksum
    uint16_t    checksum;
    uint32_t    fixes_alignment2;    // ensures storage is 32bit aligned
};

extern FlashStorage_t storage;
#endif /* __cplusplus */


//------------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

uint8_t storage_init();
uint8_t storage_is_valid();
void storage_save();
void storage_take_snapshot();
void storage_restore_snapshot();

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif  // __STORAGE_H__

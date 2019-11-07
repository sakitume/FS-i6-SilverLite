#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdint.h>

#if defined(__cplusplus)

#define STORAGE_VERSION_ID 0x03
#define STORAGE_MODEL_NAME_LEN 11
#define STORAGE_MODEL_MAX_COUNT 10

struct ModelDesc_t 
{
    enum        {   kModelNameLen = 10  };
    char        name[kModelNameLen];
    uint16_t    timer;
};

struct FlashStorage_t 
{
    uint8_t     bayang_txid[4];

    // stick calibration data
    uint16_t    chanMinMidMax[4][3];

    // model settings
    uint8_t     current_model;
    ModelDesc_t model[STORAGE_MODEL_MAX_COUNT];

    // new data should be placed BEFORE the checksum...
    //
    // checksum
    uint16_t checksum;
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

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif  // __STORAGE_H__

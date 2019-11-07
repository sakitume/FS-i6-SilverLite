#include "fsl_debug_console.h"
#include "storage.h"
#include "flash.h"

//------------------------------------------------------------------------------

FlashStorage_t storage;
static bool valid;

// lookup table for crc16 CCITT
static const uint16_t crc16_table[16] = {
  0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285, 0x6306, 0x7387,
  0x8408, 0x9489, 0xA50A, 0xB58B, 0xC60C, 0xD68D, 0xE70E, 0xF78F
};

//------------------------------------------------------------------------------
static uint16_t crc16_update(uint16_t crc, uint8_t c) {
    crc = (((crc >> 4) & 0x0FFF) ^ crc16_table[((crc ^ c) & 0x000F)]);
    crc = (((crc >> 4) & 0x0FFF) ^ crc16_table[((crc ^ (c>>4)) & 0x000F)]);
    return crc;
}

//------------------------------------------------------------------------------
static uint16_t crc16(uint8_t *buf, uint16_t len) {
    uint16_t crc = 0;
    while (len--) {
        crc = crc16_update(crc, *buf++);
    }
    return crc;
}

//------------------------------------------------------------------------------
static uint16_t storage_calc_crc(void) 
{
    uint16_t crc;
    uint8_t *storage_ptr = (uint8_t*)&storage;
    
//    crc = crc16(storage_ptr, sizeof(storage) - sizeof(storage.checksum));
    crc = crc16(storage_ptr, offsetof(FlashStorage_t, checksum));
    return crc;
}

//------------------------------------------------------------------------------
uint8_t storage_is_valid()
{
    return valid;
}

//------------------------------------------------------------------------------
uint8_t storage_init()
{
    static_assert(sizeof(storage) < 256, "FlashStorage_t is too big");

    valid = false;
    int result = flash_read(0, &storage, sizeof(storage));
    if (result == sizeof(storage))
    {
        valid = storage_calc_crc() == storage.checksum;
    }
    if (!valid)
    {
        memset(&storage, 0, sizeof(storage));
        // TODO: Provide defaults?
    }

    // TODO
    for (int i=0; i<STORAGE_MODEL_MAX_COUNT; i++)
    {
        ModelDesc_t &model = storage.model[i];
        model.timer = 4 * 60;
        strcpy(model.name, "TinyWhoop");
    }

    return valid;
}

//------------------------------------------------------------------------------
void storage_save()
{
    static_assert(sizeof(storage) % 4 == 0, "Unexpected size for FlashStorage_t");
    
    storage.checksum = storage_calc_crc();

#if 1
    PRINTF("storage_save: temporarily disabled");
#else
    int result = flash_write(0, &storage, sizeof(storage));
    if (result)
    {
        PRINTF("storage_save(): flash_write error");
    }
#endif    
}

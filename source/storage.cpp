#include "storage.h"
#include "flash.h"
#include <stddef.h>
#include <string.h>
#include "multiprotocol.h"

#define __HARDWARE_TIMER_WORKAROUND__
#if defined(__HARDWARE_TIMER_WORKAROUND__)    
    #include "timer.h"  // TODO, XXX: Hack workaround
//    #include "delay.h"
#endif

//------------------------------------------------------------------------------

static FlashStorage_t storageSnapshot;
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
static void setDefaults()
{
    memset(&storage, 0, sizeof(storage));
    storage.current_model = 0;
    for (int i=0; i<STORAGE_MODEL_MAX_COUNT; i++)
    {
        ModelDesc_t &model = storage.model[i];
        model.timer = 4 * 60;
        strcpy(model.name, "TinyWhoop");

        model.mpm_protocol = kBayangProtocol;
        model.mpm_sub_protocol = 0;    // 0 == Bayang
        model.mpm_option = 1;          // 1 == Bayang telemetry
        model.mpm_auto_bind = 0;
        model.mpm_rx_num = 0;
        model.mpm_low_power = 0;
    }
}

//------------------------------------------------------------------------------
uint8_t storage_init()
{
    static_assert(sizeof(storage) < 256, "FlashStorage_t is too big");

    valid = false;
    int result = flash_read(&storage, sizeof(storage));
    if (result == sizeof(storage))
    {
        valid = storage_calc_crc() == storage.checksum;
    }
    if (!valid)
    {
        memset(&storage, 0, sizeof(storage));
        setDefaults();
    }

    storage_take_snapshot();
    return valid;
}

//------------------------------------------------------------------------------
void storage_save()
{
#if defined(__HARDWARE_TIMER_WORKAROUND__)    
    timer_stop_hardware();
// delay doesn't seem necessary, simply stopping the timer is sufficient    
//    delay_ms(2);
#endif

    static_assert(sizeof(storage) % 4 == 0, "Unexpected size for FlashStorage_t");
    
    storage.checksum = storage_calc_crc();

    int result = flash_write(&storage, sizeof(storage));
    if (result != sizeof(storage))
    {
        //XXX printf("storage_save(): flash_write error");
    }
    else
    {
        storage_take_snapshot();
    }

#if defined(__HARDWARE_TIMER_WORKAROUND__)    
    timer_start_hardware();
#endif    
}

//------------------------------------------------------------------------------
void storage_take_snapshot()
{
    storageSnapshot = storage;
}

//------------------------------------------------------------------------------
void storage_restore_snapshot()
{
    storage = storageSnapshot;
}

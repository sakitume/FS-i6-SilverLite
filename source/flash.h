#ifndef __FLASH_H__
#define __FLASH_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

//#include <inttypes.h>

int flash_init();
void flash_test();

int flash_ready();

enum EEPROM_Map
{
    kEEPROM_TransmitterID,              // 4 bytes, transmitter ID
    kEEPROM_StickCalibration   = 4,     // 24 bytes, stick calibration data (see adc.cpp)
    kEEPROM_kNext              = 28
};

int flash_read(enum EEPROM_Map addr, void *dest, unsigned sizeBytes);
int flash_write(enum EEPROM_Map addr, const void *dest, unsigned sizeBytes);


#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif // #ifndef __FLASH_H__

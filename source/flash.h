#ifndef __FLASH_H__
#define __FLASH_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

//#include <inttypes.h>

int flash_init();
void flash_test();

int flash_ready();

int flash_read(void *dest, unsigned sizeBytes);
int flash_write(const void *dest, unsigned sizeBytes);


#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif // #ifndef __FLASH_H__

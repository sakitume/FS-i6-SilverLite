#ifndef __DRV_SOFTSPI_H__
#define __DRV_SOFTSPI_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>

//------------------------------------------------------------------------------
// Function declarations
void spi_init();
uint8_t spi_write(uint8_t command);
uint8_t spi_read();

//------------------------------------------------------------------------------
#if defined(__TEST_SOFTSPI_GPIO__)
// Solely for testing IO pins
void sck_on();
void sck_off();
void mosi_on();
void mosi_off();
int miso_read();
#endif

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif // #ifndef __DRV_SOFTSPI_H__

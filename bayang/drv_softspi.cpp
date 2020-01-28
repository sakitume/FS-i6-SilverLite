/*
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License.
 If not, see <http://www.gnu.org/licenses/>.
 */
#include "MKL16Z4.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "drv_softspi.h"
#include "pin_mux.h"

//------------------------------------------------------------------------------
// Note: Reading the Nordic doc sheet for the NRF24L01, the minimum
// SCK low and high times are 40ns

//------------------------------------------------------------------------------
// Output
#define     SCK_on      BOARD_INITPINS_SOFT_SPI_SCK_GPIO->PSOR = 1 << BOARD_INITPINS_SOFT_SPI_SCK_PIN
#define     SCK_off     BOARD_INITPINS_SOFT_SPI_SCK_GPIO->PCOR = 1 << BOARD_INITPINS_SOFT_SPI_SCK_PIN
#define     MOSI_on     BOARD_INITPINS_SOFT_SPI_MOSI_GPIO->PSOR = 1 << BOARD_INITPINS_SOFT_SPI_MOSI_PIN
#define     MOSI_off    BOARD_INITPINS_SOFT_SPI_MOSI_GPIO->PCOR = 1 << BOARD_INITPINS_SOFT_SPI_MOSI_PIN

// Input
#define     MISO_on     BOARD_INITPINS_SOFT_SPI_MISO_GPIO->PDIR & (1 << BOARD_INITPINS_SOFT_SPI_MISO_PIN)

//------------------------------------------------------------------------------
#if defined(__TEST_SOFTSPI_GPIO__)
void sck_on()   {   SCK_on;     }
void sck_off()  {   SCK_off;    }

void mosi_on()  {   MOSI_on;    }
void mosi_off() {   MOSI_off;   }

int miso_read() {  return MISO_on;  }
#endif

//------------------------------------------------------------------------------
// Configure GPIO pins
//  SCK as output, initially low
//  MOSI as output, initially low
//  MISO as input
void spi_init()
{
    gpio_pin_config_t pin_config = {
        kGPIO_DigitalOutput, 0
    };

    /* PORTC5 (pin 50) is configured as PTC5 */
    PORT_SetPinMux(BOARD_INITPINS_SOFT_SPI_SCK_PORT, BOARD_INITPINS_SOFT_SPI_SCK_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(BOARD_INITPINS_SOFT_SPI_SCK_GPIO, BOARD_INITPINS_SOFT_SPI_SCK_PIN, &pin_config);

    /* PORTC6 (pin 51) is configured as PTC6 */
    PORT_SetPinMux(BOARD_INITPINS_SOFT_SPI_MOSI_PORT, BOARD_INITPINS_SOFT_SPI_MOSI_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(BOARD_INITPINS_SOFT_SPI_MOSI_GPIO, BOARD_INITPINS_SOFT_SPI_MOSI_PIN, &pin_config);

#if 0
    port_pin_config_t input_pullup = {
        kPORT_PullUp,
        kPORT_FastSlewRate,
        kPORT_PassiveFilterDisable,
        kPORT_LowDriveStrength,
        kPORT_MuxAsGpio,
    };
    PORT_SetPinConfig(BOARD_INITPINS_SOFT_SPI_MISO_PORT, BOARD_INITPINS_SOFT_SPI_MISO_PIN, &input_pullup); // PORTB, 1
#else
    /* PORTD3 (pin 60) is configured as PTD3 */
    PORT_SetPinMux(BOARD_INITPINS_SOFT_SPI_MISO_PORT, BOARD_INITPINS_SOFT_SPI_MISO_PIN, kPORT_MuxAsGpio);
    pin_config.pinDirection = kGPIO_DigitalInput;
    GPIO_PinInit(BOARD_INITPINS_SOFT_SPI_MISO_GPIO, BOARD_INITPINS_SOFT_SPI_MISO_PIN, &pin_config);
#endif 
}

//------------------------------------------------------------------------------
uint8_t spi_write(uint8_t command) 
{
    uint8_t result=0;
    uint8_t n=8;
    SCK_off;
    MOSI_off;
    while(n--) {
        if(command & 0x80)
            MOSI_on;
        else
            MOSI_off;
        if(MISO_on)
            result = (result<<1)|0x01;
        else
            result = result<<1;
        SCK_on;
        __NOP();
        SCK_off;
        command = command << 1;
    }
    MOSI_on;
    return result;
}

//------------------------------------------------------------------------------
// read one byte from MISO
uint8_t spi_read()
{
    uint8_t result=0;
    uint8_t i;
    MOSI_off;
    __NOP();
    for(i=0;i<8;i++) {
        if(MISO_on) // if MISO is HIGH
            result = (result<<1)|0x01;
        else
            result = result<<1;
        SCK_on;
        __NOP();
        SCK_off;
        __NOP();
    }
    return result;
}


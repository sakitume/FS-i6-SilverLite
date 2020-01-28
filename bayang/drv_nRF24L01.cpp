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
#include "pin_mux.h"
#include "drv_nRF24L01.h"
#include "drv_softspi.h"
#include "drv_time.h"


//------------------------------------------------------------------------------
static uint8_t rf_setup;

//------------------------------------------------------------------------------
// Forward declarations
static uint8_t Strobe(uint8_t state);

#if __USE_ORIGINAL_READ_WRITE__
static void spi_write_address(uint8_t address, uint8_t data);
static uint8_t spi_read_address(uint8_t address);
#endif

//------------------------------------------------------------------------------
#define     CS_on   BOARD_INITPINS_SOFT_SPI_CS_NRF24L01_GPIO->PSOR = 1 << BOARD_INITPINS_SOFT_SPI_CS_NRF24L01_PIN
#define     CS_off  BOARD_INITPINS_SOFT_SPI_CS_NRF24L01_GPIO->PCOR = 1 << BOARD_INITPINS_SOFT_SPI_CS_NRF24L01_PIN
#define     CE_on   BOARD_INITPINS_NRF24L01_CE_GPIO->PSOR = 1 << BOARD_INITPINS_NRF24L01_CE_PIN
#define     CE_off  BOARD_INITPINS_NRF24L01_CE_GPIO->PCOR = 1 << BOARD_INITPINS_NRF24L01_CE_PIN

//------------------------------------------------------------------------------
void cs_on()    {   CS_on;  }
void cs_off()   {   CS_off; }

void ce_on()    {   CE_on;  }
void ce_off()   {   CE_off; }

//------------------------------------------------------------------------------
void NRF24L01_InitGPIO()
{
    // Setup GPIO for CS and CE pins to be output
    // Set CS to be high (inactive)
    // Set CE to be low (powerdown)
    gpio_pin_config_t pin_config = {
        kGPIO_DigitalOutput, 1
    };
    GPIO_PinInit(BOARD_INITPINS_SOFT_SPI_CS_NRF24L01_GPIO, BOARD_INITPINS_SOFT_SPI_CS_NRF24L01_PIN, &pin_config); // PORTD2 (number 59), spare? (unmarked test pad)

    pin_config.outputLogic = 0;
    GPIO_PinInit(BOARD_INITPINS_NRF24L01_CE_GPIO, BOARD_INITPINS_NRF24L01_CE_PIN, &pin_config); // PORTA2 (number 24)
}

#if __USE_ORIGINAL_READ_WRITE__
//------------------------------------------------------------------------------
// TODO: Only used by NRF24L01_WriteReg. Could inline this
static void spi_write_address(uint8_t address, uint8_t data) 
{
    CS_off;
    spi_write(address);
    __NOP();
    spi_write(data);
    CS_on;
}

//------------------------------------------------------------------------------
// TODO: Only used by NRF24L01_ReadReg. Could inline this
uint8_t spi_read_address(uint8_t address) 
{
    uint8_t result;
    CS_off;
    spi_write(address);
    result = spi_read();
    CS_on;
    return(result);
}
#endif


//------------------------------------------------------------------------------
uint8_t NRF24L01_WriteReg(uint8_t address, uint8_t data)
{
    CS_off;
#if __USE_ORIGINAL_READ_WRITE__
    spi_write_address(address | W_REGISTER, data);
#else
    spi_write(address | W_REGISTER);
    __NOP();
    spi_write(data);
#endif    
    CS_on;
    return 1;
}

//------------------------------------------------------------------------------
void NRF24L01_WriteRegisterMulti(uint8_t address, const uint8_t data[], uint8_t len)
{
#if __USE_ORIGINAL_WR_REG_MULTI__
    delay_us(5);
#endif    
    CS_off;
    spi_write(address | W_REGISTER);
    for(uint8_t i=0;i<len;i++)
        spi_write(data[i]);
    CS_on;
#if __USE_ORIGINAL_WR_REG_MULTI__
    delay_us(5);
#endif    
}

void NRF24L01_Initialize()
{
    rf_setup = 0x0F;
}

uint8_t NRF24L01_FlushTx()
{
    return Strobe(FLUSH_TX);
}

uint8_t NRF24L01_FlushRx()
{
    return Strobe(FLUSH_RX);
}

static uint8_t Strobe(uint8_t state)
{
    uint8_t result;
    CS_off;
    result = spi_write(state);
    CS_on;
    return result;
}

uint8_t NRF24L01_WritePayload(uint8_t *data, uint8_t length)
{
    CE_off;
    CS_off;
    spi_write(W_TX_PAYLOAD); 
    for(uint8_t i=0; i<length; i++)
        spi_write(data[i]);
    CS_on;
    CE_on; // transmit
    return 1;
}

uint8_t NRF24L01_ReadPayload(uint8_t *data, uint8_t length)
{
    uint8_t i;
    CS_off;
    spi_write(R_RX_PAYLOAD); // Read RX payload
    for (i=0;i<length;i++) {
        data[i]=spi_read();
    }
    CS_on;
    return 1;
}

uint8_t NRF24L01_ReadReg(uint8_t reg)
{
    CS_off;
#if __USE_ORIGINAL_READ_WRITE__
    uint8_t data = spi_read_address(reg);
#else
    spi_write(reg);
    uint8_t data = spi_read();
#endif    
    CS_on;
    return data;
}

uint8_t NRF24L01_Activate(uint8_t code)
{
    CS_off;
    spi_write(ACTIVATE);
    spi_write(code);
    CS_on;
    return 1;
}

void NRF24L01_SetTxRxMode(enum TXRX_State mode)
{
    if(mode == TX_EN) {
        CE_off;
        NRF24L01_WriteReg(NRF24L01_07_STATUS, (1 << NRF24L01_07_RX_DR)    //reset the flag(s)
                                            | (1 << NRF24L01_07_TX_DS)
                                            | (1 << NRF24L01_07_MAX_RT));
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)   // switch to TX mode
                                            | (1 << NRF24L01_00_CRCO)
                                            | (1 << NRF24L01_00_PWR_UP));
        delay_us(130);
        CE_on;
    } else if (mode == RX_EN) {
        CE_off;
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);        // reset the flag(s)
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0F);        // switch to RX mode
        NRF24L01_WriteReg(NRF24L01_07_STATUS, (1 << NRF24L01_07_RX_DR)    //reset the flag(s)
                                            | (1 << NRF24L01_07_TX_DS)
                                            | (1 << NRF24L01_07_MAX_RT));
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)   // switch to RX mode
                                            | (1 << NRF24L01_00_CRCO)
                                            | (1 << NRF24L01_00_PWR_UP)
                                            | (1 << NRF24L01_00_PRIM_RX));
        delay_us(130);
        CE_on;
    } else {
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)); //PowerDown
        CE_off;
    }
}

uint8_t NRF24L01_Reset()
{
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    uint8_t status1 = Strobe(0xFF); // NOP
    uint8_t status2 = NRF24L01_ReadReg(0x07);
    NRF24L01_SetTxRxMode(TXRX_OFF);
    return (status1 == status2 && (status1 & 0x0f) == 0x0e);
}

uint8_t NRF24L01_SetPower(enum TX_Power power)
{
    rf_setup = (rf_setup & 0xF9) | ((power & 0x03) << 1);
    return NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
}

uint8_t NRF24L01_SetBitrate(uint8_t bitrate)
{
    // Note that bitrate 250kbps (and bit RF_DR_LOW) is valid only
    // for nRF24L01+. There is no way to programmatically tell it from
    // older version, nRF24L01, but the older is practically phased out
    // by Nordic, so we assume that we deal with with modern version.

    // Bit 0 goes to RF_DR_HIGH, bit 1 - to RF_DR_LOW
    rf_setup = (rf_setup & 0xD7) | ((bitrate & 0x02) << 4) | ((bitrate & 0x01) << 3);
    return NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
}

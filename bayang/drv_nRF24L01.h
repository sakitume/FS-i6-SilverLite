#ifndef __DRV_NRF24L01_H__
#define __DRV_NRF24L01_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */


#include <inttypes.h>
#include "iface_nrf24l01.h"

extern void NRF24L01_InitGPIO();
extern uint8_t NRF24L01_Reset();
extern void NRF24L01_Initialize();
extern void NRF24L01_SetTxRxMode(enum TXRX_State mode);
extern uint8_t NRF24L01_WriteReg(uint8_t address, uint8_t data);
extern uint8_t NRF24L01_ReadReg(uint8_t reg);
extern void NRF24L01_WriteRegisterMulti(uint8_t address, const uint8_t data[], uint8_t len);
extern uint8_t NRF24L01_WritePayload(uint8_t *data, uint8_t length);
extern uint8_t NRF24L01_ReadPayload(uint8_t *data, uint8_t length);
extern uint8_t NRF24L01_FlushTx();
extern uint8_t NRF24L01_FlushRx();
extern uint8_t NRF24L01_SetPower(enum TX_Power power);
extern uint8_t NRF24L01_Activate(uint8_t code);
extern uint8_t NRF24L01_SetBitrate(uint8_t bitrate);


// Solely for testing GPIO
void cs_on();
void cs_off();
void ce_on();
void ce_off();

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif // #ifndef __DRV_NRF24L01_H__

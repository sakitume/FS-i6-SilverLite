#ifndef __DRV_XN297_EMU_H__
#define __DRV_XN297_EMU_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>

extern void XN297_SetTXAddr(const uint8_t* addr, uint8_t len);
extern void XN297_SetRXAddr(const uint8_t* addr, uint8_t len);
extern uint8_t XN297_ReadPayload(uint8_t* msg, uint8_t len);
extern uint8_t XN297_WritePayload(uint8_t* msg, uint8_t len);
extern void XN297_Configure(uint8_t flags);

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif // #ifndef __DRV_XN297_EMU_H__

#ifndef __DRV_TIME_H__
#define __DRV_TIME_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>

int BOARD_SysTick(void);
unsigned long micros(void);
void delay(uint32_t data);


#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif // #ifndef __DRV_TIME_H__

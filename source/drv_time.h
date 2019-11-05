#ifndef __DRV_TIME_H__
#define __DRV_TIME_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>

int BOARD_SysTick(void);

//------------------------------------------------------------------------------
// Call time_update() once at start of every main loop. It must be called at
// least every 2000000 microseconds to avoid error. Returns total number of
// elapsed milliseconds since startup.
unsigned long time_update(void);

//------------------------------------------------------------------------------
// Returns total number of elapsed microseconds since startup. Evaluated at realtime
unsigned long micros_realtime(void);

//------------------------------------------------------------------------------
// Returns number of elapsed microseconds since startup. This value is for the
// the current main-loop/update frame
unsigned long micros_this_frame(void);

//------------------------------------------------------------------------------
// Returns number of elapsed milliseconds since startup. This value is for the
// the current main-loop/update frame
unsigned long millis_this_frame(void);

//------------------------------------------------------------------------------
// Delays for the requested number of microseconds
void delay_us(int32_t us);

//------------------------------------------------------------------------------
// Delays for the requested number of milliseconds
void delay_ms(uint32_t ms);


#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif // #ifndef __DRV_TIME_H__

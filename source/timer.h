#ifndef __TIMER_H__
#define __TIMER_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>


void timer_init();
void timer_update();

typedef void (*milli_timer_cb_t)(unsigned long millis);
void timer_add_callback(milli_timer_cb_t callback);

void timer_set_timeout(unsigned timerID, unsigned timeoutMillis);
int timer_get_timeout(unsigned timerID);

void timer_start_hardware();
void timer_stop_hardware();

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif // #ifndef __TIMER_H__

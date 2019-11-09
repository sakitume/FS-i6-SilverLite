#ifndef __GUI_H__
#define __GUI_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

void gui_init();
void gui_loop();

void gui_header_render(const char *);
extern volatile uint32_t gui_loop_100ms_counter;

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif  // __GUI_H__

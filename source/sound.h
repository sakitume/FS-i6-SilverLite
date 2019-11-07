#ifndef __SOUND_H__
#define __SOUND_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

void sound_init();
void sound_update();

void sound_play_bind();
void sound_play_click();
void sound_play_low_time();

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* __SOUND_H__ */

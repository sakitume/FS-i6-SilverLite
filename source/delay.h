#ifndef _DELAY_H_
#define _DELAY_H_
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

void delay_us(int32_t us);
void delay_ms(uint32_t ms);
#define _delay_ms	delay_ms

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _DELAY_H_ */

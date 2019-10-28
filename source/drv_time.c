// Based on drv_time.c from Silverware
#include "drv_time.h"
#include "board.h"
#include "clock_config.h"

static unsigned long lastticks;
static unsigned long globalticks;
static volatile unsigned long systickcount = 0;

void SysTick_Handler(void)
{
}

// See section 3.3.1.3 of NXP reference manual for KL16: 
//  https://www.nxp.com/files-static/microcontrollers/doc/ref_manual/KL16P80M48SF4RM.pdf
//
// 3.3.1.3 System tick timer
//  The CLKSOURCE field in SysTick Control and Status register selects either the core
//  clock (when CLKSOURCE = 1) or a divide-by-16 of the core clock (when
//  CLKSOURCE = 0). Because the timing reference is a variable frequency, the TENMS
//  field in the SysTick Calibration Value Register is always 0.
//
// This means if we run at 48Mhz then the timing reference will be 48Mhz/16 => 3Mhz
// In other words the timing reference is 3Mhz.
//
static __INLINE uint32_t SysTick_Config2(uint32_t ticks)
{
    if (ticks > SysTick_LOAD_RELOAD_Msk)
        return (1); /* Reload value impossible */

    SysTick->LOAD = (ticks & SysTick_LOAD_RELOAD_Msk) - 1;       /* set reload register */
    NVIC_SetPriority(SysTick_IRQn, (1 << __NVIC_PRIO_BITS) - 1); /* set Priority for Cortex-M0 System Interrupts */
    SysTick->VAL = 0;                                            /* Load the SysTick Counter Value */
    SysTick->CTRL = //SysTick_CTRL_CLKSOURCE_Msk |  // Not setting this bit means CLKSOURCE will be core-clock divided by 16
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk; /* Enable SysTick IRQ and SysTick Timer */
    return (0);                              /* Function successful */
}

int BOARD_SysTick(void)
{
    if (SysTick_Config2(SystemCoreClock / 8))
    {
        while (1)
            ;
    }
    return 0;
}

// called at least once per 16ms or time will overflow
unsigned long time_update(void)
{
    unsigned long maxticks = SysTick->LOAD;
    unsigned long ticks = SysTick->VAL;
    unsigned long quotient;
    unsigned long elapsedticks;
    static unsigned long remainder = 0; // carry forward the remainder ticks;

    if (ticks < lastticks)
    {
        elapsedticks = lastticks - ticks;
    }
    else
    {
        // overflow ( underflow really)
        elapsedticks = lastticks + (maxticks - ticks);
    }

    lastticks = ticks;
    elapsedticks += remainder;

#if 0
    quotient = elapsedticks / 3;
    remainder = elapsedticks - quotient * 3;
    globalticks = globalticks + quotient;
    return globalticks;
#else
    // faster divide by 3
    quotient = elapsedticks * (43691*2) >> 18;
    remainder = elapsedticks - quotient * 3;
    globalticks = globalticks + quotient;
    return globalticks;
#endif
}

// return time in uS from start ( micros())
unsigned long micros()
{
    return time_update();
}

// delay in uS
// The 'data * 6' was obtained empircally thru trial and error
// using this test code:
//    while (1) {
//        delay_us(1234);
//        unsigned long delta = micros() - totalMicros;
//        screen_clear();
//        screen_put_uint14(10, 10, 1, delta);
//        screen_update();
//        _delay_ms(100);
//    }
void delay_us(uint32_t us)
{
    volatile uint32_t count;
    count = us * 6 - 6;
    while (count--)
        ;
}

void delay_ms(uint32_t ms)
{
    unsigned long startTime = micros();
    ms *= 1000;
    // wait (doing nothing) until enough time has expired
    while ((micros() - startTime) < ms)
    {
    }
}


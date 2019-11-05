// Based on drv_time.c from Silverware
#include "drv_time.h"
#include "board.h"
#include "clock_config.h"
#include "screen.h"

//------------------------------------------------------------------------------
// Total number of elapsed microseconds
// Overflows in 71 minutes
// Updated on every call to time_update()
static unsigned long gTotalMicros;  

// Total number of elapsed milliseconds
// Overflows in 49.7 days
// Updated on every call to time_update()
static unsigned long gMillisThisFrame;

// Total number of elapsed microseconds leading up to the current frame
// Overflows in 71 minutes
// Updated on every call to time_update()
static unsigned long gMicrosThisFrame;

//------------------------------------------------------------------------------
// This handler is executed every 2000000 microseconds (2 seconds)
void SysTick_Handler(void)
{
}

//------------------------------------------------------------------------------
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
// We set SysTick LOAD register to 6000000. This means the countdown timer will
// become zero (and then be reloaded) every 2000000 microseconds.
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

//------------------------------------------------------------------------------
int BOARD_SysTick(void)
{
    if (SysTick_Config2(SystemCoreClock / 8))   // 48000000 / 8 => 6000000 ticks
    {
        while (1)
            ;
    }
    return 0;
}

//------------------------------------------------------------------------------
// This must be called at least every 2 seconds to avoid underflow of SysTick
// counter
unsigned long micros_realtime(void)
{
    unsigned long maxticks = SysTick->LOAD;
    unsigned long ticks = SysTick->VAL;
    unsigned long quotient;
    unsigned long elapsedticks;
    static unsigned long remainder = 0; // carry forward the remainder ticks;
    static unsigned long lastticks;

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
    gTotalMicros = gTotalMicros + quotient;
#else
    // faster divide by 3
    quotient = elapsedticks * (43691*2) >> 18;
    remainder = elapsedticks - quotient * 3;
    gTotalMicros = gTotalMicros + quotient;
#endif


    return gTotalMicros;
}

//------------------------------------------------------------------------------
// Call time_update() once at start of every main loop. It must be called at
// least every 2000000 microseconds to avoid error.
//
// Updates the following global time accumulators:
//  gMicrosThisFrame
//  gMillisThisFrame
unsigned long time_update()
{
    static unsigned long lastMicros;
    static unsigned long remainder;

    gMicrosThisFrame = micros_realtime();
    unsigned long elapsed = gMicrosThisFrame - lastMicros;
    lastMicros = gMicrosThisFrame;
    elapsed += remainder;
    while (elapsed >= 1000)
    {
        gMillisThisFrame++;
        elapsed -= 1000;
    }
    remainder = elapsed;

    return gMillisThisFrame;
}

//------------------------------------------------------------------------------
// delay in uS
// The 'data * 6' was obtained empircally thru trial and error
// using this test code:
//    while (1) {
//        delay_us(1234);
//        unsigned long delta = micros_realtime() - totalMicros;
//        screen_clear();
//        screen_put_uint14(10, 10, 1, delta);
//        screen_update();
//        _delay_ms(100);
//    }
void delay_us(int32_t us)
{
#if 1
    // based on https:// github.com/leaflabs/libmaple
    us *= 16;

    // fudge for function call overhead
    us--;
    us--;
    us--;
    us--;
    us--;
    us--;
    us--;
    us--;

    asm volatile(".syntax unified           \n\t"
        "   mov r0, %[us]          \n\t"
        "1: subs r0, #1            \n\t"
        "   bge 1b                 \n\t"
        ".syntax divided           \n\t"
    :
    : [us] "r" (us)
    : "r0");
#else    
    volatile uint32_t count;
    count = us * 6 - 6;
    while (count--)
        ;
#endif        
}

//------------------------------------------------------------------------------
void delay_ms(uint32_t ms)
{
    unsigned long startTime = micros_realtime();
    ms *= 1000;
    // wait (doing nothing) until enough time has expired
    while ((micros_realtime() - startTime) < ms)
    {
    }
}

//------------------------------------------------------------------------------
// Returns number of elapsed microseconds since startup leading up to the current
// frame. This value is for the the current main-loop/update frame
unsigned long micros_this_frame(void)
{
    return gMicrosThisFrame;
}

//------------------------------------------------------------------------------
// Returns number of elapsed milliseconds since startup. This value is for the
// the current main-loop/update frame
unsigned long millis_this_frame(void)
{
    return gMillisThisFrame;
}

//------------------------------------------------------------------------------
void delay_test(void)
{
    unsigned long startTime = micros_realtime();
    // Test delay()
    delay_us(5432);
    unsigned long delta = micros_realtime() - startTime;
    screen_clear();
    screen_put_uint14(10, 10, 1, delta);
    screen_update();
}

//------------------------------------------------------------------------------
void millis_test(void)
{
    screen_clear();
    screen_put_time(10, 10, 1, millis_this_frame() / 1000);
    screen_update();
}

#include "fsl_debug_console.h"
#include "fsl_pit.h"
#include "timer.h"

//------------------------------------------------------------------------------
enum { kMaxCallbacks = 4 };
static milli_timer_cb_t callbacks[kMaxCallbacks];
static volatile uint8_t numCallbacks;
static volatile uint32_t gTotalMillis;

enum { kMaxTimeouts = 4 };
static volatile uint32_t timeouts[kMaxTimeouts];
//------------------------------------------------------------------------------
extern "C" void PIT_IRQHandler(void)
{
    /* Clear interrupt flag.*/
    PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);


    gTotalMillis++;
    for (unsigned i=0; i<numCallbacks; i++)
    {
        if (callbacks[i])
        {
            callbacks[i](gTotalMillis);
        }
    }
    for (unsigned i=0; i<kMaxTimeouts; i++)
    {
        uint32_t timeout = timeouts[i];
        if (timeout)
        {
            timeouts[i] = timeout - 1;
        }
    }
}

//------------------------------------------------------------------------------
void timer_init()
{
    // Init periodic timer interrupt (every 1000 microseconds) which will
    // execute our PIT_IRQHandler function above
    pit_config_t pitConfig;
    PIT_GetDefaultConfig(&pitConfig);   // sets: pitConfig.enableRunInDebug = false

    /* Init pit module */
    PIT_Init(PIT, &pitConfig);

    /* Set timer period for channel 0 */
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, USEC_TO_COUNT(1000U, CLOCK_GetFreq(kCLOCK_BusClk)));

    /* Enable timer interrupts for channel 0 */
    PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);

    /* Enable at the NVIC */
    EnableIRQ(PIT_IRQn);

    // Finally let us start the timer
    PIT_StartTimer(PIT, kPIT_Chnl_0);
}

//------------------------------------------------------------------------------
void timer_add_callback(milli_timer_cb_t callback)
{
    if (callback && (numCallbacks < kMaxCallbacks))
    {
        callbacks[numCallbacks] = callback;
        numCallbacks++;
    }
}

//------------------------------------------------------------------------------
void timer_update()
{
    // Nothing to do
}

//------------------------------------------------------------------------------
void timer_set_timeout(unsigned timerID, unsigned timeoutMillis)
{
    if (timerID < kMaxTimeouts)
    {
        timeouts[timerID] = timeoutMillis;
    }
}

//------------------------------------------------------------------------------
int timer_get_timeout(unsigned timerID)
{
    if (timerID < kMaxTimeouts)
    {
        return timeouts[timerID];
    }
    return 0;
}


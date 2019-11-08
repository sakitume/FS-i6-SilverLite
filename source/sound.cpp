#include "fsl_gpio.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "sound.h"
#include "drv_time.h"

#include "fsl_clock.h"
#include "fsl_tpm.h"

//#define __UPDATE_ON_ISR_THREAD__
#if defined(__UPDATE_ON_ISR_THREAD__)    
    #include "timer.h"
#endif

//------------------------------------------------------------------------------
// Buzzer is connected to TPM1_CH0
#define BOARD_TIMER_BASEADDR TPM1
#define BOARD_TIMER_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_McgPll0Clk)
#define TIMER_CLOCK_MODE 1U

//------------------------------------------------------------------------------
struct tone_t
{
    uint16_t 	frequency;
    int16_t 	duration_ms;
};
enum 	{	SOUND_QUEUE_SIZE = 10 };
static tone_t sound_queue[SOUND_QUEUE_SIZE];
static int sound_queue_state;
static uint8_t timerStarted;

#if defined(__UPDATE_ON_ISR_THREAD__)    
static volatile int sound_tone_duration;
#else
static int sound_tone_duration;
#endif

//------------------------------------------------------------------------------
// Note: This might be called by our millisecond ISR so be mindful!
static void sound_set_frequency(uint32_t freq)
{
    tpm_chnl_pwm_signal_param_t tpmParam;
    tpmParam.chnlNumber = (tpm_chnl_t)0;	// TPM1_CH0

    // TODO: I haven't physically inspected how the buzzer is wired.
    // Is it active high? Or active low?    
    tpmParam.level = kTPM_HighTrue;
//    tpmParam.level = kTPM_LowTrue;
   	tpmParam.dutyCyclePercent = 50;

	// TPM_SetupPwm() fails with freq values below approx 1500
	if (freq < 1500)
	{
		if (timerStarted)
		{
			timerStarted = false;
			TPM_StopTimer(BOARD_TIMER_BASEADDR);
		}
	}
	else
	{
		TPM_SetupPwm(BOARD_TIMER_BASEADDR, &tpmParam, 1, kTPM_EdgeAlignedPwm, freq, BOARD_TIMER_SOURCE_CLOCK);
		if (!timerStarted)
		{
			timerStarted = true;
		    TPM_StartTimer(BOARD_TIMER_BASEADDR, kTPM_SystemClock);
		}
	}
}

//------------------------------------------------------------------------------
static void InitPWM()
{
    /* Initialize TPM module */
    /*
     * tpmInfo->prescale = kTPM_Prescale_Divide_1;
     * tpmInfo->useGlobalTimeBase = false;
     * tpmInfo->enableDoze = false;
     * tpmInfo->enableDebugMode = false;
     * tpmInfo->enableReloadOnTrigger = false;
     * tpmInfo->enableStopOnOverflow = false;
     * tpmInfo->enableStartOnTrigger = false;
     * tpmInfo->enablePauseOnTrigger = false;
     * tpmInfo->triggerSelect = kTPM_Trigger_Select_0;
     * tpmInfo->triggerSource = kTPM_TriggerSource_External;
     */
    tpm_config_t tpmInfo;
    TPM_GetDefaultConfig(&tpmInfo);
    TPM_Init(BOARD_TIMER_BASEADDR, &tpmInfo);
    CLOCK_SetTpmClock(TIMER_CLOCK_MODE);
	sound_set_frequency(0);
}

//------------------------------------------------------------------------------
#if defined(__UPDATE_ON_ISR_THREAD__)    
static void on_milli_update(unsigned long millis)
{
    if (sound_queue_state == 0) {
        // off, return
        return;
    }

    if (sound_tone_duration <= 0) {

        // next sample
        uint32_t id = sound_queue_state - 1;
        if ((id == SOUND_QUEUE_SIZE) || (sound_queue[id].duration_ms == 0)) {
            // no more samples, switch off:
            sound_set_frequency(0);
            sound_queue_state = 0;
        } else {
            // fetch next sample
            sound_tone_duration = sound_queue[id].duration_ms;
            sound_set_frequency(sound_queue[id].frequency);
            sound_queue_state++;
        }
    } else {
        sound_tone_duration--;
    }
}
#endif

//------------------------------------------------------------------------------
void sound_update()
{
#if defined(__UPDATE_ON_ISR_THREAD__)    
#else
	static uint32_t	msLast;
	uint32_t msNow = millis_this_frame();
	unsigned delta = msNow - msLast;
	msLast = msNow;

    if (sound_queue_state == 0) {
        // off, return
        return;
    }

	sound_tone_duration -= delta;
    if (sound_tone_duration <= 0) {
		sound_tone_duration = 0;

        // next sample
        uint32_t id = sound_queue_state - 1;
        if ((id == SOUND_QUEUE_SIZE) || (sound_queue[id].duration_ms == 0)) {
            // no more samples, switch off:
            sound_set_frequency(0);
            sound_queue_state = 0;
        } else {
            // fetch next sample
            sound_tone_duration = sound_queue[id].duration_ms;
            sound_set_frequency(sound_queue[id].frequency);
            sound_queue_state++;
        }
    }
#endif    
}

//------------------------------------------------------------------------------
void sound_init()
{
	InitPWM();	

    sound_queue_state = 0;
    sound_set_frequency(0);
    sound_tone_duration = 0;

#if defined(__UPDATE_ON_ISR_THREAD__)    
    timer_add_callback(on_milli_update);
#endif
}

//------------------------------------------------------------------------------
void sound_play_bind(void) 
{
	sound_tone_duration = 0;

    sound_queue[0].frequency   = 2200;
    sound_queue[0].duration_ms = 100;
    sound_queue[1].frequency   = 1500;
    sound_queue[1].duration_ms = 100;
    sound_queue[2].duration_ms = 0;
    sound_queue_state = 1;
}

//------------------------------------------------------------------------------
void sound_play_click(void)
{
	sound_tone_duration = 0;

    sound_queue[0].frequency   = 20000;
    sound_queue[0].duration_ms = 120;
    sound_queue[1].duration_ms = 0;
    sound_queue_state = 1;
}

//------------------------------------------------------------------------------
void sound_play_low_time(void) 
{
	sound_tone_duration = 0;

    sound_queue[0].frequency   = 4000;
    sound_queue[0].duration_ms = 300;
    sound_queue[1].duration_ms = 0;
    sound_queue_state = 1;
}


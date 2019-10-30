#include "buttons.h"
#include "debug.h"
#include "console.h"
#include "pin_mux.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "drv_time.h"

//------------------------------------------------------------------------------
struct ButtonState_t
{
    uint8_t     debounce        : 4;    // 15 millisecond debounce countdown timer
    uint8_t     lastState       : 1;    // measured state last frame
    uint8_t     currState       : 1;    // measured state this frame
    uint8_t     debounceState   : 1;    // debounced state this frame
    uint8_t     toggled         : 1;    // Set if the state toggled this frame
};
static struct ButtonState_t buttons[4][3];  // only takes 12 bytes

#ifdef __MEASURE_UPDATE_TIME__
static unsigned long updateTime;
#endif

//------------------------------------------------------------------------------
// Buttons are configured as a 4x3 matrix. L1-L4 are rows, R1-R3 are columns
//
//      R1      R2          R3
// L1   Roll-R  Throttle-U  Down
// L1   Roll-L  Throttle-U  Up
// L3   Pitch-U Yaw-R       Ok
// L4   Pitch-D Yaw-L       Cancel
//
// R1-R3 button pins must be configured for input with an internal pulldown resistor.
//
// L1-L4 are pulled high with external 10k pullup resistors and configured for floating input
// (no pulldown or pullup). During button scan we'll temporarily switch them to output low.
// 
void buttons_init(void)
{
    // Configure R1 thru R3 as input pins with internal pulldown
    port_pin_config_t input_pulldown = {
        kPORT_PullDown,
        kPORT_FastSlewRate,
        kPORT_PassiveFilterDisable,
        kPORT_LowDriveStrength,
        kPORT_MuxAsGpio,
    };
#if 0    
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_R1_PORT, BOARD_INITPINS_BUTTON_R1_PIN, &input_pulldown); // PORTB, 1
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_R2_PORT, BOARD_INITPINS_BUTTON_R2_PIN, &input_pulldown); // PORTB, 2
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_R3_PORT, BOARD_INITPINS_BUTTON_R3_PIN, &input_pulldown); // PORTB, 3
#else
    PORT_SetMultiplePinsConfig(BOARD_INITPINS_BUTTON_R1_PORT, 0
        | (1 << BOARD_INITPINS_BUTTON_R1_PIN)
        | (1 << BOARD_INITPINS_BUTTON_R2_PIN)
        | (1 << BOARD_INITPINS_BUTTON_R3_PIN),
        &input_pulldown
    );        
#endif    

    // Configure L1 thru L4 as input pins with no pullup/pulldown
    port_pin_config_t input_pulldisabled = {
        kPORT_PullDisable,
        kPORT_FastSlewRate,
        kPORT_PassiveFilterDisable,
        kPORT_LowDriveStrength,
        kPORT_MuxAsGpio,
    };
#if 0    
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_L1_PORT, BOARD_INITPINS_BUTTON_L1_PIN, &input_pulldisabled);   // PORTB, 16
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_L2_PORT, BOARD_INITPINS_BUTTON_L2_PIN, &input_pulldisabled);   // PORTB, 17
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_L3_PORT, BOARD_INITPINS_BUTTON_L3_PIN, &input_pulldisabled);   // PORTB, 18
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_L4_PORT, BOARD_INITPINS_BUTTON_L4_PIN, &input_pulldisabled);   // PORTB, 19
#else
    PORT_SetMultiplePinsConfig(BOARD_INITPINS_BUTTON_L1_PORT, 0
        | (1 << BOARD_INITPINS_BUTTON_L1_PIN)
        | (1 << BOARD_INITPINS_BUTTON_L2_PIN)
        | (1 << BOARD_INITPINS_BUTTON_L3_PIN)
        | (1 << BOARD_INITPINS_BUTTON_L4_PIN),
        &input_pulldisabled
    );        
#endif    

    // Configure Bind, SwA and SwD pins as input (with internal pullup, but pulldown would also work)
    gpio_pin_config_t pin_config_input = {
        kGPIO_DigitalInput, 0
    };
    GPIO_PinInit(BOARD_INITPINS_Bind_GPIO, BOARD_INITPINS_Bind_PIN, &pin_config_input);       // PORTA, 5
    GPIO_PinInit(BOARD_INITPINS_SwA_GPIO, BOARD_INITPINS_SwA_PIN, &pin_config_input);         // PORTD, 0
    GPIO_PinInit(BOARD_INITPINS_SwD_GPIO, BOARD_INITPINS_SwD_PIN, &pin_config_input);         // PORTD, 4
}

//------------------------------------------------------------------------------
// Buttons are configured as a 4x3 matrix. L1-L4 are rows, R1-R3 are columns
//
//      R1      R2          R3
// L1   Roll-R  Throttle-U  Down
// L2   Roll-L  Throttle-U  Up
// L3   Pitch-U Yaw-R       Ok
// L4   Pitch-D Yaw-L       Cancel
//
//------------------------------------------------------------------------------
void buttons_update(void)
{
#ifdef __MEASURE_UPDATE_TIME__
    unsigned long usNow = micros_realtime();
#endif    

    static unsigned long lastUpdate;
    unsigned long now = millis_this_frame();
    unsigned long elapsed = now - lastUpdate;
    lastUpdate = now;

    // Reset toggled and currState for all buttons
    struct ButtonState_t *pButton = &buttons[0][0];
    for (int i=0; i<sizeof(buttons)/sizeof(buttons[0][0]); i++, pButton++)
    {
        pButton->toggled    = 0;
        pButton->currState  = 0;
    }

    // R1 is at bit 1, R2 is at bit 2, R3 is at bit 3
    // R1 bit mask would be: 0x02
    // R2 bit mask would be: 0x04
    // R3 bit mask would be: 0x08
    enum
    {
        kR1_Mask    = (1 << BOARD_INITPINS_BUTTON_R1_PIN),
        kR2_Mask    = (1 << BOARD_INITPINS_BUTTON_R2_PIN),
        kR3_Mask    = (1 << BOARD_INITPINS_BUTTON_R3_PIN)
    };

    // Get current state of the columns in our matrix (R1, R2, R3)
    const uint32_t R1R2R3 = BOARD_INITPINS_BUTTON_R1_GPIO->PDIR & (kR1_Mask|kR2_Mask|kR3_Mask);

    // For each row of the matrix (L1 thru L4 pins)
    for (int row=0; row<4; row++)
    {
        // Set row pin to be output being low
        gpio_pin_config_t pin_config_output = {
            kGPIO_DigitalOutput, 0
        };
        GPIO_PinInit(BOARD_INITPINS_BUTTON_L1_GPIO, BOARD_INITPINS_BUTTON_L1_PIN + row, &pin_config_output);

        // We must have a slight delay for pin inputs to stabilize before trying to read
// No: This is no longer needed now that I fixed row pins to not use internal pullups        
//        __NOP();

        // Read state of R1, R2, R3 pins and see if anything changed
        const uint32_t testR1R2R3 = BOARD_INITPINS_BUTTON_R1_GPIO->PDIR & (kR1_Mask|kR2_Mask|kR3_Mask);
        uint32_t diff = testR1R2R3 ^ R1R2R3;
        if (diff)
        {
            // It did change, this means one or more of the buttons for this row is active
            for (int col=0; col<3; col++)
            {
                struct ButtonState_t *pButton = &buttons[row][col];

                // If bit changed for this row/col combo then the button at this location is active
                const int bit = 1 << (BOARD_INITPINS_BUTTON_R1_PIN + col);
                pButton->currState = (bit & diff) ? 1 : 0;
            }
        }

        // Set row pin back to being an input
        gpio_pin_config_t pin_config_input = {
            kGPIO_DigitalInput, 0
        };
        GPIO_PinInit(BOARD_INITPINS_BUTTON_L1_GPIO, BOARD_INITPINS_BUTTON_L1_PIN + row, &pin_config_input);
    }

    // For each button
    pButton = &buttons[0][0];
    for (int i=0; i<sizeof(buttons)/sizeof(buttons[0][0]); i++, pButton++)
    {
        // If state has toggled then reload debounce countdown timer
        if (pButton->lastState != pButton->currState)
        {
            pButton->lastState = pButton->currState;
            pButton->debounce = 0xF;    // 0xF is max value that can be stored in this data member
        }
        else if (pButton->debounce) //..otherwise if button is being debounced
        {
            //...check if it has stabilized for the required amount of time
            int counter = pButton->debounce - (int)elapsed;
            if (counter <= 0)
            {
                pButton->debounce = 0;
                if (pButton->debounceState != pButton->currState)
                {
                    pButton->debounceState = pButton->currState;
                    pButton->toggled = 1;
                }
            }
            else
            {
                pButton->debounce = counter;
            }
        }
    }

#ifdef __MEASURE_UPDATE_TIME__
    // Testing reveals it takes 28us
    updateTime = micros_realtime() - usNow;
#endif    
}

//------------------------------------------------------------------------------
void buttons_test(void)
{
    console_clear();

    //     R1 R2 R3
    //  L1  0  0  0
    //  L2  0  0  0
    //  L3  0  0  0
    //  L4  0  0  0
    debug("   R1 R2 R3\n");
    for (int row=0; row<4; row++)
    {
        debug_putc('L');
        debug_put_uint8(row+1);
        debug("  ");
        for (int col=0; col<3; col++)
        {
            debug_put_uint8(buttons[row][col].debounceState);
            debug("  ");
        }
        debug("\n");
    }
    debug("Bind: ");
    debug_putc('0'+GPIO_ReadPinInput(BOARD_INITPINS_Bind_GPIO, BOARD_INITPINS_Bind_PIN));
    debug("\n");

    debug("SwA: ");
    debug_putc('0'+GPIO_ReadPinInput(BOARD_INITPINS_SwA_GPIO, BOARD_INITPINS_SwA_PIN));
    debug("  ");

    debug("SwD: ");
    debug_putc('0'+GPIO_ReadPinInput(BOARD_INITPINS_SwD_GPIO, BOARD_INITPINS_SwD_PIN));
    debug("\n");

#ifdef __MEASURE_UPDATE_TIME__
    // Previous runs of this test code show button_update() taking about 28 to 30 microseconds
    debug("Update: ");
    debug_put_hex16(updateTime);
    debug("\n");
#endif

    debug_flush();
}

int button_toggled(e_BtnIndex btnIndex)
{
    // Note: Treating buttons[][] as a single dimensional array
    return buttons[0][btnIndex].toggled;
}

int button_toggledActive(e_BtnIndex btnIndex)
{
    // Note: Treating buttons[][] as a single dimensional array
    return buttons[0][btnIndex].toggled && buttons[0][btnIndex].debounceState;
}

int button_active(e_BtnIndex btnIndex)
{
    // Note: Treating buttons[][] as a single dimensional array
    return buttons[0][btnIndex].debounceState;
}

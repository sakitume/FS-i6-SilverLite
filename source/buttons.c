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
// L1-L4 are pulled high with external 10k pullup resistors and configured for output
//
// If any of the Throttle or Yaw trim buttons are depressed, then R2 goes high.
// If any of the Roll or Pitch trim buttons are depressed, then R1 goes high.
// If Down, Up, Ok or Cancel buttons are depressed then R3 goes high
//
void buttons_init(void)
{
    // Must configure R1 thru R3 input pins to use internal pulldown and not pullup    
    port_pin_config_t port_pin_config = {
        kPORT_PullDown,
        kPORT_FastSlewRate,
        kPORT_PassiveFilterDisable,
        kPORT_LowDriveStrength,
        kPORT_MuxAsGpio,
    };

    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_R1_PORT, BOARD_INITPINS_BUTTON_R1_PIN, &port_pin_config);
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_R1_PORT, BOARD_INITPINS_BUTTON_R1_PIN, &port_pin_config); // PORTB, 1
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_R2_PORT, BOARD_INITPINS_BUTTON_R2_PIN, &port_pin_config); // PORTB, 2
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_R3_PORT, BOARD_INITPINS_BUTTON_R3_PIN, &port_pin_config); // PORTB, 3

    // Configure L1 thru L4 as output pins with initial state of HIGH
    gpio_pin_config_t pin_config_output = {
        kGPIO_DigitalOutput, 1
    };
    GPIO_PinInit(BOARD_INITPINS_BUTTON_L1_GPIO, BOARD_INITPINS_BUTTON_L1_PIN, &pin_config_output); // PORTB, 16
    GPIO_PinInit(BOARD_INITPINS_BUTTON_L2_GPIO, BOARD_INITPINS_BUTTON_L2_PIN, &pin_config_output); // PORTB, 17
    GPIO_PinInit(BOARD_INITPINS_BUTTON_L3_GPIO, BOARD_INITPINS_BUTTON_L3_PIN, &pin_config_output); // PORTB, 18
    GPIO_PinInit(BOARD_INITPINS_BUTTON_L4_GPIO, BOARD_INITPINS_BUTTON_L4_PIN, &pin_config_output); // PORTB, 19

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
// R1-R3 button pins must be configured for input with an internal pulldown resistor.
// L1-L4 are pulled high with external 10k pullup resistors and configured for output
//
// 
// If any of the Throttle or Yaw trim buttons are depressed, then R2 goes high.
// If any of the Roll or Pitch trim buttons are depressed, then R1 goes high.
// If Down, Up, Ok or Cancel buttons are depressed then R3 goes high
//
//------------------------------------------------------------------------------
static unsigned long updateTime;
void buttons_update(void)
{
    unsigned long usNow = micros_realtime();

    static unsigned long lastUpdate;
    unsigned long now = millis_this_frame();
    unsigned long elapsed = now - lastUpdate;
    lastUpdate = now;

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
        // Set row pin LOW
        const int pin = BOARD_INITPINS_BUTTON_L1_PIN + row;
        BOARD_INITPINS_BUTTON_L1_GPIO->PCOR = 1 << pin;

        // We must have a slight delay for pin inputs to stabilize before trying to read
        delay_us(1);

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
        else
        {
            for (int col=0; col<3; col++)
            {
                buttons[row][col].currState = 0;
            }
        }

        // Set pin back to HIGH
        BOARD_INITPINS_BUTTON_L1_GPIO->PSOR = 1 << pin;
    }

    // For each button
    struct ButtonState_t *pButton = &buttons[0][0];
    for (int i=0; i<sizeof(buttons)/sizeof(buttons[0][0]); i++, pButton++)
    {
        pButton->toggled    = 0;

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

    updateTime = micros_realtime() - usNow;
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
    debug("\n");

    debug("Bind: ");
    debug_putc('0'+GPIO_ReadPinInput(BOARD_INITPINS_Bind_GPIO, BOARD_INITPINS_Bind_PIN));
    debug("\n");

    debug("SwA: ");
    debug_putc('0'+GPIO_ReadPinInput(BOARD_INITPINS_SwA_GPIO, BOARD_INITPINS_SwA_PIN));
    debug("  ");

    debug("SwD: ");
    debug_putc('0'+GPIO_ReadPinInput(BOARD_INITPINS_SwD_GPIO, BOARD_INITPINS_SwD_PIN));
    debug("\n");

    debug("Update: ");
    debug_put_hex16(updateTime);
    debug("\n");

    debug_flush();
}

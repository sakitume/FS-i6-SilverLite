#include "buttons.h"
#include "debug.h"
#include "console.h"
#include "pin_mux.h"
#include "fsl_gpio.h"

#include "fsl_port.h"

//------------------------------------------------------------------------------
void buttons_init(void)
{
#if 1   // Must configure input pins to use internal pulldown and not pullup    
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

    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_L1_PORT, BOARD_INITPINS_BUTTON_L1_PIN, &port_pin_config); // PORTB, 16
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_L2_PORT, BOARD_INITPINS_BUTTON_L2_PIN, &port_pin_config); // PORTB, 17
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_L3_PORT, BOARD_INITPINS_BUTTON_L3_PIN, &port_pin_config); // PORTB, 18
    PORT_SetPinConfig(BOARD_INITPINS_BUTTON_L4_PORT, BOARD_INITPINS_BUTTON_L4_PIN, &port_pin_config); // PORTB, 19

    PORT_SetPinConfig(BOARD_INITPINS_Bind_PORT, BOARD_INITPINS_Bind_PIN, &port_pin_config);       // PORTA, 5
    PORT_SetPinConfig(BOARD_INITPINS_SwA_PORT, BOARD_INITPINS_SwA_PIN, &port_pin_config);         // PORTD, 0
    PORT_SetPinConfig(BOARD_INITPINS_SwD_PORT, BOARD_INITPINS_SwD_PIN, &port_pin_config);         // PORTD, 4
#else    
    gpio_pin_config_t pin_config = {
        kGPIO_DigitalInput, 0,
    };

    GPIO_PinInit(BOARD_INITPINS_BUTTON_R1_GPIO, BOARD_INITPINS_BUTTON_R1_PIN, &pin_config); // PORTB, 1
    GPIO_PinInit(BOARD_INITPINS_BUTTON_R2_GPIO, BOARD_INITPINS_BUTTON_R2_PIN, &pin_config); // PORTB, 2
    GPIO_PinInit(BOARD_INITPINS_BUTTON_R3_GPIO, BOARD_INITPINS_BUTTON_R3_PIN, &pin_config); // PORTB, 3

    GPIO_PinInit(BOARD_INITPINS_BUTTON_L1_GPIO, BOARD_INITPINS_BUTTON_L1_PIN, &pin_config); // PORTB, 16
    GPIO_PinInit(BOARD_INITPINS_BUTTON_L2_GPIO, BOARD_INITPINS_BUTTON_L2_PIN, &pin_config); // PORTB, 17
    GPIO_PinInit(BOARD_INITPINS_BUTTON_L3_GPIO, BOARD_INITPINS_BUTTON_L3_PIN, &pin_config); // PORTB, 18
    GPIO_PinInit(BOARD_INITPINS_BUTTON_L4_GPIO, BOARD_INITPINS_BUTTON_L4_PIN, &pin_config); // PORTB, 19

    GPIO_PinInit(BOARD_INITPINS_Bind_GPIO, BOARD_INITPINS_Bind_PIN, &pin_config);       // PORTA, 5
    GPIO_PinInit(BOARD_INITPINS_SwA_GPIO, BOARD_INITPINS_SwA_PIN, &pin_config);         // PORTD, 0
    GPIO_PinInit(BOARD_INITPINS_SwD_GPIO, BOARD_INITPINS_SwD_PIN, &pin_config);         // PORTD, 4
#endif    
}

//------------------------------------------------------------------------------
void buttons_update(void)
{
    // TODO, perform any debouncing here
}

//------------------------------------------------------------------------------
void buttons_test(void)
{
    console_clear();

    // R1 thru R3 are sequential and on PORTB
    debug("R: 1 2 3\n");
    debug("   ");
    int pin = BOARD_INITPINS_BUTTON_R1_PIN;
    for (int i=0; i<3; i++)
    {
        debug_putc('0' + GPIO_ReadPinInput(BOARD_INITPINS_BUTTON_R1_GPIO, pin+i));
        debug_putc(' ');
    }
    debug("\n");

    debug("L: 1 2 3 4\n");
    debug("   ");
    pin = BOARD_INITPINS_BUTTON_L1_PIN;
    for (int i=0; i<4; i++)
    {
        unsigned state = GPIO_ReadPinInput(BOARD_INITPINS_BUTTON_L1_GPIO, pin+i);
        debug_putc('0'+state);
        debug_putc(' ');
    }
    debug("\n");

    debug("Bind: ");
    debug_putc('0'+GPIO_ReadPinInput(BOARD_INITPINS_Bind_GPIO, BOARD_INITPINS_Bind_PIN));
    debug("\n");

    debug("SwA: ");
    debug_putc('0'+GPIO_ReadPinInput(BOARD_INITPINS_SwA_GPIO, BOARD_INITPINS_SwA_PIN));
    debug("\n");

    debug("SwD: ");
    debug_putc('0'+GPIO_ReadPinInput(BOARD_INITPINS_SwD_GPIO, BOARD_INITPINS_SwD_PIN));
    debug("\n");

    debug_flush();
}

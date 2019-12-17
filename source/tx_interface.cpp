#include "tx_interface.h"
#include "multiprotocol.h"

// Voltage of flight controller
uint16_t tx_get_fc_voltage()
{
    return 400;
}

// RSSI of tx receiving from flight controller
uint16_t tx_get_rssi_tx()
{
    return 100;
}

// RSSI reported by flight controller
uint16_t tx_get_rssi_fc()
{
    return 100;
}



void tx_update(){}
void tx_start()
{
    multiprotocol_enable();
}

void tx_stop()  
{
    multiprotocol_disable();
}

void tx_reset()
{
    multiprotocol_rebind();
}


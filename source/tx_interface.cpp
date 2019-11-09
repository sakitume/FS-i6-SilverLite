#include "tx_interface.h"

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
void tx_start() {}
void tx_stop()  {}
void tx_reset() {}


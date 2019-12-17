#include "tx_interface.h"
#include "multiprotocol.h"

// Voltage of flight controller
uint16_t tx_get_fc_voltage()
{
//    return multiprotocol_get_telemetry(0) * 2;
    return multiprotocol_get_telemetry(1) * 2;
}

// RSSI of tx receiving from flight controller
uint16_t tx_get_rssi_tx()
{
    return multiprotocol_get_telemetry(2);
}

// RSSI reported by flight controller
uint16_t tx_get_rssi_fc()
{
    return multiprotocol_get_telemetry(3);
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


#ifndef __TX_INTERFACE_H__
#define __TX_INTERFACE_H__

#include <stdint.h>

// Voltage of flight controller
uint16_t tx_get_fc_voltage();

// RSSI of tx receiving from flight controller
uint16_t tx_get_rssi_tx();

// RSSI reported by flight controller
uint16_t tx_get_rssi_fc();

void tx_start();
void tx_stop();
void tx_reset();


#endif // #ifndef __TX_INTERFACE_H__

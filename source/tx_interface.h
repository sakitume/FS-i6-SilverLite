#ifndef __TX_INTERFACE_H__
#define __TX_INTERFACE_H__

#include <stdint.h>

// Voltage of flight controller
uint16_t tx_get_fc_voltage();

// RSSI of tx receiving from flight controller
uint16_t tx_get_rssi_tx();

// RSSI reported by flight controller
uint16_t tx_get_rssi_fc();

// PID for roll pitch yaw
// fixed point with 3 decimal digits after the point
//  kp[roll, pitch, yaw]
//  ki[roll, pitch, yaw]
//  kd[roll, pitch, yaw]
uint16_t tx_get_pid(int row, int col);

void tx_update();
void tx_start();
void tx_stop();
void tx_reset();


#endif // #ifndef __TX_INTERFACE_H__

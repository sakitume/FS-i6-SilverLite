#ifndef __BAYANG_H__
#define __BAYANG_H__
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>

//------------------------------------------------------------------------------
void Bayang_tx_test();
void Bayang_tx_test2();
void Bayang_rx_test();

//------------------------------------------------------------------------------
enum ESubProtocol
{
    kDisabled,
    kStock,
    kSilverware,
    kSilverwareTelemetry
};
void Bayang_init();
void Bayang_tx_reset(enum ESubProtocol protocol);
void Bayang_tx_update();
void Bayang_tx_ui();
void Bayang_disable();

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif // #ifndef __BAYANG_H__

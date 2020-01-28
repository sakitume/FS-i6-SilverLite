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
enum EProtocol
{
    kBayangDisabled,
    kBayangStock,
    kBayangSilverware
};

enum EBayangOptionFlags
{
	BAYANG_OPTION_FLAG_TELEMETRY	= 0x01,
	BAYANG_OPTION_FLAG_ANALOGAUX	= 0x02,
    BAYANG_OPTION_FLAG_SILVERLITE   = 0x04
};

void Bayang_init();
void Bayang_tx_reset(enum EProtocol protocol, uint8_t options);
void Bayang_tx_update();
void Bayang_tx_ui();
void Bayang_disable();

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif // #ifndef __BAYANG_H__

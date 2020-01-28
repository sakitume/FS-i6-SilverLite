#include "tx_interface.h"
#include "multiprotocol.h"
#include "storage.h"
#include "config.h"

#if defined(__USING_INTERNAL_TRX__)
    #include "bayang.h"
    #include "bayang_common.h"
    #include "drv_time.h"
    #include "sound.h"
#endif

static bool usingInternal()
{
    const ModelDesc_t &model = storage.model[storage.current_model];
    return (0xFF == model.mpm_protocol);
}

// Voltage of flight controller
uint16_t tx_get_fc_voltage()
{
#if defined(__USING_INTERNAL_TRX__)
    if (usingInternal())
    {
//      return gSilverLiteData.vbattFilt;
        return gSilverLiteData.vbattComp;
    }
#endif    

//  return multiprotocol_get_telemetry(0) * 2;
    return multiprotocol_get_telemetry(1) * 2;
}

// RSSI of tx receiving from flight controller
uint16_t tx_get_rssi_tx()
{
#if defined(__USING_INTERNAL_TRX__)
    if (usingInternal())
    {
        // If timeout value hasn't expired then we can return tlmRatio
        return gTXContext.telemetryTimeout ? gTXContext.tlmRatio : 0;
    }
#endif    

    return multiprotocol_get_telemetry(6) * 2;
}

// RSSI reported by flight controller
uint16_t tx_get_rssi_fc()
{
#if defined(__USING_INTERNAL_TRX__)
    if (usingInternal())
    {
        // If timeout value hasn't expired then we can return PPS
        return gTXContext.telemetryTimeout ? gSilverLiteData.pktsPerSec : 0;
    }
#endif
    return multiprotocol_get_telemetry(5) * 2;
}



void tx_update()
{
#if defined(__USING_INTERNAL_TRX__)
    if (usingInternal())
    {
        Bayang_tx_update();
        return;
    }
#endif
}

void tx_start()
{
#if defined(__USING_INTERNAL_TRX__)
    if (usingInternal())
    {
        Bayang_init();
        Bayang_tx_reset(kSilverwareTelemetry);
        return;
    }
#endif
    multiprotocol_enable();
}

void tx_stop()  
{
#if defined(__USING_INTERNAL_TRX__)
    if (usingInternal())
    {
        Bayang_disable();
        return;
    }
#endif
    multiprotocol_disable();
}

void tx_reset()
{
#if defined(__USING_INTERNAL_TRX__)
    if (usingInternal())
    {
        Bayang_tx_reset(kSilverwareTelemetry);
        return;
    }
#endif
    multiprotocol_rebind();
}


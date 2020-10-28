#include "tx_interface.h"
#include "multiprotocol.h"
#include "storage.h"
#include "config.h"
#include "multiprotocol_enums.h"
#include "buttons.h"
#include "silverlite_data.h"

#if defined(__USING_INTERNAL_TRX__)
    #include "bayang.h"
    #include "bayang_common.h"
    #include "drv_time.h"
    #include "sound.h"
#endif

//------------------------------------------------------------------------------
static bool usingInternal()
{
    const ModelDesc_t &model = storage.model[storage.current_model];
    return (0xFF == model.mpm_protocol);
}

//------------------------------------------------------------------------------
static bool silverLiteDataIsRecent()
{
    // return true if gSilverLiteData was updated within the last 100 milliseconds
    return (millis_this_frame() - gSilverLiteData.lastUpdate) <= 100;
}

//------------------------------------------------------------------------------
// Voltage of flight controller
uint16_t tx_get_fc_voltage()
{
    //
    if (silverLiteDataIsRecent())
    {
        return gSilverLiteData.vbattFilt;
//        return gSilverLiteData.vbattComp;
    }
    return 0;
}

//------------------------------------------------------------------------------
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

    //
    if (silverLiteDataIsRecent())
    {
//        return gSilverLiteData.tlmCount;
        return gSilverLiteData.tlmPPS;
    }
    return 0;
}

//------------------------------------------------------------------------------
// RSSI reported by flight controller
uint16_t tx_get_rssi_fc()
{
    if (silverLiteDataIsRecent())
    {
        return gSilverLiteData.pktsPerSec;
    }
    return 0;
}

//------------------------------------------------------------------------------
uint16_t tx_get_pid(int row, int col)
{
    // if gSilverLiteData PID data was updated within the last 100 milliseconds
    if ((millis_this_frame() - gSilverLiteData.lastPIDUpdate) <= 100)
    {
        switch (row)
        {
            case 0: return gSilverLiteData.P[col];
            case 1: return gSilverLiteData.I[col];
            case 2: return gSilverLiteData.D[col];
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
void tx_update()
{
    gSilverLiteData.update();

#if defined(__USING_INTERNAL_TRX__)
    if (usingInternal())
    {
        Bayang_tx_update();
        return;
    }
#endif
}

//------------------------------------------------------------------------------
#if defined(__USING_INTERNAL_TRX__)
static void internalReset(bool bInitialize)
{
    if (bInitialize)
    {
        Bayang_init();
    }

    const ModelDesc_t &model = storage.model[storage.current_model];
    EProtocol protocol = kBayangStock;
    switch (model.mpm_sub_protocol)
    {
        case 1:
            protocol = kBayangSilverware;
            break;

        case 2: // LT8900, not merged yet, so fall through
        
        default:
        case 0: protocol = kBayangStock;

    }
    Bayang_tx_reset(protocol, model.mpm_option);
}
#endif

//------------------------------------------------------------------------------
void tx_start()
{
    gSilverLiteData.reset();

#if defined(__USING_INTERNAL_TRX__)
    if (usingInternal())
    {
        internalReset(true);
        return;
    }
#endif
    multiprotocol_enable();
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void tx_reset()
{
    gSilverLiteData.reset();
#if defined(__USING_INTERNAL_TRX__)
    if (usingInternal())
    {
        internalReset(false);
        return;
    }
#endif
    multiprotocol_rebind();
}


#include "switchID.h"
#include "adc.h"
#include "buttons.h"

const char *gSwitchNames[_kSw_Max] =
{
    "None",
    "SwA1",
    "SwA2",
    "SwB1",
    "SwB2",
    "SwC1",
    "SwC2",
    "SwC3",
    "SwC1+2",
    "SwC2+3",
    "SwD1",
    "SwD2"
};

int switchIsActive(uint8_t id)
{
    switch (id)
    {
        case kSwA_1:
            return !button_active(kBtn_SwA);

        case kSwA_2:
            return button_active(kBtn_SwA);
        
        case kSwB_1:
            return adc_get_channel_calibrated(ADC_ID_SwB) < 100;

        case kSwB_2:
            return adc_get_channel_calibrated(ADC_ID_SwB) >= 100;

        case kSwC_1:
            return adc_get_channel_calibrated(ADC_ID_SwC) < 100;

        case kSwC_2:
        {
            uint16_t SwC = adc_get_channel_calibrated(ADC_ID_SwC);
            return (SwC >= 412) && (SwC < 923);
        }
        break;

        case kSwC_3:
            return adc_get_channel_calibrated(ADC_ID_SwC) >= 923;

        case kSwC_1_2:
            return adc_get_channel_calibrated(ADC_ID_SwC) < 923;

        case kSwC_2_3:
            return adc_get_channel_calibrated(ADC_ID_SwC) >= 412;

        case kSwD_1:
            return !button_active(kBtn_SwD);

        case kSwD_2:
            return button_active(kBtn_SwD);

        case kSw_None:
        default:
            return 0;
    }
}

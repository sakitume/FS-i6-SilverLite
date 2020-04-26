#include "debug.h"
#include "console.h"
#include "GEM.h"
#include "storage.h"
#include "config.h"
#include <string.h>
#include "switchID.h"
#include "bayang_chan.h"

extern GEM gGEM;
GEMPage menuPageBayangChans("Bayang Channel");

static void saveAction();
static SelectOptionByte selectSwitchOptions[_kSw_Max];
static GEMSelect selectSwitch(sizeof(selectSwitchOptions)/sizeof(selectSwitchOptions[0]), selectSwitchOptions);
static uint8_t BayangChanToAuxChanMap[_CH_Max];


static GEMItem miSelect_CH_INV(gBayangChanNames[CH_INV], BayangChanToAuxChanMap[CH_INV], selectSwitch, saveAction);
static GEMItem miSelect_CH_VID(gBayangChanNames[CH_VID], BayangChanToAuxChanMap[CH_VID], selectSwitch, saveAction);
static GEMItem miSelect_CH_PIC(gBayangChanNames[CH_PIC], BayangChanToAuxChanMap[CH_PIC], selectSwitch, saveAction);
static GEMItem miSelect_CH_TO(gBayangChanNames[CH_TO], BayangChanToAuxChanMap[CH_TO], selectSwitch, saveAction);
static GEMItem miSelect_CH_EMG(gBayangChanNames[CH_EMG], BayangChanToAuxChanMap[CH_EMG], selectSwitch, saveAction);
static GEMItem miSelect_CH_FLIP(gBayangChanNames[CH_FLIP], BayangChanToAuxChanMap[CH_FLIP], selectSwitch, saveAction);
static GEMItem miSelect_CH_HEADFREE(gBayangChanNames[CH_HEADFREE], BayangChanToAuxChanMap[CH_HEADFREE], selectSwitch, saveAction);
static GEMItem miSelect_CH_RTH(gBayangChanNames[CH_RTH], BayangChanToAuxChanMap[CH_RTH], selectSwitch, saveAction);

void gui_init_bayang_chans(GEMPage &parentMenuPage)
{
    for (int i=0; i<_kSw_Max; i++)
    {
        SelectOptionByte &opt = selectSwitchOptions[i];
        opt.name = gSwitchNames[i];
        opt.val_byte = i;
    }

    const uint8_t *mapping = storage.model[storage.current_model].bayangChans;
    for (int i=0; i<_CH_Max; i++)
    {
        BayangChanToAuxChanMap[i] = mapping[i];
    }

    // Note: The order these are added must match the EBayangCHan enumeration list
    menuPageBayangChans.addMenuItem(miSelect_CH_INV);
    menuPageBayangChans.addMenuItem(miSelect_CH_VID);
    menuPageBayangChans.addMenuItem(miSelect_CH_PIC);
    menuPageBayangChans.addMenuItem(miSelect_CH_TO);
    menuPageBayangChans.addMenuItem(miSelect_CH_EMG);
    menuPageBayangChans.addMenuItem(miSelect_CH_FLIP);
    menuPageBayangChans.addMenuItem(miSelect_CH_HEADFREE);
    menuPageBayangChans.addMenuItem(miSelect_CH_RTH);

    menuPageBayangChans.setParentMenuPage(parentMenuPage);
}

static void saveAction()
{
    ModelDesc_t &model = storage.model[storage.current_model];
    const int chIndex = gGEM.getValueSelectIndex();
    model.bayangChans[chIndex] = BayangChanToAuxChanMap[chIndex];
    storage_save();
}


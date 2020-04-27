#include "debug.h"
#include "console.h"
#include "GEM.h"
#include "storage.h"
#include "config.h"
#include <string.h>
#include "multiprotocol_enums.h"

extern GEM gGEM;
extern GEMPage menuPageMain;

GEMPage menuPageEditModel("Edit Model");

// gui_config_chans_bayang.cpp
extern void gui_init_bayang_chans(GEMPage &parentMenuPage);
extern GEMPage menuPageBayangChans;
GEMItem menuItem_ConfigBayangChans("Bayang Channels", menuPageBayangChans);

extern GEMSelect selectSwitch;

static void saveArmSwitch();
static uint8_t armSwitch;
GEMItem menuItem_ArmSwitch("Arm Switch:", armSwitch, selectSwitch, saveArmSwitch);
static void saveArmSwitch()
{
    ModelDesc_t &model = storage.model[storage.current_model];
    model.armSwitch = armSwitch;
    storage_save();
}

static void applyProtocol();
static uint8_t protocol;
static SelectOptionByte selectProtocolOptions[] = 
{
#if defined(__USING_INTERNAL_TRX__)    
    {"Internl", 0xFF},  // 0xFF means use internal TRX
#endif    
    {"Bayang", PROTO_BAYANG},
    {"FSky 2A", PROTO_AFHDS2A},

    {"CX10", PROTO_CX10},
    {"H8 3D", PROTO_H8_3D},
    {"KN", PROTO_KN},
    {"SymaX", PROTO_SYMAX},
    {"V2x2", PROTO_V2X2},

    {"SilvrLt", PROTO_SILVERLITE}
};
static GEMSelect selectProtocol(sizeof(selectProtocolOptions)/sizeof(selectProtocolOptions[0]), selectProtocolOptions);
static GEMItem miSelectProtocol("Protocol:", protocol, selectProtocol, applyProtocol);

static void applySubprotocol();
static uint8_t subprotocol;
#if defined(__USING_INTERNAL_TRX__)    
static SelectOptionByte InternalSubprotocolOptions[] = 
{
    {"Stock", 0},
    {"SilvrWr", 1},
    {"LT8900", 2},
    {nullptr,}
};
#endif
static SelectOptionByte BayangSubprotocolOptions[] = 
{
    {"BAYANG", BAYANG},
    {"H8S3D", H8S3D},
    {"X16_AH", X16_AH},
    {"IRDRONE", IRDRONE},
    {"DHD_D4", DHD_D4},
    {"LT8900", LT8900},
    {nullptr,}
};
static SelectOptionByte FSky2ASubprotocolOptions[] = 
{
    {"PWM_IBUS", PWM_IBUS},
    {"PPM_IBUS", PPM_IBUS},
    {"PWM_SBUS", PWM_SBUS},
    {"PPM_SBUS", PPM_SBUS},
    {nullptr,}
};

struct Subprotocols_t
{
    uint8_t             protocol;
    SelectOptionByte*   options;
};
static Subprotocols_t subprotocols[] =
{
#if defined(__USING_INTERNAL_TRX__)    
    { 0xFF, InternalSubprotocolOptions },
#endif    
    { PROTO_BAYANG, BayangSubprotocolOptions },
    { PROTO_AFHDS2A, FSky2ASubprotocolOptions },

    // TODO. Should add subprotocols for PROTO_CX10, PROTO_H8_3D, PROTO_KN, PROTO_SYMAX, PROTO_V2X2

    { PROTO_SILVERLITE, BayangSubprotocolOptions }  // SilverLite is Bayang with special extensions/additions
};

static GEMSelect selectSubprotocol(sizeof(BayangSubprotocolOptions)/sizeof(BayangSubprotocolOptions[0]), BayangSubprotocolOptions);
static GEMItem miSelectSubprotocol("Subprotocol:", subprotocol, selectSubprotocol, applySubprotocol);

static void applyProtocolOption();
static uint8_t protocolOption;
static GEMItem miProtocolOption("Option:", protocolOption, applyProtocolOption);

static void applyAutoBind();
static bool autoBind;
static GEMItem miAutoBind("Auto-bind:", autoBind, applyAutoBind);

static void applyRXNum();
static uint8_t rxNum;
static GEMItem miRXNum("RX Num:", rxNum, applyRXNum);

static void applySecondsTimer();
static int secondsTimer;
static GEMItem miSeconds("Seconds:", secondsTimer, applySecondsTimer);

static void applyModelName();
static char modelName[GEM_STR_LEN];
static GEMItem miEditName("Name:", modelName, applyModelName);

static void applySecondsTimer()
{
    storage.model[storage.current_model].timer = secondsTimer;
    storage_save();
}

static void applyAutoBind()
{
    storage.model[storage.current_model].mpm_auto_bind = autoBind ? 1 : 0;
    storage_save();
}

static void applyRXNum()
{
    storage.model[storage.current_model].mpm_rx_num = rxNum;
    storage_save();
}


static void applyProtocolOption()
{
    storage.model[storage.current_model].mpm_option = protocolOption;
    storage_save();
}


// Call this when protocol changes so we can update the subprotocol parameters
static void updateSubprotocolParams(uint8_t newProtocol)
{
    selectSubprotocol.changeOptions(0, nullptr);
    for (unsigned int i=0; i<sizeof(subprotocols)/sizeof(subprotocols[0]); i++)
    {
        if (subprotocols[i].protocol == newProtocol)
        {
            int numOptions = 0;
            while (subprotocols[i].options[numOptions].name != nullptr)
            {
                numOptions++;
            }
            subprotocol = subprotocols[i].options[0].val_byte;
            protocolOption = 0;
            autoBind = true;
            selectSubprotocol.changeOptions(numOptions, subprotocols[i].options);
            break;
        }
    }
}

static void applyProtocol()
{
    ModelDesc_t &model = storage.model[storage.current_model];

    // Call updateSubprotocolParams() to update various parameters
    // (menu stuff, but also: subprotocol and protocolOption)
    updateSubprotocolParams(protocol);

    //
    model.mpm_protocol = protocol;
    model.mpm_sub_protocol = subprotocol;
    model.mpm_option = protocolOption;
    model.mpm_auto_bind = autoBind;

    storage_save();
}

static void applySubprotocol()
{
    storage.model[storage.current_model].mpm_sub_protocol = subprotocol;
    storage_save();
}


static void copyNameWithPadding(char *dst, int dstBuffSize, const char *src)
{
    for (int i=0; i<dstBuffSize-1; i++)
    {
        char ch = *src;
        if (!ch)
        {
            ch = ' ';
        }
        else
        {
            src++;
        }
        dst[i] = ch;
    }
    dst[dstBuffSize-1] = '\0';
}

void gui_init_edit_model_properties()
{
    ModelDesc_t &model = storage.model[storage.current_model];
    copyNameWithPadding(modelName, ModelDesc_t::kModelNameLen, model.name);
    secondsTimer = model.timer;
    protocol = model.mpm_protocol;
    updateSubprotocolParams(protocol);
    // updateSubprotocolParams() will change subprotocol and protocolOption, so we
    // must be sure to initialize subprotocol *after* calling updateSubprotocolParams()
    subprotocol = model.mpm_sub_protocol;
    protocolOption = model.mpm_option;
    autoBind = model.mpm_auto_bind ? 1 : 0;
    rxNum = model.mpm_rx_num;

    armSwitch = model.armSwitch;
    extern void gui_init_edit_model_properties_bayang_chans();
    gui_init_edit_model_properties_bayang_chans();
}

static void applyModelName()
{
    ModelDesc_t &model = storage.model[storage.current_model];
    copyNameWithPadding(model.name, ModelDesc_t::kModelNameLen, modelName);

    // TODO, XXX: More weirdness with GEM. It will remove trailing whitespace
    // of the model name that was just edited. This will cause problems when
    // we try to re-edit this same model. So copy the name over once again
    // and ensure the whitespace padding is added
    copyNameWithPadding(modelName, ModelDesc_t::kModelNameLen, model.name);

    storage_save();
}


void gui_init_edit_model()
{
    // Need to initialize the variables that our various menu items reference
    gui_init_edit_model_properties();

    miEditName.setTypeToSimpleString();
    menuPageEditModel.addMenuItem(miEditName); 
    menuPageEditModel.addMenuItem(miSeconds);
    menuPageEditModel.addMenuItem(miSelectProtocol);
    menuPageEditModel.addMenuItem(miSelectSubprotocol);
    menuPageEditModel.addMenuItem(miProtocolOption);
    menuPageEditModel.addMenuItem(miAutoBind);
    menuPageEditModel.addMenuItem(miRXNum);
    menuPageEditModel.addMenuItem(menuItem_ArmSwitch);

    gui_init_bayang_chans(menuPageEditModel);
    menuPageEditModel.addMenuItem(menuItem_ConfigBayangChans);

    menuPageEditModel.setParentMenuPage(menuPageMain);
}

#include "debug.h"
#include "console.h"
#include "GEM.h"
#include "storage.h"
#include "config.h"
#include <string.h>

extern GEM gGEM;
extern GEMPage menuPageMain;

GEMPage menuPageModels("Models");

//-- From Multiprotocol.h ------------------------------------------------------
enum PROTOCOLS
{
    MODE_SERIAL		= 0,	// Serial commands
    PROTO_FLYSKY 	= 1,	// =>A7105
    PROTO_HUBSAN	= 2,	// =>A7105
    PROTO_FRSKYD	= 3,	// =>CC2500
    PROTO_HISKY		= 4,	// =>NRF24L01
    PROTO_V2X2		= 5,	// =>NRF24L01
    PROTO_DSM		= 6,	// =>CYRF6936
    PROTO_DEVO		= 7,	// =>CYRF6936
    PROTO_YD717		= 8,	// =>NRF24L01
    PROTO_KN		= 9,	// =>NRF24L01
    PROTO_SYMAX		= 10,	// =>NRF24L01
    PROTO_SLT		= 11,	// =>NRF24L01
    PROTO_CX10		= 12,	// =>NRF24L01
    PROTO_CG023		= 13,	// =>NRF24L01
    PROTO_BAYANG	= 14,	// =>NRF24L01
    PROTO_FRSKYX	= 15,	// =>CC2500
    PROTO_ESKY		= 16,	// =>NRF24L01
    PROTO_MT99XX	= 17,	// =>NRF24L01
    PROTO_MJXQ		= 18,	// =>NRF24L01
    PROTO_SHENQI	= 19,	// =>NRF24L01
    PROTO_FY326		= 20,	// =>NRF24L01
    PROTO_SFHSS		= 21,	// =>CC2500
    PROTO_J6PRO		= 22,	// =>CYRF6936
    PROTO_FQ777		= 23,	// =>NRF24L01
    PROTO_ASSAN		= 24,	// =>NRF24L01
    PROTO_FRSKYV	= 25,	// =>CC2500
    PROTO_HONTAI	= 26,	// =>NRF24L01
    PROTO_OPENLRS	= 27,	// =>OpenLRS hardware
    PROTO_AFHDS2A	= 28,	// =>A7105
    PROTO_Q2X2		= 29,	// =>NRF24L01, extension of CX-10 protocol
    PROTO_WK2x01	= 30,	// =>CYRF6936
    PROTO_Q303		= 31,	// =>NRF24L01
    PROTO_GW008		= 32,	// =>NRF24L01
    PROTO_DM002		= 33,	// =>NRF24L01
    PROTO_CABELL	= 34,	// =>NRF24L01
    PROTO_ESKY150	= 35,	// =>NRF24L01
    PROTO_H8_3D		= 36,	// =>NRF24L01
    PROTO_CORONA	= 37,	// =>CC2500
    PROTO_CFLIE     = 38,   // =>NRF24L01
    PROTO_HITEC     = 39,   // =>CC2500
    PROTO_WFLY		= 40,	// =>CYRF6936
    PROTO_BUGS		= 41,	// =>A7105
    PROTO_BUGSMINI	= 42,	// =>NRF24L01
    PROTO_TRAXXAS	= 43,	// =>CYRF6936
    PROTO_NCC1701	= 44,	// =>NRF24L01
    PROTO_E01X		= 45,	// =>NRF24L01
    PROTO_V911S		= 46,	// =>NRF24L01
    PROTO_GD00X		= 47,	// =>NRF24L01
    PROTO_V761		= 48,	// =>NRF24L01
    PROTO_KF606		= 49,	// =>NRF24L01
    PROTO_REDPINE	= 50,	// =>CC2500
    PROTO_POTENSIC	= 51,	// =>NRF24L01
    PROTO_ZSX		= 52,	// =>NRF24L01
    PROTO_FLYZONE	= 53,	// =>A7105
    PROTO_SCANNER	= 54,	// =>CC2500
    PROTO_FRSKY_RX	= 55,	// =>CC2500
    PROTO_AFHDS2A_RX= 56,	// =>A7105
    PROTO_HOTT		= 57,	// =>CC2500
    PROTO_FX816		= 58,	// =>NRF24L01
    PROTO_BAYANG_RX	= 59,	// =>NRF24L01
	PROTO_PELIKAN	= 60,	// =>A7105
	PROTO_TIGER		= 61,	// =>NRF24L01
	PROTO_SILVERLITE= 62,   // =>NRF24L01
	PROTO_XN297DUMP	= 63,	// =>NRF24L01
};

enum Flysky
{
	Flysky	= 0,
	V9X9	= 1,
	V6X6	= 2,
	V912	= 3,
	CX20	= 4,
};
enum Flyzone
{
	FZ410	= 0,
};
enum Hubsan
{
	H107	= 0,
	H301	= 1,
	H501	= 2,
};
enum AFHDS2A
{
	PWM_IBUS = 0,
	PPM_IBUS = 1,
	PWM_SBUS = 2,
	PPM_SBUS = 3,
};
enum Hisky
{
	Hisky	= 0,
	HK310	= 1,
};
enum DSM
{
	DSM2_22	= 0,
	DSM2_11	= 1,
	DSMX_22	= 2,
	DSMX_11	= 3,
	DSM_AUTO = 4,
};
enum YD717
{       			
	YD717	= 0,
	SKYWLKR	= 1,
	SYMAX4	= 2,
	XINXUN	= 3,
	NIHUI	= 4,
};
enum KN
{
	WLTOYS	= 0,
	FEILUN	= 1,
};
enum SYMAX
{
	SYMAX	= 0,
	SYMAX5C	= 1,
};
enum SLT
{
	SLT_V1	= 0,
	SLT_V2	= 1,
	Q100	= 2,
	Q200	= 3,
	MR100	= 4,
};
enum CX10
{
	CX10_GREEN	= 0,
	CX10_BLUE	= 1,	// also compatible with CX10-A, CX12
	DM007		= 2,
	JC3015_1	= 4,
	JC3015_2	= 5,
	MK33041		= 6,
};
enum Q2X2
{
	Q222		= 0,
	Q242		= 1,
	Q282		= 2,
	F_Q222		= 8,
	F_Q242		= 9,
	F_Q282		= 10,
};
enum CG023
{
    CG023	= 0,
    YD829	= 1,
};
enum BAYANG
{
    BAYANG	= 0,
    H8S3D	= 1,
    X16_AH  = 2,
	IRDRONE = 3,
	DHD_D4	= 4,
    LT8900  = 5
};
enum MT99XX
{
	MT99	= 0,
	H7		= 1,
	YZ		= 2,
	LS		= 3,
	FY805	= 4,
};
enum MJXQ
{
	WLH08	= 0,
	X600	= 1,
	X800	= 2,
	H26D	= 3,
	E010	= 4,
	H26WH	= 5,
	PHOENIX = 6,
};
enum FRSKYX
{
	CH_16	= 0,
	CH_8	= 1,
	EU_16	= 2,
	EU_8	= 3,
};
enum HONTAI
{
	HONTAI	= 0,
	JJRCX1	= 1,
	X5C1	= 2,
	FQ777_951 =3,
};
enum V2X2
{
	V2X2	= 0,
	JXD506	= 1,
};
enum FY326
{
	FY326	= 0,
	FY319	= 1,
};
enum WK2x01
{
	WK2801	= 0,
	WK2401	= 1,
	W6_5_1	= 2,
	W6_6_1	= 3,
	W6_HEL	= 4,
	W6_HEL_I= 5,
};
enum Q303
{
	Q303	= 0,
	CX35	= 1,
	CX10D	= 2,
	CX10WD	= 3,
};
enum CABELL
{
	CABELL_V3			= 0,
	CABELL_V3_TELEMETRY	= 1,
	CABELL_SET_FAIL_SAFE= 6,
	CABELL_UNBIND		= 7,
};
enum H8_3D
{
	H8_3D	= 0,
	H20H	= 1,
	H20MINI	= 2,
	H30MINI	= 3,
};
enum CORONA
{
	COR_V1	= 0,
	COR_V2	= 1,
	FD_V3	= 2,
};
enum HITEC
{
	OPT_FW	= 0,
	OPT_HUB	= 1,
	MINIMA	= 2,
};
enum E01X
{
	E012	= 0,
	E015	= 1,
	E016H	= 2,
};
enum GD00X
{
	GD_V1	= 0,
	GD_V2	= 1,
};
enum BUGSMINI
{
	BUGSMINI= 0,
	BUGS3H	= 1,
};
enum REDPINE
{
	RED_FAST= 0,
	RED_SLOW= 1,
};
enum TRAXXAS
{
	RX6519	= 0,
};
enum ESKY150
{
	ESKY150_4CH	= 0,
	ESKY150_7CH	= 1,
};


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


static void applyModelIndex();
static uint8_t selectedModelIndex;
static SelectOptionByte selectModelOptions[] = 
{
    {"1", 0},
    {"2", 1},
    {"3", 2},
    {"4", 3},
    {"5", 4},
    {"6", 5},
    {"7", 6},
    {"8", 7},
    {"9", 8},
    {"10", 9}
};
static GEMSelect selectModel(sizeof(selectModelOptions)/sizeof(selectModelOptions[0]), selectModelOptions);
static GEMItem miSelectModel("Select Model:", selectedModelIndex, selectModel, applyModelIndex);

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
    for (int i=0; i<sizeof(subprotocols)/sizeof(subprotocols[0]); i++)
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

static void updateModelDataParams(int modelIndex)
{
    ModelDesc_t &model = storage.model[modelIndex];
    strncpy(modelName, model.name, GEM_STR_LEN - 1);
    secondsTimer = model.timer;
    protocol = model.mpm_protocol;
    updateSubprotocolParams(protocol);
    // updateSubprotocolParams() will change subprotocol and protocolOption, so we
    // must be sure to initialize subprotocol *after* calling updateSubprotocolParams()
    subprotocol = model.mpm_sub_protocol;
    protocolOption = model.mpm_option;
    autoBind = model.mpm_auto_bind ? 1 : 0;
    rxNum = model.mpm_rx_num;
}

static void applyModelIndex()
{
    storage.current_model = selectedModelIndex;
    updateModelDataParams(selectedModelIndex);
    storage_save();
}

static void applyModelName()
{
    strncpy(storage.model[storage.current_model].name, modelName, sizeof(storage.model[0].name)-1);
    storage_save();
}

void gui_init_models()
{
    selectedModelIndex = storage.current_model;
    updateModelDataParams(selectedModelIndex);

    menuPageModels.addMenuItem(miSelectModel);
    miEditName.setTypeToSimpleString();
    menuPageModels.addMenuItem(miEditName); 
    menuPageModels.addMenuItem(miSeconds);
    menuPageModels.addMenuItem(miSelectProtocol);
    menuPageModels.addMenuItem(miSelectSubprotocol);
    menuPageModels.addMenuItem(miProtocolOption);
    menuPageModels.addMenuItem(miAutoBind);
    menuPageModels.addMenuItem(miRXNum);
    menuPageModels.setParentMenuPage(menuPageMain);
}

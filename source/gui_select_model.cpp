#include "debug.h"
#include "console.h"
#include "GEM.h"
#include "storage.h"
#include "config.h"
#include <string.h>

extern GEM gGEM;
extern GEMPage menuPageMain;
extern void RunTXCtx();
extern void gui_init_edit_model_properties();

GEMPage menuPageSelectModel("Select Model");

static void onModelSelected()
{
    // GEM doesn't provide us any parameters, expecting each button callback
    // is unique. So we'll need to peek into GEM directly
    // Note: Since we set menuPageSelectModel to have a parent page, the
    // current item num will be one more than the selected model index
    // because the back page is first in the list
    storage.current_model = menuPageSelectModel.currentItemNum - 1;
    storage_save();

    menuPageMain.title = storage.model[storage.current_model].name;
    gGEM.setMenuPageCurrent(menuPageMain);

    // Call this to initialize variables that will be used by menuItems of menuPageEditModel. 
    gui_init_edit_model_properties();

    RunTXCtx();
}

static GEMItem miModels[STORAGE_MODEL_MAX_COUNT] = 
{
    { nullptr, onModelSelected },
    { nullptr, onModelSelected },
    { nullptr, onModelSelected },
    { nullptr, onModelSelected },
    { nullptr, onModelSelected },
    { nullptr, onModelSelected },
    { nullptr, onModelSelected },
    { nullptr, onModelSelected },
    { nullptr, onModelSelected },
    { nullptr, onModelSelected }
};

void gui_init_select_model()
{
    for (int i=0; i<STORAGE_MODEL_MAX_COUNT; i++)
    {
        miModels[i].title = storage.model[i].name;
        miModels[i].buttonAction = onModelSelected;
        menuPageSelectModel.addMenuItem(miModels[i]);
    }
    menuPageSelectModel.setParentMenuPage(menuPageMain);

    // Note: Since we set menuPageSelectModel to have a parent page, the
    // current item num will be one more than the selected model index
    // because the back page is first in the list
    menuPageSelectModel.currentItemNum = storage.current_model + 1;
}

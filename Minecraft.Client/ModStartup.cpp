#include "stdafx.h"
#include "ModLoader.h"


struct ModStartup {
    ModStartup() {
        ModLoader::Get().Initialize();
    }
};

static ModStartup g_ModStartup;
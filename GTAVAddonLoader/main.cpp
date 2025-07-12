#include "script.h"

#include "GitInfo.h"
#include "NativeMemory.hpp"
#include "Util/Logger.hpp"
#include "Util/Paths.h"
#include "Util/Versions.h"

#include <inc/main.h>

void initPaths(HMODULE hInstance){
    Paths::SetOurModuleHandle(hInstance);
    std::string logFile = Paths::GetModuleFolder(hInstance) + modDir +
        "\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";
    gLogger.SetPath(logFile);
    gLogger.Clear();
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        initPaths(hInstance);
        LOG(Info, "Add-On Vehicle Spawner {} ({} @ {})", DISPLAY_VERSION, GIT_HASH GIT_DIFF, GIT_DATE);
        LOG(Info, "Game version - {}", Versions::GetName(getGameVersion()));
        scriptRegister(hInstance, ScriptMain);
        setupHooks();
        LOG(Info, "Script registered");
        break;
    case DLL_PROCESS_DETACH:
        scriptUnregister(hInstance);
        break;
    }
    return TRUE;
}

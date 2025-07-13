/*
 * Credits & Acknowledgements:
 * Generating vehicle model list: ScriptHookVDotNet source (drp4lyf/zorg93)
 * Getting vehicle modkit IDs:    Unknown Modder
 * Getting textures from dicts:   Unknown Modder
 * InitVehicleArchetype (Legacy):       Unknown Modder
 * InitVehicleArchetype (Enhanced):     avail
 * Find enable mp vehicles (Legacy):    drp4lyf/zorg93
 * Find enable mp vehicles (Enhanced):
 */

#include "NativeMemory.hpp"

#include "Hooking.h"
#include "Util/Logger.hpp"
#include "Util/Util.hpp"
#include "Util/Versions.h"

#include <Windows.h>
#include <Psapi.h>

#include <array>
#include <vector>
#include <unordered_map>

typedef CVehicleModelInfo* (*GetModelInfo_t)(unsigned int modelHash, int* index);

typedef CVehicleModelInfo* (*InitVehicleArchetype_t)(const char*, bool, unsigned int);
typedef int64_t(*InitVehicleArchetypeEnhanced_t)(uint32_t a1, const char*);

GetModelInfo_t GetModelInfo;

GlobalTable globalTable;
ScriptTable* scriptTable;
ScriptHeader* shopController;

CCallHook<InitVehicleArchetype_t>* g_InitVehicleArchetype = nullptr;
CCallHook<InitVehicleArchetypeEnhanced_t>* g_InitVehicleArchetypeEnhanced = nullptr;

extern std::unordered_map<Hash, std::string> g_vehicleHashes;

int gameVersion = getGameVersion();

CVehicleModelInfo* initVehicleArchetype_stub(const char* name, bool a2, unsigned int a3) {
    g_vehicleHashes.insert({ joaat(name), name });
    return g_InitVehicleArchetype->mFunc(name, a2, a3);
}

int64_t initVehicleArchetypeEnhanced_stub(uint32_t a1, const char* name) {
    g_vehicleHashes.insert({ joaat(name), name });
    return g_InitVehicleArchetypeEnhanced->mFunc(a1, name);
}

void setupHooks() {
    uintptr_t addr = 0;
    if (Versions::IsEnhanced()) {
        addr = MemoryAccess::FindPattern("\xE8\x00\x00\x00\x00\x43\x89\x44\x2C", "x????xxxx");
    }
    else {
        addr = MemoryAccess::FindPattern("\xE8\x00\x00\x00\x00\x48\x8B\x4D\xE0\x48\x8B\x11", "x????xxxxxxx");
    }
    if (!addr) {
        LOG(Error, "Couldn't find InitVehicleArchetype");
        return;
    }
    LOG(Info, "Found InitVehicleArchetype at {}", (void*)addr);
    if (Versions::IsEnhanced()) {
        g_InitVehicleArchetypeEnhanced = HookManager::SetCall(addr, initVehicleArchetypeEnhanced_stub);
    }
    else {
        g_InitVehicleArchetype = HookManager::SetCall(addr, initVehicleArchetype_stub);
    }
}

void removeHooks() {
    if (g_InitVehicleArchetype) {
        delete g_InitVehicleArchetype;
        g_InitVehicleArchetype = nullptr;
    }
    if (g_InitVehicleArchetypeEnhanced) {
        delete g_InitVehicleArchetypeEnhanced;
        g_InitVehicleArchetypeEnhanced = nullptr;
    }
}

void MemoryAccess::initGetModelInfo() {
    if (Versions::IsEnhanced()) {
        LOG(Info, "GetModelInfo skipped for Enhanced");
        GetModelInfo = nullptr;
        return;
    }

    uintptr_t addr;
    if (gameVersion <= Versions::EGameVersion::L_1_0_1868_1_NOSTEAM) {
        addr = MemoryAccess::FindPattern(
            "\x0F\xB7\x05\x00\x00\x00\x00"
            "\x45\x33\xC9\x4C\x8B\xDA\x66\x85\xC0"
            "\x0F\x84\x00\x00\x00\x00"
            "\x44\x0F\xB7\xC0\x33\xD2\x8B\xC1\x41\xF7\xF0\x48"
            "\x8B\x05\x00\x00\x00\x00"
            "\x4C\x8B\x14\xD0\xEB\x09\x41\x3B\x0A\x74\x54",
            "xxx????"
            "xxxxxxxxx"
            "xx????"
            "xxxxxxxxxxxx"
            "xx????"
            "xxxxxxxxxxx");

        if (!addr) {
            LOG(Error, "Couldn't find GetModelInfo");
        }
        else {
            GetModelInfo = (GetModelInfo_t)(addr);
            return;
        }
    }
    else {
        addr = FindPattern("\xEB\x09\x41\x3B\x0A\x74\x54", "xxxxxxx");
        if (!addr) {
            LOG(Error, "Couldn't find GetModelInfo (v58+)");
        }
        else {
            addr = addr - 0x2C;
            GetModelInfo = (GetModelInfo_t)(addr);
            return;
        }
    }
    GetModelInfo = nullptr;
}

void MemoryAccess::initEnableCarsGlobal() {
    if (Versions::IsEnhanced()) {
        LOG(Info, "Skipping enabling MP cars for Enhanced");
        return;
    }

    // find enable MP cars patterns
    if (findShopController())
        enableCarsGlobal();
}

void MemoryAccess::Init() {
    initGetModelInfo();
    initEnableCarsGlobal();
}

// Thank you, Unknown Modder!
template < typename ModelInfo >
std::vector<uint16_t> GetVehicleModKits_t(int modelHash) {
    if (!GetModelInfo)
        return {};

    std::vector<uint16_t> modKits;
    int index = 0xFFFF;
    auto* modelInfo = reinterpret_cast<ModelInfo*>(GetModelInfo(modelHash, &index));
    if (modelInfo && modelInfo->GetModelType() == 5) {
        uint16_t count = modelInfo->m_modKitsCount;
        for (uint16_t i = 0; i < count; i++) {
            uint16_t modKit = modelInfo->m_modKits[i];
            modKits.push_back(modKit);
        }
    }
    return modKits;
}

std::vector<uint16_t> MemoryAccess::GetVehicleModKits(int modelHash) {
    if (gameVersion < 38) {
        return GetVehicleModKits_t<CVehicleModelInfo>(modelHash);
    }
    else {
        return GetVehicleModKits_t<CVehicleModelInfo1290>(modelHash);
    }
}

std::string MemoryAccess::GetVehicleMakeName(int modelHash) {
    if (!GetModelInfo)
        return std::string();

    int index = 0xFFFF;
    void* modelInfo = GetModelInfo(modelHash, &index);
    const char* memName = nullptr;
    if (gameVersion < 38) {
        memName = ((CVehicleModelInfo*)modelInfo)->m_manufacturerName;
    }
    else {
        memName = ((CVehicleModelInfo1290*)modelInfo)->m_manufacturerName;
    }
    return memName ? memName : std::string();
}

uintptr_t MemoryAccess::FindPattern(const char* pattern, const char* mask, const char* startAddress, size_t size) {
    const char* address_end = startAddress + size;
    const auto mask_length = static_cast<size_t>(strlen(mask) - 1);

    for (size_t i = 0; startAddress < address_end; startAddress++) {
        if (*startAddress == pattern[i] || mask[i] == '?') {
            if (mask[i + 1] == '\0') {
                return reinterpret_cast<uintptr_t>(startAddress) - mask_length;
            }
            i++;
        }
        else {
            i = 0;
        }
    }
    return 0;
}

uintptr_t MemoryAccess::FindPattern(const char* pattern, const char* mask) {
    MODULEINFO modInfo = { };
    GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));

    return FindPattern(pattern, mask, reinterpret_cast<const char*>(modInfo.lpBaseOfDll), modInfo.SizeOfImage);
}

// from EnableMPCars by drp4lyf
bool MemoryAccess::findShopController() {
    // FindPatterns
    __int64 patternAddr = FindPattern("\x4C\x8D\x05\x00\x00\x00\x00\x4D\x8B\x08\x4D\x85\xC9\x74\x11", "xxx????xxxxxxxx");
    if (!patternAddr) {
        LOG(Error, "ERROR: finding address 0");
        LOG(Error, "Aborting...");
        return false;
    }
    globalTable.GlobalBasePtr = (__int64**)(patternAddr + *(int*)(patternAddr + 3) + 7);


    patternAddr = FindPattern("\x48\x03\x15\x00\x00\x00\x00\x4C\x23\xC2\x49\x8B\x08", "xxx????xxxxxx");
    if (!patternAddr) {
        LOG(Error, "ERROR: finding address 1");
        LOG(Error, "Aborting...");
        return false;
    }
    scriptTable = (ScriptTable*)(patternAddr + *(int*)(patternAddr + 3) + 7);

    DWORD startTime = GetTickCount();
    DWORD timeout = 10000; // in millis

    // FindScriptAddresses
    while (!globalTable.IsInitialised()) {
        scriptWait(100); //Wait for GlobalInitialisation before continuing
        if (GetTickCount() > startTime + timeout) {
            LOG(Error, "ERROR: couldn't init global table");
            LOG(Error, "Aborting...");
            return false;
        }
    }

    //LOG(Info, "Found global base pointer " + std::to_string((__int64)globalTable.GlobalBasePtr));

    ScriptTableItem* Item = scriptTable->FindScript(0x39DA738B);
    if (Item == NULL) {
        LOG(Error, "ERROR: finding address 2");
        LOG(Error, "Aborting...");
        return false;
    }
    while (!Item->IsLoaded())
        Sleep(100);

    shopController = Item->Header;
    //LOG(Info, "Found shopcontroller");
    return true;
}

void MemoryAccess::enableCarsGlobal() {
    const char* patt617_1 = "\x2C\x01\x00\x00\x20\x56\x04\x00\x6E\x2E\x00\x01\x5F\x00\x00\x00\x00\x04\x00\x6E\x2E\x00\x01";
    const char* mask617_1 = "xx??xxxxxx?xx????xxxx?x";
    const unsigned int offset617_1 = 13;

    const char* patt1604_0 = "\x2D\x00\x00\x00\x00\x2C\x01\x00\x00\x56\x04\x00\x6E\x2E\x00\x01\x5F\x00\x00\x00\x00\x04\x00\x6E\x2E\x00\x01";
    const char* mask1604_0 = "x??xxxx??xxxxx?xx????xxxx?x";
    const unsigned int offset1064_0 = 17;

    // Updated pattern entirely thanks to @alexguirre
    const char* patt2802_0 = "\x2D\x00\x00\x00\x00\x2C\x01\x00\x00\x56\x04\x00\x71\x2E\x00\x01\x62\x00\x00\x00\x00\x04\x00\x71\x2E\x00\x01";
    const char* mask2802_0 = "x??xxxx??xxxxx?xx????xxxx?x";
    const unsigned int offset2802_0 = 17;

    const char* pattern = patt617_1;
    const char* mask = mask617_1;
    int offset = offset617_1;

    if (getGameVersion() >= 80) {
        pattern = patt2802_0;
        mask = mask2802_0;
        offset = offset2802_0;
    }
    else if (getGameVersion() >= 46) {
        pattern = patt1604_0;
        mask = mask1604_0;
        offset = offset1064_0;
    }

    for (int i = 0; i < shopController->CodePageCount(); i++) {
        int size = shopController->GetCodePageSize(i);
        if (size) {
            uintptr_t address = FindPattern(pattern, mask, (const char*)shopController->GetCodePageAddress(i), size);
            if (address) {
                int globalindex = *(int*)(address + offset) & 0xFFFFFF;
                LOG(Info, "Setting Global Variable {} to true", globalindex);
                *globalTable.AddressOf(globalindex) = 1;
                LOG(Info, "MP Cars enabled");
                return;
            }
        }
    }

    LOG(Error, "Global Variable not found, check game version >= 1.0.678.1");
}

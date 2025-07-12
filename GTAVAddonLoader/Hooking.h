#pragma once

#include <windows.h>
#include <inttypes.h>

template <typename T>
class IHook {
public:
    virtual ~IHook() = default;
    virtual void Remove() = 0;
};

template <typename T>
class CCallHook : public IHook<T> {
public:
    CCallHook(uintptr_t addr, T func)
        : mAddress(addr)
        , mFunc(func) {
    }

    ~CCallHook() {
        Remove();
    }

    void Remove() override {
        *reinterpret_cast<int32_t*>(mAddress + 1) = static_cast<int32_t>((intptr_t)mFunc - (intptr_t)mAddress - 5);
    }

    uintptr_t mAddress;
    T mFunc;
};

class HookManager
{
public:
    template <typename T>
    static inline CCallHook<T> *SetCall(uintptr_t address, T target)
    {
        T orig = reinterpret_cast<T>(*reinterpret_cast<int *>(address + 1) + (address + 5));

        HMODULE hModule = GetModuleHandle(NULL);

        auto pFunc = AllocateFunctionStub((void*)(hModule), (void*)target, 0);

        *reinterpret_cast<BYTE*>(address) = 0xE8;

        *reinterpret_cast<int32_t*>(address + 1) = static_cast<int32_t>((intptr_t)pFunc - (intptr_t)address - 5);

        return new CCallHook<T>(address, orig);
    }

    static void *AllocateFunctionStub(void *origin, void *function, int type);

    static LPVOID FindPrevFreeRegion(LPVOID pAddress, LPVOID pMinAddr, DWORD dwAllocationGranularity);
};

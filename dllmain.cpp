#include <Windows.h>
#include <string>
#include <vector>
#include <Psapi.h>

#define REGISTER_SIZE sizeof(UINT_PTR)

#ifdef _WIN64
#define FUNCSIG "?SomeFunction4@@YAHHHPEBDHH@Z"
#define BYTEOFFSET 3
#else
#define FUNCSIG "?SomeFunction4@@YAHHHPBDHH@Z"
#define BYTEOFFSET 2
#endif

enum class CALLINGCONVENTION : DWORD
{
    CC_CDECL,
    CC_STDCALL,
    CC_FASTCALL
};

// Don't call directly.
extern "C" uintptr_t __cdecl EXECUTE(uintptr_t FunctionAddress, uintptr_t ManipAddress, uintptr_t VariablesAddress, size_t VariableAmount, CALLINGCONVENTION CallConvention);

PVOID SigScan(PCHAR StartAddress, SIZE_T Size, PCHAR Pattern, SIZE_T PatternLen)
{
    bool Found = TRUE;
    for (int i1 = 0; i1 < Size; i1++)
    {
        Found = TRUE;
        for (int i2 = 0; i2 < PatternLen; i2++)
        {
            if (StartAddress[i1 + i2] != Pattern[i2])
            {
                Found = FALSE;
                break;
            }
        }

        if (Found)
            return StartAddress + i1;
    }

    return nullptr;
}

uintptr_t SpoofReturn(HMODULE Module, void* FunctionAddress, std::vector<uintptr_t> Arguments = {}, CALLINGCONVENTION CallConvention = CALLINGCONVENTION::CC_FASTCALL)
{
#ifdef _WIN64
    // If the number of arguments is less than 4, we complete it to 4.
    while (Arguments.size() < 4)
        Arguments.push_back(0);
#else
    if (CallConvention == CALLINGCONVENTION::CC_FASTCALL)
    {
        // If the number of arguments is less than 2, we complete it to 2.
        while (Arguments.size() < 2)
            Arguments.push_back(0);
    }
#endif

    // Getting the module information so we can SigScan on it.
    MODULEINFO ModInfo = {};
    K32GetModuleInformation(GetCurrentProcess(), Module, &ModInfo, sizeof(MODULEINFO));

    // add rsp, 0x20 (64-bit) OR 0x00 (32-bit, no shadow space)
    // ret
#ifdef _WIN64
    CHAR SearchBytes[] = "\x48\x83\xC4\x20\xC3";
#else
    CHAR SearchBytes[] = "\x83\xC4\x00\xC3";
#endif

    void* GadgetAddress = nullptr;
    size_t ArgumentAmount = 0;

    size_t Try = 0;
    while (TRUE)
    {
        // Adding on top of the ( 0x20 (SHADOW SPACE SIZE ON X64) OR 0x00 ) the amount of extra arguments to pop basically.
        ArgumentAmount = Arguments.size();
#ifdef _WIN64
        SearchBytes[BYTEOFFSET] += ArgumentAmount > 4 ? (ArgumentAmount - 4) * REGISTER_SIZE : 0;
#else
        if (CallConvention == CALLINGCONVENTION::CC_FASTCALL)
            SearchBytes[BYTEOFFSET] += ArgumentAmount > 2 ? (ArgumentAmount - 2) * REGISTER_SIZE : 0;
        else
            SearchBytes[BYTEOFFSET] += ArgumentAmount * REGISTER_SIZE;
#endif

        GadgetAddress = SigScan((PCHAR)ModInfo.lpBaseOfDll, ModInfo.SizeOfImage, SearchBytes, sizeof(SearchBytes) - 1);

        // If there is no gadget for that particular amount of variables to pop we add junk arguments to the stack which will be pushed
        // first and not be used by the function anyways (this won't interrupt the function frame)
        if (!GadgetAddress)
        {
            if (Try > 20)
            {
                printf("[!] Couldn't find any gadget.\n");
                return -1;
            }

            Arguments.push_back(0);
            Try++;
            continue;
        }
        break;
    }

    return EXECUTE((uintptr_t)FunctionAddress, (uintptr_t)GadgetAddress, (uintptr_t)&Arguments[0], ArgumentAmount, CallConvention);
}

void MainThread(HMODULE hModule)
{
    printf("= RETSPOOFER INJECTED =\n");
    printf("Press [F1] to make a spoof call\n\n");

    while (!GetAsyncKeyState(VK_F1))
        ;

    const HMODULE DllBase = GetModuleHandle(NULL);

    // Default call
    auto SomeFunc = (int(__cdecl*)(int, int, const char*, int, int))GetProcAddress(DllBase, FUNCSIG);

    printf("DEFAULT CALL\n");
    uintptr_t RetVal = SomeFunc(1, 9, "hey", 3, 3);
    printf("RetVal: %i\n\n", RetVal);

    // Spoofed call
    printf("SPOOFED CALL\n");
    RetVal = SpoofReturn(DllBase, SomeFunc, { 1, 9, (uintptr_t)"hey", 3, 3}, CALLINGCONVENTION::CC_CDECL);
    printf("RetVal: %i\n\n", RetVal);

    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        HANDLE hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MainThread, hModule, NULL, NULL);
        if (hThread)
            CloseHandle(hThread);
        break;
    }
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
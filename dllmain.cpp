#include <Windows.h>
#include <string>
#include <vector>
#include <Psapi.h>

#define REGISTER_SIZE sizeof(UINT_PTR)

#ifdef _WIN64
#define FUNCSIG "?SomeFunction4@@YAHHHPEBDHH@Z"
#else
#define FUNCSIG "?SomeFunction4@@YAHHHPBDHH@Z"
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

    // jmp ebx
    CHAR SearchBytes[] = "\xFF\xE3";

    void* GadgetAddress = nullptr;
    size_t ArgumentAmount = Arguments.size();

    size_t Try = 0;
    while (TRUE)
    {
        PIMAGE_DOS_HEADER pImageDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ModInfo.lpBaseOfDll);
        PIMAGE_NT_HEADERS pNtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>((char*)pImageDosHeader + pImageDosHeader->e_lfanew);
        PIMAGE_SECTION_HEADER pFirstSec = IMAGE_FIRST_SECTION(pNtHeaders);
        for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++)
        {
            if ((pFirstSec->Characteristics & IMAGE_SCN_CNT_CODE) || (_stricmp((const char*)pFirstSec->Name, ".text") == 0))
            {
                GadgetAddress = SigScan((PCHAR)pImageDosHeader + pFirstSec->VirtualAddress, pFirstSec->Misc.VirtualSize, SearchBytes, sizeof(SearchBytes) - 1);
            
                if (GadgetAddress)
                    break;
            }
            pFirstSec++;
        }

        if (!GadgetAddress)
        {
            printf("[!] Couldn't find any gadget.\n");
            return -1;
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
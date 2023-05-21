<p align="center">
<img src="https://github.com/paskalian/RetSpoofer/blob/master/Images/RetSpoofer.svg" alt="Menu"/>
</p>

## Information
**Made for educational purposes only.**<br>

**The 32-bit version isn't tested yet.**

## Compatibility
Compatible with both x64 and (--x86--) processes, you **must** use the **specific** version for a process that is x64 or x86 respectively.

## Example Process Source
```cpp
#include <intrin.h>
#include <iostream>
#include <Windows.h>
#include <Psapi.h>

void IsAuthorizedMemory(uintptr_t Memory)
{
    static HMODULE SelfBase = GetModuleHandleA(NULL);
    static MODULEINFO ModInfo = {};
    static BOOL CallOnce = K32GetModuleInformation(GetCurrentProcess(), SelfBase, &ModInfo, sizeof(MODULEINFO));

    if ((Memory >= (uintptr_t)SelfBase && Memory <= (uintptr_t)SelfBase + ModInfo.SizeOfImage))
    {
        printf("[*] Called from an authorized memory.\n\n");
    }
    else
    {
        // ... WOULD CRASH THE PROCESS AND/OR FLAG YOU ETC. NORMALLY ... //

        printf("[!] Called from an unauthorized memory.\n\n");
    }
}

__declspec(dllexport) void SomeFunction()
{
    uintptr_t* pRet = (uintptr_t*)_AddressOfReturnAddress();

    uintptr_t ReturnAddress = *pRet;
    printf("[*] Return Address: %p\n", ReturnAddress);

    IsAuthorizedMemory(ReturnAddress);

    // ... FUNCTION CODE ... //
}

__declspec(dllexport) void SomeFunction2(int a, int b)
{
    uintptr_t* pRet = (uintptr_t*)_AddressOfReturnAddress();

    uintptr_t ReturnAddress = *pRet;
    printf("[*] Return Address: %p\n", ReturnAddress);

    IsAuthorizedMemory(ReturnAddress);

    // ... FUNCTION CODE ... //
}

__declspec(dllexport) void SomeFunction3(int a, int b, const char* c, int d, int e)
{
    uintptr_t* pRet = (uintptr_t*)_AddressOfReturnAddress();

    uintptr_t ReturnAddress = *pRet;
    printf("[*] Return Address: %p\n", ReturnAddress);

    IsAuthorizedMemory(ReturnAddress);

    // ... FUNCTION CODE ... //
}

__declspec(dllexport) int SomeFunction4(int a, int b, const char* c, int d, int e)
{
    uintptr_t* pRet = (uintptr_t*)_AddressOfReturnAddress();

    int lel = a + b + d + e;

    uintptr_t ReturnAddress = *pRet;
    printf("[*] Return Address: %p\n", ReturnAddress);

    IsAuthorizedMemory(ReturnAddress);

    // ... FUNCTION CODE ... //

    return lel;
}

int main()
{
    SomeFunction();

    while (TRUE)
        ;
}
```
This is the source code of the example process which exports some functions (not a must, made it like this so I can easily get the address of it on my dll) and checks if any of those functions were called from it's own module.

## Usage
```cpp
SpoofReturn(DllBase, SomeFunc, { 1, 9, (uintptr_t)"hey", 3, 3}, CALLINGCONVENTION::CC_CDECL);
```
1. SpoofReturn is the handler function that does the prior set-up before actually performing a return address spoof.
   - The first param is the target module base address.
   - The second param is the function to be called inside that module.
   - The third parameter is a std::vector\<uintptr_t\> which can hold up any value given to it (since the "hey" string is actually a "const char*" I can just cast it to "uintptr_t" and pass it). **For now it doesn't support floating point values, but if you are planning to use it on WinAPI functions, that won't be a huge problem.**
   - The last parameter is the calling convention to be used on this function. **Ignored in x64 architecture.**

2. After setting up the SpoofReturn(s) compile the project and inject into the target process.

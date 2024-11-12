#include <Windows.h>
#include <gl/GL.h>
#include <dbghelp.h>

typedef HGLRC(WINAPI* wglCreateContext_t)(HDC hdc);
wglCreateContext_t original_wglCreateContext = nullptr;

HGLRC WINAPI Hooked_wglCreateContext(HDC hdc) {
    // uhh i guess this can work?
    return NULL;
}

void HookIAT() {
    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) return;

    ULONG size;
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(
        hModule, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size);

    if (!importDesc) return;

    // find opengl32.dll
    for (; importDesc->Name; importDesc++) {
        const char* modName = (const char*)((BYTE*)hModule + importDesc->Name);
        if (_stricmp(modName, "opengl32.dll") != 0) continue;

        // get the adress?
        PIMAGE_THUNK_DATA thunkILT = (PIMAGE_THUNK_DATA)((BYTE*)hModule + importDesc->OriginalFirstThunk);
        PIMAGE_THUNK_DATA thunkIAT = (PIMAGE_THUNK_DATA)((BYTE*)hModule + importDesc->FirstThunk);

        // find wglCreateContext
        for (; thunkILT->u1.Function; thunkILT++, thunkIAT++) {
            PROC* funcAddress = (PROC*)&thunkIAT->u1.Function;
            if (*funcAddress == (PROC)GetProcAddress(GetModuleHandleA("opengl32.dll"), "wglCreateContext")) {
                // Found wglCreateContext; replace it
                DWORD oldProtect;
                VirtualProtect(funcAddress, sizeof(PROC), PAGE_EXECUTE_READWRITE, &oldProtect);
                *funcAddress = (PROC)Hooked_wglCreateContext;
                VirtualProtect(funcAddress, sizeof(PROC), oldProtect, &oldProtect);
                break;
            }
        }
        break;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        HookIAT();
    }
    return TRUE;
}
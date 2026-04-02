#include "runtime.h"

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        shh::OnProcessAttach(module);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        shh::OnProcessDetach();
    }

    return TRUE;
}

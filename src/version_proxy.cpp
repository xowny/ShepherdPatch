#include <windows.h>

namespace
{
using GetFileVersionInfoAFn = BOOL(WINAPI*)(LPCSTR, DWORD, DWORD, LPVOID);
using GetFileVersionInfoExAFn = BOOL(WINAPI*)(DWORD, LPCSTR, DWORD, DWORD, LPVOID);
using GetFileVersionInfoExWFn = BOOL(WINAPI*)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);
using GetFileVersionInfoSizeAFn = DWORD(WINAPI*)(LPCSTR, LPDWORD);
using GetFileVersionInfoSizeExAFn = DWORD(WINAPI*)(DWORD, LPCSTR, LPDWORD);
using GetFileVersionInfoSizeExWFn = DWORD(WINAPI*)(DWORD, LPCWSTR, LPDWORD);
using GetFileVersionInfoSizeWFn = DWORD(WINAPI*)(LPCWSTR, LPDWORD);
using GetFileVersionInfoWFn = BOOL(WINAPI*)(LPCWSTR, DWORD, DWORD, LPVOID);
using VerQueryValueAFn = BOOL(WINAPI*)(LPCVOID, LPCSTR, LPVOID*, PUINT);
using VerQueryValueWFn = BOOL(WINAPI*)(LPCVOID, LPCWSTR, LPVOID*, PUINT);

INIT_ONCE g_versionInitOnce = INIT_ONCE_STATIC_INIT;
INIT_ONCE g_modInitOnce = INIT_ONCE_STATIC_INIT;
HMODULE g_realVersion = nullptr;
GetFileVersionInfoAFn g_getFileVersionInfoA = nullptr;
GetFileVersionInfoExAFn g_getFileVersionInfoExA = nullptr;
GetFileVersionInfoExWFn g_getFileVersionInfoExW = nullptr;
GetFileVersionInfoSizeAFn g_getFileVersionInfoSizeA = nullptr;
GetFileVersionInfoSizeExAFn g_getFileVersionInfoSizeExA = nullptr;
GetFileVersionInfoSizeExWFn g_getFileVersionInfoSizeExW = nullptr;
GetFileVersionInfoSizeWFn g_getFileVersionInfoSizeW = nullptr;
GetFileVersionInfoWFn g_getFileVersionInfoW = nullptr;
VerQueryValueAFn g_verQueryValueA = nullptr;
VerQueryValueWFn g_verQueryValueW = nullptr;

BOOL CALLBACK InitializeVersionProxy(PINIT_ONCE, PVOID, PVOID*)
{
    wchar_t systemDirectory[MAX_PATH] = {};
    const UINT length = GetSystemDirectoryW(systemDirectory, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
    {
        return FALSE;
    }

    wchar_t versionPath[MAX_PATH] = {};
    wsprintfW(versionPath, L"%s\\version.dll", systemDirectory);

    g_realVersion = LoadLibraryW(versionPath);
    if (g_realVersion == nullptr)
    {
        return FALSE;
    }

    g_getFileVersionInfoA = reinterpret_cast<GetFileVersionInfoAFn>(
        GetProcAddress(g_realVersion, "GetFileVersionInfoA"));
    g_getFileVersionInfoExA = reinterpret_cast<GetFileVersionInfoExAFn>(
        GetProcAddress(g_realVersion, "GetFileVersionInfoExA"));
    g_getFileVersionInfoExW = reinterpret_cast<GetFileVersionInfoExWFn>(
        GetProcAddress(g_realVersion, "GetFileVersionInfoExW"));
    g_getFileVersionInfoSizeA = reinterpret_cast<GetFileVersionInfoSizeAFn>(
        GetProcAddress(g_realVersion, "GetFileVersionInfoSizeA"));
    g_getFileVersionInfoSizeExA = reinterpret_cast<GetFileVersionInfoSizeExAFn>(
        GetProcAddress(g_realVersion, "GetFileVersionInfoSizeExA"));
    g_getFileVersionInfoSizeExW = reinterpret_cast<GetFileVersionInfoSizeExWFn>(
        GetProcAddress(g_realVersion, "GetFileVersionInfoSizeExW"));
    g_getFileVersionInfoSizeW = reinterpret_cast<GetFileVersionInfoSizeWFn>(
        GetProcAddress(g_realVersion, "GetFileVersionInfoSizeW"));
    g_getFileVersionInfoW = reinterpret_cast<GetFileVersionInfoWFn>(
        GetProcAddress(g_realVersion, "GetFileVersionInfoW"));
    g_verQueryValueA = reinterpret_cast<VerQueryValueAFn>(
        GetProcAddress(g_realVersion, "VerQueryValueA"));
    g_verQueryValueW = reinterpret_cast<VerQueryValueWFn>(
        GetProcAddress(g_realVersion, "VerQueryValueW"));

    return g_getFileVersionInfoA != nullptr && g_getFileVersionInfoExA != nullptr &&
           g_getFileVersionInfoExW != nullptr && g_getFileVersionInfoSizeA != nullptr &&
           g_getFileVersionInfoSizeExA != nullptr &&
           g_getFileVersionInfoSizeExW != nullptr && g_getFileVersionInfoSizeW != nullptr &&
           g_getFileVersionInfoW != nullptr && g_verQueryValueA != nullptr &&
           g_verQueryValueW != nullptr;
}

bool EnsureVersionProxyInitialized()
{
    return InitOnceExecuteOnce(&g_versionInitOnce, &InitializeVersionProxy, nullptr, nullptr) !=
           FALSE;
}

BOOL CALLBACK InitializeModernizerModule(PINIT_ONCE, PVOID, PVOID*)
{
    HMODULE self = nullptr;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&InitializeModernizerModule), &self) == FALSE)
    {
        return FALSE;
    }

    wchar_t modulePath[MAX_PATH] = {};
    if (GetModuleFileNameW(self, modulePath, MAX_PATH) == 0)
    {
        return FALSE;
    }

    wchar_t* lastSlash = wcsrchr(modulePath, L'\\');
    if (lastSlash == nullptr)
    {
        return FALSE;
    }

    *(lastSlash + 1) = L'\0';
    wcscat_s(modulePath, L"ShepherdPatch.asi");
    return LoadLibraryW(modulePath) != nullptr;
}

void EnsureModernizerInitialized()
{
    InitOnceExecuteOnce(&g_modInitOnce, &InitializeModernizerModule, nullptr, nullptr);
}

DWORD WINAPI BootstrapModernizerThread(void*)
{
    EnsureModernizerInitialized();
    return 0;
}
} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, &BootstrapModernizerThread, nullptr, 0, nullptr);
        if (thread != nullptr)
        {
            CloseHandle(thread);
        }
    }

    return TRUE;
}

extern "C" BOOL WINAPI GetFileVersionInfoA(LPCSTR filename, DWORD handle, DWORD length,
                                           LPVOID data)
{
    EnsureModernizerInitialized();
    if (!EnsureVersionProxyInitialized())
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return FALSE;
    }

    return g_getFileVersionInfoA(filename, handle, length, data);
}

extern "C" BOOL WINAPI GetFileVersionInfoExA(DWORD flags, LPCSTR filename, DWORD handle,
                                             DWORD length, LPVOID data)
{
    EnsureModernizerInitialized();
    if (!EnsureVersionProxyInitialized())
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return FALSE;
    }

    return g_getFileVersionInfoExA(flags, filename, handle, length, data);
}

extern "C" BOOL WINAPI GetFileVersionInfoExW(DWORD flags, LPCWSTR filename, DWORD handle,
                                             DWORD length, LPVOID data)
{
    EnsureModernizerInitialized();
    if (!EnsureVersionProxyInitialized())
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return FALSE;
    }

    return g_getFileVersionInfoExW(flags, filename, handle, length, data);
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR filename, LPDWORD handle)
{
    EnsureModernizerInitialized();
    if (!EnsureVersionProxyInitialized())
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return 0;
    }

    return g_getFileVersionInfoSizeA(filename, handle);
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeExA(DWORD flags, LPCSTR filename, LPDWORD handle)
{
    EnsureModernizerInitialized();
    if (!EnsureVersionProxyInitialized())
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return 0;
    }

    return g_getFileVersionInfoSizeExA(flags, filename, handle);
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeExW(DWORD flags, LPCWSTR filename,
                                                  LPDWORD handle)
{
    EnsureModernizerInitialized();
    if (!EnsureVersionProxyInitialized())
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return 0;
    }

    return g_getFileVersionInfoSizeExW(flags, filename, handle);
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeW(LPCWSTR filename, LPDWORD handle)
{
    EnsureModernizerInitialized();
    if (!EnsureVersionProxyInitialized())
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return 0;
    }

    return g_getFileVersionInfoSizeW(filename, handle);
}

extern "C" BOOL WINAPI GetFileVersionInfoW(LPCWSTR filename, DWORD handle, DWORD length,
                                           LPVOID data)
{
    EnsureModernizerInitialized();
    if (!EnsureVersionProxyInitialized())
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return FALSE;
    }

    return g_getFileVersionInfoW(filename, handle, length, data);
}

extern "C" BOOL WINAPI VerQueryValueA(LPCVOID block, LPCSTR subBlock, LPVOID* buffer,
                                      PUINT length)
{
    EnsureModernizerInitialized();
    if (!EnsureVersionProxyInitialized())
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return FALSE;
    }

    return g_verQueryValueA(block, subBlock, buffer, length);
}

extern "C" BOOL WINAPI VerQueryValueW(LPCVOID block, LPCWSTR subBlock, LPVOID* buffer,
                                      PUINT length)
{
    EnsureModernizerInitialized();
    if (!EnsureVersionProxyInitialized())
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return FALSE;
    }

    return g_verQueryValueW(block, subBlock, buffer, length);
}

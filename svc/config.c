#include "config.h"

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <wchar.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Shell32.lib")

static int ClampPct(int v)
{
    if (v < 0)   return 0;
    if (v > 100) return 100;
    return v;
}

// Resolves to %ProgramData%\SwitchProSvc\switchprosvc.ini.
static void ResolveIniPath(wchar_t* path, size_t chars)
{
    path[0] = L'\0';
    wchar_t base[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, base))) {
        return;
    }
    if (FAILED(StringCchPrintfW(path, chars, L"%s\\SwitchProSvc\\switchprosvc.ini", base))) {
        path[0] = L'\0';
    }
}

void SvcConfigLoad(SVC_CONFIG* out, wchar_t* outIniPath, size_t outIniPathChars)
{
    out->LeftStickDeadzonePct  = 0;
    out->RightStickDeadzonePct = 0;
    out->SwapFaceButtons = FALSE;

    wchar_t path[MAX_PATH];
    ResolveIniPath(path, MAX_PATH);

    if (outIniPath && outIniPathChars > 0) {
        wcsncpy_s(outIniPath, outIniPathChars, path, _TRUNCATE);
    }

    if (path[0] == L'\0' || !PathFileExistsW(path)) {
        return; // file missing -> defaults
    }

    out->LeftStickDeadzonePct  = ClampPct(GetPrivateProfileIntW(L"Sticks",  L"LeftStickDeadzonePct",  0, path));
    out->RightStickDeadzonePct = ClampPct(GetPrivateProfileIntW(L"Sticks",  L"RightStickDeadzonePct", 0, path));
    out->SwapFaceButtons       = GetPrivateProfileIntW(L"Buttons", L"SwapFaceButtons", 0, path) ? TRUE : FALSE;
}

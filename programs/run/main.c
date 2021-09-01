#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <commctrl.h>

// #include "taskmgr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(run);

typedef void(WINAPI *RUNFILEDLG)(
    HWND hwndOwner,
    HICON hIcon,
    LPCSTR lpstrDirectory,
    LPCSTR lpstrTitle,
    LPCSTR lpstrDescription,
    UINT uFlags);

#define RFF_NOBROWSE 0x01      /* Removes the browse button. */
#define RFF_NODEFAULT 0x02     /* No default item selected. */
#define RFF_CALCDIRECTORY 0x04 /* Calculates the working directory from the file name. */
#define RFF_NOLABEL 0x08       /* Removes the edit box label. */
#define RFF_NOSEPARATEMEM 0x20 /* Removes the Separate Memory Space check box (Windows NT only). */

int __cdecl wmain()
{
    HANDLE hwnd = GetCurrentProcess();
    RUNFILEDLG RunFileDlg;
    OSVERSIONINFOW versionInfo;
    RunFileDlg = (void *)GetProcAddress(GetModuleHandleW(L"shell32.dll"), (LPCSTR)61);
    if (RunFileDlg)
    {
        // HICON hIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_TASKMANAGER));
        versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
        GetVersionExW(&versionInfo);

        if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            WCHAR wTitle[64];
            LoadStringW(GetModuleHandleW(NULL), "IDS_RUNDLG_CAPTION", wTitle, 64);
            RunFileDlg(hwnd, NULL, NULL, (LPCSTR)wTitle, NULL, RFF_CALCDIRECTORY);
        }
        else
        {
            char szTitle[64];
            LoadStringA(GetModuleHandleW(NULL), "IDS_RUNDLG_CAPTION", szTitle, 64);
            RunFileDlg(hwnd, NULL, NULL, szTitle, NULL, RFF_CALCDIRECTORY);
        }
    }
    else{
        WINE_ERR("failed opening run dialog\n");
    }

    return 0;
}

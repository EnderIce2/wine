#ifndef __WINE_NTUSER_H
#define __WINE_NTUSER_H

#if !defined(_WIN32U_)
#define WIN32UAPI DECLSPEC_HIDDEN
#else
#define WIN32UAPI
#endif


WIN32UAPI DWORD            NTAPI NtUserGetGuiResources(HANDLE hProcess, DWORD uiFlags);

#endif /* __WINE_NTUSER_H */

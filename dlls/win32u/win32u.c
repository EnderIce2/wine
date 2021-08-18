#include <stdarg.h>

#include "winternl.h"
#include "windef.h"
#include "ntuser.h"
#include "ddk/wdm.h"
#include "win32u_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win32u);

POBJECT_TYPE *PsProcessType = NULL;

NTSTATUS stub_ObReferenceObjectByHandle(HANDLE handle, ACCESS_MASK access,
                                        POBJECT_TYPE type,
                                        KPROCESSOR_MODE mode, void **ptr,
                                        POBJECT_HANDLE_INFORMATION info)
{
    return STATUS_SUCCESS;
}

/**********************************************************************
 *	NtUserGetGuiResources	(WIN32U.@)
 */
DWORD NTAPI NtUserGetGuiResources(HANDLE hProcess, DWORD uiFlags)
{
    static int once;
    PRIVATE_EPROCESS process;
    PPROCESSINFO W32Process;
    DWORD error;
    DWORD ret = 0;

    if (!once++)
        FIXME("(%p,%x): stub\n", hProcess, uiFlags);
    // need fix, taskmgr is crashing if i use ObReferenceObjectByHandle
    error = stub_ObReferenceObjectByHandle(hProcess, PROCESS_QUERY_INFORMATION, *PsProcessType, ExGetPreviousMode(), (PVOID *)&process, NULL); // ObReferenceObjectByHandle
    if (!NT_SUCCESS(error))
    {
        ERR("ObReferenceObjectByHandle failed (%d)\n", error);
        SetLastError(error);
        return 0;
    }
    W32Process = (PPROCESSINFO)process.W32Process;
    if (!W32Process)
    {
        ERR("Invalid parameter\n");
        ObDereferenceObject(&process);
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    switch (uiFlags)
    {
    case GR_GDIOBJECTS:
    {
        ret = (DWORD)W32Process->GDIHandleCount;
        break;
    }
    case GR_USEROBJECTS:
    {
        ret = (DWORD)W32Process->UserHandleCount;
        break;
    }
    default:
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
    }
    }
    ObDereferenceObject(&process);
    return ret;
}

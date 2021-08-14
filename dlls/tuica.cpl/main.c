#define NONAMELESSUNION
#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <commctrl.h>
#include <cpl.h>
#include "ole2.h"

#include "wine/debug.h"
#include "tuica.h"

WINE_DEFAULT_DEBUG_CHANNEL(tuica_cpl);
DECLSPEC_HIDDEN HMODULE hcpl;

BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %d, %p)\n", hdll, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hdll);
            hcpl = hdll;
    }
    return TRUE;
}

/******************************************************************************
 * propsheet_callback [internal]
 */
static int CALLBACK propsheet_callback(HWND hwnd, UINT msg, LPARAM lparam)
{
    TRACE("(%p, 0x%08x/%d, 0x%lx)\n", hwnd, msg, msg, lparam);
    switch (msg)
    {
        case PSCB_INITIALIZED:
            break;
    }
    return 0;
}

#define OFN_PATHMUSTEXIST            0x00000800
#define OFN_FILEMUSTEXIST            0x00001000
#define PSN_APPLY               (PSN_FIRST-2)

static inline char *get_text(HWND dialog, WORD id)
{
    HWND item = GetDlgItem(dialog, id);
    int len = GetWindowTextLengthA(item) + 1;
    char *result = len ? HeapAlloc(GetProcessHeap(), 0, len) : NULL;
    if (!result) return NULL;
    if (GetWindowTextA(item, result, len) == 0) {
        HeapFree (GetProcessHeap(), 0, result);
        return NULL;
    }
    return result;
}

#include <commdlg.h>

static INT_PTR CALLBACK init_elements(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    TRACE("(%p, 0x%08x/%d, 0x%lx)\n", hwnd, msg, msg, lparam);
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            EnableWindow(GetDlgItem(hwnd, IDC_BUTTONWALLPAPER), TRUE);
            if (GetTileWallpaper() == TRUE)
            {
                EnableWindow(GetDlgItem(hwnd, IDC_BUTTONENABLETILE), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_BUTTONDISABLETILE), TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hwnd, IDC_BUTTONENABLETILE), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_BUTTONDISABLETILE), FALSE);
            }
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wparam))
            {
                case IDC_BUTTONWALLPAPER:
                {
                    char *location;
                    location = get_text(hwnd, IDC_TEXTBOXLOCATION);
                    FIXME("checking directory not implemented\n");
                    //ERR("path seems missing\n");
                    //MessageBoxW(hwnd, (LPCWSTR)L"Invalid path!", (LPCWSTR)L"Error", MB_OK | MB_ICONEXCLAMATION);
                    //return FALSE;
                    
                    /*                                                      "C:\\test.bmp"                                  */
                    BOOL out = SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, location, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                    SetDlgItemTextA(hwnd, IDC_CURRENTLOCATION, location);
                    if (out == FALSE)
                    {
                        ERR("SystemParametersInfoA returned %u\n", out);
                        MessageBoxW(hwnd, (LPCWSTR)L"SystemParametersInfoA failed\nCheck wine output", (LPCWSTR)L"Error", MB_OK | MB_ICONEXCLAMATION);
                        return FALSE;
                    }
                    break;
                }
                case IDC_BUTTONENABLETILE:
                {
                    SetTileWallpaper(TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONENABLETILE), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONDISABLETILE), TRUE);
                    break;
                }
                case IDC_BUTTONDISABLETILE:
                {
                    SetTileWallpaper(FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONENABLETILE), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDC_BUTTONDISABLETILE), FALSE);
                    break;
                }

                break;
            }
            return TRUE;
        }
        case PSN_APPLY:
        {
            FIXME("PSN_APPLY not implemented yet\n");
            return TRUE;
            break;
        }
        case EN_CHANGE:
        {
            /* enable apply button */
            SendMessageW(GetParent(hwnd), PSM_CHANGED, 0, 0);
            break;
        }
        case WM_NOTIFY:
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

/******************************************************************************
 * display_cpl_sheets [internal]
 *
 * Build and display the dialog with all control panel propertysheets
 *
 */
static void display_cpl_sheets(HWND parent)
{
    INITCOMMONCONTROLSEX icex;
    PROPSHEETPAGEW psp[NUM_PROPERTY_PAGES];
    PROPSHEETHEADERW psh;
    DWORD id = 0;

    OleInitialize(NULL);
    /* Initialize common controls */
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    ZeroMemory(&psh, sizeof(psh));
    ZeroMemory(psp, sizeof(psp));

    psp[id].dwSize = sizeof (PROPSHEETPAGEW);
    psp[id].hInstance = hcpl;
    psp[id].u.pszTemplate = MAKEINTRESOURCEW(IDD_MAIN);
    psp[id].pfnDlgProc = init_elements;
    id++;

    /* Fill out the PROPSHEETHEADER */
    psh.dwSize = sizeof (PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = parent;
    psh.hInstance = hcpl;
    psh.pszCaption = MAKEINTRESOURCEW(IDS_CPL_NAME);
    psh.nPages = id;
    psh.u3.ppsp = psp;
    psh.pfnCallback = propsheet_callback;

    /* display the dialog */
    PropertySheetW(&psh);

    OleUninitialize();
}

/*********************************************************************
 * CPlApplet (tuica.cpl.@)
 *
 * Control Panel entry point
 *
 * PARAMS
 *  hWnd    [I] Handle for the Control Panel Window
 *  command [I] CPL_* Command
 *  lParam1 [I] first extra Parameter
 *  lParam2 [I] second extra Parameter
 *
 * RETURNS
 *  Depends on the command
 *
 */
LONG CALLBACK CPlApplet(HWND hwnd, UINT command, LPARAM lParam1, LPARAM lParam2)
{
    TRACE("(%p, %u, 0x%lx, 0x%lx)\n", hwnd, command, lParam1, lParam2);

    switch (command)
    {
        case CPL_INIT:
        {
            return TRUE;
        }
        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
        {
            CPLINFO *appletInfo = (CPLINFO *) lParam2;

            appletInfo->idIcon = ICO_MAIN;
            appletInfo->idName = IDS_CPL_NAME;
            appletInfo->idInfo = IDS_CPL_INFO;
            appletInfo->lData = 0;
            return TRUE;
        }

        case CPL_DBLCLK:
            display_cpl_sheets(hwnd);
            break;

        case CPL_STOP:
            break;
    }

    return FALSE;
}

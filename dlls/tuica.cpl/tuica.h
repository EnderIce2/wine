#ifndef __WINE_TUICACPL__
#define __WINE_TUICACPL__

#include <winuser.h>
#include <windef.h>
#include <commctrl.h>
#include <dinput.h>

extern HMODULE hcpl;

#define TEST_MAX_BUTTONS    32
#define TEST_MAX_AXES       4

#define NUM_PROPERTY_PAGES 3

/* strings */
#define IDS_CPL_NAME        1
#define IDS_CPL_INFO        2

/* dialogs */
#define IDC_STATIC          -1

#define IDD_MAIN                1000

#define IDC_BUTTONWALLPAPER     2001
#define IDC_BUTTONENABLETILE    2002
#define IDC_BUTTONDISABLETILE   2004
#define IDC_TEXTBOXLOCATION     2005
#define IDC_CURRENTLOCATION     2006

#define ICO_MAIN            100

#define IDC_STATIC          -1

#define IDD_LIST            1000
#define IDD_TEST            1001
#define IDD_FORCEFEEDBACK   1002

#define IDC_JOYSTICKLIST    2000
#define IDC_BUTTONDISABLE   2001
#define IDC_BUTTONENABLE    2002
#define IDC_DISABLEDLIST    2003
#define IDC_TESTSELECTCOMBO 2004
#define IDC_TESTGROUPXY     2005
#define IDC_TESTGROUPRXRY   2006
#define IDC_TESTGROUPZRZ    2007
#define IDC_TESTGROUPPOV    2008

#define IDC_FFSELECTCOMBO   2009
#define IDC_FFEFFECTLIST    2010

#define ICO_MAIN            100

/* constants */
#define TEST_POLL_TIME      100

#define TEST_BUTTON_COL_MAX 8
#define TEST_BUTTON_X       8
#define TEST_BUTTON_Y       122
#define TEST_NEXT_BUTTON_X  30
#define TEST_NEXT_BUTTON_Y  25
#define TEST_BUTTON_SIZE_X  20
#define TEST_BUTTON_SIZE_Y  18

#define TEST_AXIS_X         43
#define TEST_AXIS_Y         60
#define TEST_NEXT_AXIS_X    77
#define TEST_AXIS_SIZE_X    3
#define TEST_AXIS_SIZE_Y    3
#define TEST_AXIS_MIN       -25
#define TEST_AXIS_MAX       25

#define FF_AXIS_X           248
#define FF_AXIS_Y           60
#define FF_AXIS_SIZE_X      3
#define FF_AXIS_SIZE_Y      3

#define FF_PLAY_TIME        2*DI_SECONDS
#define FF_PERIOD_TIME      FF_PLAY_TIME/4

#endif
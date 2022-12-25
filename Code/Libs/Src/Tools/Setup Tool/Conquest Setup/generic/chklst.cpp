/*****************************************************************************
 *
 *      chklst.c
 *
 *      Wrappers that turn a listview into a checked listbox.
 *
 *      Typical usage:
 *
 *      // at app startup
 *      CheckList_Init();
 *
 *      // Dialog template should look like this:
 *
 *          CONTROL         "", IDC_TYPE_CHECKLIST, WC_LISTVIEW,
 *                          LVS_REPORT | LVS_SINGLESEL |
 *                          LVS_NOCOLUMNHEADER |
 *                          LVS_SHAREIMAGELISTS |
 *                          WS_TABSTOP | WS_BORDER,
 *                          7, 17, 127, 117
 *
 *      // Do not use the LVS_SORTASCENDING or LVS_SORTDESCENDING flags.
 *
 *      // in the dialog's WM_INITDIALOG handler
 *      hwndList = GetDlgItem(hDlg, IDC_TYPE_CHECKLIST);
 *      Checklist_OnInitDialog(hwndList);
 *
 *      // The first item added is always item zero, but you can put it
 *      // into a variable if it makes you feel better
 *      iFirst = Checklist_AddString(hwndList,
 *                                   "Checkitem, initially checked", TRUE);
 *
 *      // The second item added is always item one, but you can put it
 *      // into a variable if it makes you feel better
 *      iSecond = Checklist_AddString(hwndList,
 *                                    "Checkitem, initially unchecked", FALSE);
 *
 *      Checklist_InitFinish(hwndList);
 *
 *      // To suck out values
 *      if (Checklist_GetState(hwndList, iFirst)) {...}
 *      if (Checklist_GetState(hwndList, iSecond)) {...}
 *
 *      // At dialog box destruction
 *      Checklist_OnDestroy(hwndList);
 *
 *      // at app shutdown
 *      Checklist_Term();
 *
 *****************************************************************************/

// #include "setup.h"
#include "windows.h"
#include <commctrl.h>


#include "CheckList.h"
#include "resource.h"
#include "hotsetup.h"
#ifndef	STATEIMAGEMASKTOINDEX
#define	STATEIMAGEMASKTOINDEX(i) ((i & LVIS_STATEIMAGEMASK) >> 12)
#endif

using namespace NGLOBALS;

HIMAGELIST g_himlState;

/*****************************************************************************
 *
 *      Checklist_Init
 *
 *      One-time initialization.  Call this at app startup.
 *
 *      IDB_CHECK should refer to chk.bmp.
 *
 *****************************************************************************/

BOOL WINAPI
Checklist_Init(void)
{
    g_himlState = ImageList_LoadImage(GetResourceInst(), MAKEINTRESOURCE(IDB_CHECK),
                                      0, 4, RGB(0xFF, 0x00, 0xFF),
                                      IMAGE_BITMAP, 0);

    return (BOOL)g_himlState;
}

/*****************************************************************************
 *
 *      Checklist_Term
 *
 *      One-time shutdown.  Call this at app termination.
 *
 *****************************************************************************/

void WINAPI
Checklist_Term(void)
{
    if (g_himlState) {
        ImageList_Destroy(g_himlState);
    }
}

/*****************************************************************************
 *
 *      Checklist_OnInitDialog
 *
 *      Initialize a single checklist control.  Call this when the
 *      control is created.
 *
 *****************************************************************************/

void WINAPI
Checklist_OnInitDialog(HWND hwnd)
{
    ListView_SetImageList(hwnd, g_himlState, LVSIL_STATE);
}

/*****************************************************************************
 *
 *      Checklist_AddString
 *
 *      Add a string and a checkbox.
 *
 *****************************************************************************/

int WINAPI
Checklist_AddString(HWND hwnd, char * ptszText, BOOL fCheck)
{
    LV_ITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_STATE;
    lvi.iSubItem = 0;
    lvi.pszText = ptszText;
    lvi.state = INDEXTOSTATEIMAGEMASK(fCheck ? 2 : 1);
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    lvi.iItem = ListView_GetItemCount(hwnd);

    return ListView_InsertItem(hwnd, &lvi);
}

/*****************************************************************************
 *
 *      Checklist_InitFinish
 *
 *      Wind up the initialization.  Do this after you've added all the
 *      strings you plan on adding.
 *
 *****************************************************************************/

void WINAPI
Checklist_InitFinish(HWND hwnd)
{
    RECT rc;
    LV_COLUMN col;
    int icol;

    /*
     *  Add the one and only column.
     */
    GetClientRect(hwnd, &rc);
    col.mask = LVCF_WIDTH;
    col.cx = rc.right;
    icol = ListView_InsertColumn(hwnd, 0, &col);

    ListView_SetColumnWidth(hwnd, icol, LVSCW_AUTOSIZE);
}

/*****************************************************************************
 *
 *  CheckList_GetState
 *
 *  Read the state of a checklist item
 *
 *****************************************************************************/

BOOL WINAPI Checklist_GetState(HWND hwnd, int iItem)
{
    LV_ITEM lvi;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    ListView_GetItem(hwnd, &lvi);
    return STATEIMAGEMASKTOINDEX(lvi.state);
}



void WINAPI Checklist_SetState(HWND hwnd, int iItem, BOOL bState)
{
	LV_ITEM lvi;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
	lvi.state = INDEXTOSTATEIMAGEMASK(bState ? 2 : 1);
    ListView_SetItem(hwnd, &lvi);
}

BOOL WINAPI Checklist_IsAutomatic(HWND hwnd, int iItem)
{
    LV_ITEM lvi;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    ListView_GetItem(hwnd, &lvi);
    return STATEIMAGEMASKTOINDEX(lvi.state) > 2;
}

BOOL WINAPI Checklist_IsChecked(HWND hwnd, int iItem)
{
    LV_ITEM lvi;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    ListView_GetItem(hwnd, &lvi);
    return !(STATEIMAGEMASKTOINDEX(lvi.state) % 2);
}


/*****************************************************************************
 *
 *      Checklist_OnDestroy
 *
 *      Clean up a checklist.  Call this before destroying the window.
 *
 *****************************************************************************/

void WINAPI
Checklist_OnDestroy(HWND hwnd)
{
}

//
//	widclass.cpp
//
//
//
//

#include "windows.h"
#include "stdio.h"
#include "string.h"
#include "hotsetup.h"
#include "widclass.h"
#include "resource.h"

using namespace NGLOBALS;

// defined in setup.cpp
extern int X0;
extern int Y0;

HINSTANCE hInst;
Dialog *glpDialog=NULL;
Container::Container()
{
  int ObjCount = 0;
  ObjArray = NULL;
}

Container::~Container()
{
	int x;
	if(ObjArray == NULL)
		return;
	for(x=0;x<ObjCount;x++)
	{
		if(ObjArray[x] != (Object *)NULL)
		{
			switch(ObjArray[x]->objType)
			{
			   case PUSHBUTTON:
				   delete (PushButton *)ObjArray[x];
			      break;
			}
		}
	}
	delete[] ObjArray;
}
BOOL Container::Add(Object *Obj)
{
	if(ObjCount == 0)
	{
		ObjArray = new Object *[5];
		if(ObjArray == NULL)
			return FALSE;
		memset(ObjArray,0,sizeof(Object *)*5);
		ObjCount = 1;
		ObjArray[0] = Obj;
		return TRUE;
	}
	else
	{
		int x;
		for(x = 0;x<ObjCount;x++)
		{
			if(ObjArray[x] == NULL)
			{
				ObjArray[x] = Obj;
				return (TRUE);
			}
		}
		Object **TempArray = new Object *[ObjCount+5];
		if(TempArray == NULL)
			return FALSE;
		memset(TempArray,0,sizeof(Object *)*(ObjCount+5));
		memmove((void *)TempArray,(void *)ObjArray,sizeof(Object *)*ObjCount);
		delete[] ObjArray;
		ObjArray = TempArray;
		ObjArray[ObjCount] = Obj;
		ObjCount += 5;
		return TRUE;
	}
	return FALSE;
}

BOOL Container::Remove(Object *Obj)
{
	int x;
	if(Obj == NULL)
		return (TRUE);
	for(x=0;x<ObjCount;x++)
	{
		if(ObjArray[x] == Obj)
		{
			switch(ObjArray[x]->objType)
			{
			   case PUSHBUTTON:
					delete (PushButton *)ObjArray[x];
					break;
			   default:
					delete ObjArray[x];
					break;
			}
			ObjArray[x] = NULL;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Container::ProcessMessage(WORD message,WPARAM wParam,LPARAM lParam)
{
	int x;
	for(x=0;x<ObjCount;x++)
	{
		if(ObjArray[x] == NULL)
			continue;
	
	   int objType = ObjArray[x]->objType;
	   switch(message)
	   {
		case WM_COMMAND:
		switch (objType)
		{
			case PUSHBUTTON:
			switch(HIWORD(wParam))
			{
			case 0:
			case 1:
				if(((PushButton *)ObjArray[x])->baseID == LOWORD(wParam))
		 		    ((PushButton *)ObjArray[x])->ProcessCommand(BN_CLICKED);
				break;
			default:
				if(((PushButton *)ObjArray[x])->hButWnd == (HWND)lParam)
		 		    ((PushButton *)ObjArray[x])->ProcessCommand(HIWORD(wParam));
				break;
			}
		}
			break;
		case WM_DRAWITEM:
			switch(objType)
			{
			case PUSHBUTTON:
				if(((PushButton *)ObjArray[x])->hButWnd == (HWND)((LPDRAWITEMSTRUCT)lParam)->hwndItem)
				    return ( (PushButton *)ObjArray[x])->Draw((LPDRAWITEMSTRUCT)lParam);
				break;
			}
			break;
		case WM_MEASUREITEM:
			switch(objType)
			{
			case PUSHBUTTON:
				return FALSE;
				break;
			}
			break;
	   }
	}
	return FALSE;
}

void PushButton::ProcessCommand(WORD nCode)
{

	switch(nCode)
	{
	case BN_CLICKED:

		if(bEnabled)
		{
			if(*SoundFile)

				EBUPlaySound(SoundFile, GetResourceInst(), SND_RESOURCE | SND_ASYNC);
			ButtonClicked();	   
		}
		break;
	}
	return;
}
PushButton::PushButton(HWND hWnd,BUTTONRECT *br,char *sndfile)
{
	parenthWnd = hWnd;
	memset(&DIS,0,sizeof(DRAWITEMSTRUCT));
	objType = PUSHBUTTON;
	baseID = br->BitID;
	bEnabled = TRUE;
	if(br!=NULL)
		memmove(&butrect,br,sizeof(BUTTONRECT));
	strcpy(SoundFile,sndfile);
	hButWnd = CreateWindow("button",NULL,WS_VISIBLE  | WS_CHILD | WS_CLIPSIBLINGS | BS_PUSHBUTTON | WS_TABSTOP,
		br->ButtonRect.left+X0,br->ButtonRect.top+Y0,br->ButtonRect.right,
		br->ButtonRect.bottom,hWnd,(HMENU)br->BitID,
		hInst,NULL);
	setTitle(br->BitID);
	InvalidateRect(hButWnd,NULL,FALSE);
	if(!hButWnd)
	{
		DWORD err = GetLastError();
	    hButWnd = (HWND)err;
	}
}

BOOL PushButton::Paint()
{
	if(DIS.hwndItem != NULL)
	{
       DIS.hDC=GetDC(hButWnd);
       DIS.rcItem.top = DIS.rcItem.left = 0;
	   DIS.rcItem.bottom = butrect.ButtonRect.bottom;// - butrect.ButtonRect.top;
	   DIS.rcItem.right = butrect.ButtonRect.right;// - butrect.ButtonRect.left;
	   Draw(&DIS);
	   ReleaseDC(hButWnd,DIS.hDC);
	   return (TRUE);
	}

	return FALSE;
}

BOOL PushButton::Draw(LPDRAWITEMSTRUCT lpDIS)
{
    BITMAP   Bitmap;			// bitmap struct
    HBITMAP  hBitmap;			// handle to bitmap
    HDC      hBitmapDC;			// display context for map
    HBITMAP  hOldBitmap;		// dc old bit map
    RECT     rcInside;			// update rect
    register int x, y, cx, cy;		// counters
    WORD     wBitmapOffset = 0; // etc.
	memmove(&DIS,lpDIS,sizeof(DRAWITEMSTRUCT));

    switch (lpDIS->CtlType)	// type of control, we only deal with buttons
        {
        case ODT_BUTTON :
            switch (lpDIS->itemAction) // what to do with button
                {
                case ODA_DRAWENTIRE :
                case ODA_SELECT :
                case ODA_FOCUS :		// all the ops we handle
				
                    if (lpDIS->itemState &ODS_SELECTED)	// is selected (down)
                        hBitmap = LoadResourceBitmap (GetResourceInst(), MAKEINTRESOURCE (butrect.BitID+ ((bEnabled ) ? 1 : 0)));
                    else
                        hBitmap = LoadResourceBitmap (GetResourceInst(), MAKEINTRESOURCE (IDB_CHECK));
//                        hBitmap = LoadResourceBitmap (GetResourceInst(), MAKEINTRESOURCE (butrect.BitID));
                    CopyRect ((LPRECT) &rcInside, &lpDIS->rcItem);	// copy the area to update
                    FillRect (lpDIS->hDC, &rcInside, (HBRUSH)GetStockObject (BLACK_BRUSH));
                    // calculte the size of the map
                    cx = lpDIS->rcItem.right - lpDIS->rcItem.left;
                    cy = lpDIS->rcItem.bottom - lpDIS->rcItem.top;
                    // get the bitmap itself (at least it's control info)
                    GetObject (hBitmap, sizeof (BITMAP), (LPSTR) &Bitmap);
                    // make sure map is positioned within the buttons
                    x = 0;
                    y = 0;
		              // we do our drawing inside a compatible dc, not the original dc
                    hBitmapDC = CreateCompatibleDC (lpDIS->hDC);
                    // select in the button bitmap
                    if ((hOldBitmap = (HBITMAP)SelectObject (hBitmapDC, hBitmap)))
                    {
                        int iDiff = cx - Bitmap.bmWidth;  // Center bitmap
                        if (iDiff > 0)
                           x += iDiff/2;
                        if ((iDiff = cy - Bitmap.bmHeight) > 0)
                           y += iDiff/2;

                        // blt it about to fit into the window area
                        BitBlt (lpDIS->hDC, 0, 0,
                        Bitmap.bmWidth, Bitmap.bmHeight, hBitmapDC, 0, 0, SRCCOPY);
                        // restore the old map
                        SelectObject (hBitmapDC, hOldBitmap);
                    }
                    // clean up
                    DeleteDC (hBitmapDC);
                    DeleteObject (hBitmap);
                    break;
                }
        }
	return TRUE;
}
HBITMAP PushButton::LoadResourceBitmap(HINSTANCE hInst, LPSTR lpString)
{
    HPALETTE hPal;
	HBITMAP hBitmap = ::LoadResourceBitmap(hInst,parenthWnd,lpString,&hPal);
   if(hPal)
	   DeleteObject(hPal);
   return hBitmap;
}

EBURETCODE Dialog::start(LPARAM lParam)
{
   return (EBURETCODE)DialogBoxParam(GetResourceInst(),szTemplate, GetWndParent(),(DLGPROC)CPPDlgProc,(LPARAM)lParam);
}
EBURETCODE Dialog::start()
{
   return (EBURETCODE)DialogBox(GetResourceInst(),szTemplate, GetWndParent(),(DLGPROC)CPPDlgProc);
}

_declspec(dllexport) BOOL CALLBACK CPPDlgProc (HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
	if(glpDialog)
		return glpDialog->ProcessCommand(hDlg,msg,wParam,lParam);
	else
		return FALSE;
}

BOOL Dialog::ProcessCommand(HWND hwnd, WORD msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_NOTIFY:
		return Notify(wParam, lParam);
		case WM_INITDIALOG:
		    hDlg = hwnd;
		    return Init(lParam);
	    case WM_COMMAND:
		switch(LOWORD(wParam))
		{
			case IDOK:
				return Ok();
			case IDCANCEL:
				return Cancel();
			case IDHELP:
				return(Help(HIWORD(wParam)));
			default:
				return Command(LOWORD(wParam),HIWORD (wParam),lParam);
        }
		case WM_ACTIVATE:
		    return ((LOWORD(wParam) == WA_INACTIVE) ? Activate(FALSE) : Activate(TRUE));
		case WM_DESTROY:
		    return Destroy();
		default:
		    return FALSE;
    }
}
//----------------------------------------------------------------------------
// Procedure    LoadResourceBitmap
//
// Purpose      Load a DIB from app resource, create a palette for it,
//				and convert make a DDB out of it.
//
// Parameters   hInstance- handle of application instance.
//				lpString- address of bitmap resource name.
//				lphPalette- address of logical palette handle.
//
// Return       If successful, returns handle to DDB; Otherwise, returns NULL.
//
HBITMAP LoadResourceBitmap(HINSTANCE hInstance, HWND hWnd,LPSTR lpString, HPALETTE * lphPalette)
{
    HRSRC  hRsrc;
    HGLOBAL hGlobal;
    HBITMAP hBitmapFinal = NULL;
    LPBITMAPINFOHEADER  lpbi;
    HDC hdc;
    int iNumColors;

    if (hRsrc = FindResource(hInstance, lpString, RT_BITMAP))
       {
       hGlobal = LoadResource(hInstance, hRsrc);
       lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);

       hdc = GetDC(hWnd);
	   if(lphPalette)
	   {
          *lphPalette =  CreateDIBPalette (hdc,(LPBITMAPINFO)lpbi, &iNumColors);
          if (*lphPalette)
             {
             SelectPalette(hdc,*lphPalette,FALSE);
             RealizePalette(hdc);
             }
       }
       hBitmapFinal = CreateDIBitmap(hdc,
                   (LPBITMAPINFOHEADER)lpbi,
                   (LONG)CBM_INIT,
                   (LPSTR)lpbi + lpbi->biSize + iNumColors *sizeof(RGBQUAD),
                    (LPBITMAPINFO)lpbi,
                   DIB_RGB_COLORS );

       ReleaseDC(hWnd,hdc);
       UnlockResource(hGlobal);
       FreeResource(hGlobal);
       }
    return (hBitmapFinal);
}
//----------------------------------------------------------------------------
// Procedure    CreateDIBPalette
//
// Purpose      Creates a logical palette based on the bitmap info passed in.
//
// Parameters   lpbmi- address of structure with bitmap data.
//				lpiNumColors- address of the number of colors in bitmap
//
// Return       If the function succeeds, the return value is a handle that
//				identifies a logical palette for the bitmap.  Otherwise return NULL.
//
HPALETTE CreateDIBPalette (HDC hdc,LPBITMAPINFO lpbmi, LPINT lpiNumColors)
{
   LPBITMAPINFOHEADER  lpbi;
   LPLOGPALETTE     lpPal;
   HANDLE           hLogPal;
   HPALETTE         hPal = NULL;
   PALETTEENTRY		PalEntStart[10];
   PALETTEENTRY		PalEntEnd[10];
   int              i;

   GetSystemPaletteEntries(hdc,0,10,&PalEntStart[0]);
   GetSystemPaletteEntries(hdc,246,10,&PalEntStart[0]);
   lpbi = (LPBITMAPINFOHEADER)lpbmi;
   if(lpbi->biClrUsed != 0)
	   *lpiNumColors = lpbi->biClrUsed;
   else if (lpbi->biBitCount <= 8)
       *lpiNumColors = (1 << lpbi->biBitCount);
   else
       *lpiNumColors = 0;  // No palette needed for 24 BPP DIB

   if (*lpiNumColors)
      {
      hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) +
                             sizeof (PALETTEENTRY) * (*lpiNumColors));
      lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
      lpPal->palVersion    = 0x300;
      lpPal->palNumEntries = *lpiNumColors;

      for (i = 0;  i < *lpiNumColors;  i++)
         {
         lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
         lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
         lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
         lpPal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
         }
	  for(i=0;i<10;i++)
	  {
         lpPal->palPalEntry[i].peRed   = PalEntStart[i].peRed;
         lpPal->palPalEntry[i].peGreen = PalEntStart[i].peGreen;
         lpPal->palPalEntry[i].peBlue  = PalEntStart[i].peBlue;
         lpPal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
		 if(*lpiNumColors > 246)
		 {
            lpPal->palPalEntry[i+246].peRed   = PalEntEnd[i].peRed;
            lpPal->palPalEntry[i+246].peGreen = PalEntEnd[i].peGreen;
            lpPal->palPalEntry[i+246].peBlue  = PalEntEnd[i].peBlue;
            lpPal->palPalEntry[i+246].peFlags = PC_NOCOLLAPSE;
		 }
	  }
      hPal = CreatePalette (lpPal);
      GlobalUnlock (hLogPal);
      GlobalFree   (hLogPal);
   }
   return hPal;
}



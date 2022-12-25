//--------------------------------------------------------------------------//
//                                                                          //
//                             VfxShapeViewer.cpp                           //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jasony $

    $Header: /Libs/Src/Viewer/VfxView/VfxShapeViewer.cpp 10    4/21/98 6:26p Jasony $
*/			    
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "VfxShapeViewer.h"
#include "Document.h"
#include "IConnection.h"
#include "Vfx.h"
#include "HeapObj.h"

#include <commdlg.h>
#include <stdlib.h>


HINSTANCE hInstance;	

static char interface_name[] = "IViewer";

#define IDM_EDIT_COLOR  0x2000
//--------------------------
// WinVfx16.asm
extern "C"{
S32  WINAPI VFX_shape_scan8 (PANE *pane, U32 transparent_color, S32 hotX, S32 hotY, VFX_SHAPETABLE *shape_table);
void WINAPI VFX_shape_draw8 (PANE *pane, VFX_SHAPETABLE *shape_table, S32 shape_number, S32 hotX, S32 hotY);
void WINAPI VFX_shape_draw_unclipped8 (PANE *pane, VFX_SHAPETABLE *shape_table, S32 shape_number, S32 hotX, S32 hotY);
void WINAPI VFX_shape_palette (VFX_SHAPETABLE *shape_table, S32 shape_num, VFX_RGB *palette);
S32  WINAPI VFX_shape_colors (VFX_SHAPETABLE *shape_table, S32 shape_num, VFX_CRGB *colors);
S32  WINAPI VFX_shape_bounds (VFX_SHAPETABLE *shape_table, S32 shape_num);
} // end extern "C"

/*
   Viewer of VFX shape files.
   <- arrow key decrements the current displayed shape
   -> arrow key increments the current displayed shape
*/

static COLORREF g_TransparentColor = RGB(255,0,128);

//--------------------------------------------------------------------------
//---------------------------Viewer class-------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
Viewer::~Viewer (void)
{
	if (hDIB != NULL)
	{
		DeleteObject(hDIB);
		hDIB = NULL;
	}
	if (hPalette != NULL)
	{
		DeleteObject(hPalette);
		hPalette = NULL;
	}

	if (pLogPal)
	{
		free(pLogPal);
		pLogPal = 0;
	}

	if (pbmi)
	{
		free(pbmi);
		pbmi = 0;
	}

	if (hMainWindow)
		DestroyWindow(hMainWindow);
	hMainWindow=0;

	if (doc)
		OnClose(doc);
}
//--------------------------------------------------------------------------
//
void * Viewer::operator new (size_t size)
{
	return calloc(size, 1);
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::set_display_state (BOOL32 state)
{
 	bVisible = (state != 0);
	if (hMainWindow)
	{
		ShowWindow(hMainWindow, (bVisible) ? SW_SHOW:SW_HIDE);
		return GR_OK;
	}
	return GR_GENERIC;
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::get_display_state (BOOL32 *state)
{
	if (state)
	{
		*state = bVisible;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::get_class_name (C8 *name)
{
	if (name)
	{
		strcpy(name, szClassName);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::set_instance_name (const C8 *name)
{
	if (name)
	{
		strncpy(szInstanceName, name, sizeof(szInstanceName)-1);
		SetWindowText(hMainWindow, szInstanceName);
		bNameOverriden=1;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::get_instance_name (C8 *name)
{
	if (name)
	{
		strcpy(name, szInstanceName);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::get_main_window (void **hwnd)
{
	if (hwnd)
	{
		*hwnd = hMainWindow;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::set_rect (const struct tagRECT *pRect)
{
	if (pRect)
	{
		rect = *pRect;
		if (hMainWindow)
			MoveWindow(hMainWindow, rect.left, rect.top, rect.right - rect.left, rect.bottom-rect.top, TRUE);
		else
			return GR_GENERIC;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::get_rect (struct tagRECT *pRect)
{
	if (pRect)
	{
		*pRect = rect;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::set_read_only (BOOL32 value)
{
	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::get_read_only (BOOL32 *value)
{
	if (value)
	{
		*value = 1;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
void Viewer::refreshSurface (HDC hdc)
{
	SelectPalette(hdc, hPalette, 0);
	
	if (bPaletteChanged)
	{
		bPaletteChanged = 0;
		RealizePalette(hdc);
	}

	//
	// Disable Boolean operations during stretching
	//

	//   SetStretchBltMode(hdc, COLORONCOLOR);


	//
	// Stretch bitmap to conform to the size of the output window
	//

	U32 width = dwClientWidth;

	if ((S32)dwClientHeight <= -pbmih->biHeight && pbmih->biWidth < nMinWinX)
		width = pbmih->biWidth;

	if (hDIB != NULL)
		StretchDIBits(hdc,    // Destination DC
			  0,              // Destination X
			  0,              // Destination Y
			  width,		  // Destination (client area) width
			  dwClientHeight, // Destination (client area) height 
			  0,              // Source X
			  0,              // Source Y
			  pbmih->biWidth, // Source (back buffer) width
			  -pbmih->biHeight,// Source (back buffer) height
			  lpDIBBuffer,    // Pointer to source (back buffer)
			  pbmi,           // Bitmap info for back buffer
			  DIB_RGB_COLORS, // Bitmap contains index values
			  SRCCOPY);       // Do normal copy with stretching
}
//--------------------------------------------------------------------------
//
static U32 __stdcall getMaxWidth (VFX_SHAPETABLE * pData)
{
	int i = pData->shape_count;
	U32 maxX = 0, tmpX;

	while (i-- > 0)
	{
//		tmpX = VFX_shape_resolution(pData, i);
		tmpX = VFX_shape_bounds(pData, i);
		tmpX >>= 16;
		if (tmpX > maxX)
			maxX = tmpX;
	}
	
	return maxX;
}
//--------------------------------------------------------------------------
// Called when a document client has modified the document.
// '_doc' may be our document, or a child document. (sub document)
//  This viewer ignores the 'message' and 'parm' parameters, and always
//  updates all of its data.
//
GENRESULT Viewer::OnUpdate (struct IDocument *_doc, const C8 *message, void *parm)
{
	DWORD dwRead;
	VFX_SHAPETABLE * pData;
	HDC hdc;
	VFX_RGB vfx_palette[256];
	int i;
	RECT tRect;
	PANE pane;
	VFX_WINDOW window;
	DWORD dwResX, dwResY;
	char fileName[64];
	char buffer[128];

	if (doc != _doc)			// is this our document? (ignore updates from child docs)
		return GR_GENERIC;

	if (hDIB != NULL)
	{
		DeleteObject(hDIB);
		hDIB = NULL;
	}
	if (hPalette != NULL)
	{
		DeleteObject(hPalette);
		hPalette = NULL;
	}
	
	dwRead = doc->GetFileSize();
	if ((pData = (VFX_SHAPETABLE *) malloc(dwRead)) == 0)
		return GR_OUT_OF_MEMORY;

	doc->SetFilePointer(0,0);
	if (doc->ReadFile(0, pData, dwRead, &dwRead, 0) == 0)
	{
		return GR_FILE_ERROR;
	}

	// do something with data

	if (currentImage >= pData->shape_count)
		currentImage = pData->shape_count-1;

//	dwResX = VFX_shape_resolution(pData, currentImage);
	dwResX = VFX_shape_bounds(pData, currentImage);
	dwResY = (U32) ((U16)dwResX);
	dwResX >>= 16;

	tRect.top = tRect.left = 0;
	tRect.bottom = dwResY - 1;

	S32 widest = (getMaxWidth(pData)+3)&~3;
	nMinWinX = __max(nMinWinX, widest);
	tRect.right = nMinWinX - 1;

	AdjustWindowRect(&tRect, GetWindowLong(hMainWindow, GWL_STYLE), 0);

	SetWindowPos(hMainWindow, HWND_TOP, 
		rect.left, rect.top, 
		tRect.right - tRect.left + 1,
		tRect.bottom - tRect.top  + 1,
		SWP_NOCOPYBITS | SWP_NOZORDER);
	
	GetWindowRect(hMainWindow, &rect);

	pbmih->biSize          =  sizeof(BITMAPINFOHEADER);
	pbmih->biWidth         =  (dwResX+3)&~3;
	pbmih->biHeight        = -(long)dwResY;
	pbmih->biPlanes        =  1;
	pbmih->biBitCount      =  8;
	pbmih->biSizeImage     =  0;
	pbmih->biXPelsPerMeter =  0;
	pbmih->biYPelsPerMeter =  0;
	pbmih->biClrUsed       =  0;
	pbmih->biClrImportant  =  0;
	pbmih->biCompression = BI_RGB;

	hdc = GetDC(hMainWindow);
	
	hDIB = CreateDIBSection(hdc,             // Device context
		pbmi,            // BITMAPINFO structure
		DIB_RGB_COLORS,  // Color data type
		&lpDIBBuffer,     // Address of image map pointer
		NULL,            // File
		0);              // Bitmap file offset

	ReleaseDC(hMainWindow, hdc);
	
	if (hDIB == NULL)
		return GR_GENERIC;

	VFX_shape_palette(pData, currentImage, vfx_palette);

	//
	// Update palette array
	//

	// testing!!
	vfx_palette[255].r = GetRValue(g_TransparentColor);
	vfx_palette[255].g = GetGValue(g_TransparentColor);
	vfx_palette[255].b = GetBValue(g_TransparentColor);
	// end of test
	
	for (i=0; i < 256; i++)
	{
		pLogPal->palPalEntry[i].peRed   = pbmi->bmiColors[i].rgbRed   = (unsigned char) vfx_palette[i].r;
		pLogPal->palPalEntry[i].peGreen = pbmi->bmiColors[i].rgbGreen = (unsigned char) vfx_palette[i].g;
		pLogPal->palPalEntry[i].peBlue  = pbmi->bmiColors[i].rgbBlue  = (unsigned char) vfx_palette[i].b;
		pLogPal->palPalEntry[i].peFlags = NULL;
	}

	hPalette = CreatePalette(pLogPal);
	bPaletteChanged = TRUE;

	window.buffer = lpDIBBuffer;                // Pointer to window buffer
	window.x_max = ((dwResX+3)&~3)-1;
	window.y_max = dwResY-1;
	window.pixel_pitch = 1;
	window.bytes_per_pixel = 1;

	// testing!!!
	memset(lpDIBBuffer, 0xFF, ((dwResX+3)&~3)*dwResY);


	pane.window = &window;
	pane.x0 = pane.y0 = 0;
	pane.x1 = dwResX-1;
	pane.y1 = dwResY-1;

	VFX_shape_draw8(&pane, pData, currentImage, 0, 0);

	// 
	// if not overriden, set title bar
	//

	if (bNameOverriden == 0)
	{
		doc->GetFileName(fileName, sizeof(fileName));
		wsprintf(buffer, "%s - #%d [%d x %d]", fileName, currentImage, dwResX, dwResY);
		SetWindowText(hMainWindow, buffer);
	}

	InvalidateRect(hMainWindow, 0, 1); 

	free(pData);

	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::OnClose (struct IDocument *document)
{
	if (document == doc && doc)	//is this the right instance?
	{
		COMPTR<IDAConnectionPoint> connection;

		if (doc->QueryOutgoingInterface("IDocumentClient", connection) != GR_OK)
			return GR_GENERIC;

		if (hDIB != NULL)
		{
			DeleteObject(hDIB);
			hDIB = NULL;
		}
		if (hPalette != NULL)
		{
			DeleteObject(hPalette);
			hPalette = NULL;
		}

		if (pLogPal)
		{
			free(pLogPal);
			pLogPal = 0;
		}

		if (pbmi)
		{
			free(pbmi);
			pbmi = 0;
		}

		if (hMainWindow)
			DestroyWindow(hMainWindow);
		hMainWindow = 0;

		connection->Unadvise(connHandle);
		doc = 0;
		GetBase()->Release();		// get rid our extra reference
	}

	return GR_OK;
}
//--------------------------------------------------------------------------
//
LRESULT Viewer::MainWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO) lParam; // address of structure 

			nMinWinX = __max(nMinWinX, lpmmi->ptMinTrackSize.x);
		}
		break;

	case WM_CREATE:
		hMainWindow = hwnd;
		return 0; 	// end WM_CREATE case

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;

			hdc = BeginPaint(hwnd, &ps);
			refreshSurface(hdc);
			EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_SIZE:
		dwClientWidth = LOWORD(lParam);
		dwClientHeight = HIWORD(lParam);
		// fall through intentional
	case WM_MOVE:
		GetWindowRect(hwnd, &rect);
		break;

	case WM_KEYDOWN:
		if ((TCHAR)wParam == VK_LEFT)
		{
			if (currentImage > 0)
			{
				currentImage--;
				OnUpdate(doc);
			}
		}
		else
		if ((TCHAR)wParam == VK_RIGHT)
		{
			currentImage++;
			OnUpdate(doc);
		}
		break;

	case WM_PALETTECHANGED:
		if ((HWND) wParam == hwnd)
		{
			break;
		}
		// fall through intentional
	case WM_QUERYNEWPALETTE:
		bPaletteChanged = TRUE;
		InvalidateRect(hwnd, 0, 1);
		break;

	case WM_CLOSE:
		set_display_state(0);
		return 0;

	case WM_DESTROY:
		if (hMainWindow==hwnd)
		{
			bVisible=0;
			hMainWindow=0;
		}
		break;

	case WM_SYSCOMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_EDIT_COLOR:
			{
				CHOOSECOLOR choose;
				COLORREF colors[16];

				memset(&choose, 0, sizeof(choose));
				choose.lStructSize = sizeof(choose);
				choose.hwndOwner = hwnd;
				choose.rgbResult = g_TransparentColor;
				choose.lpCustColors = colors;
				choose.Flags = CC_FULLOPEN | CC_RGBINIT;

				if (ChooseColor(&choose))
				{
					g_TransparentColor = choose.rgbResult;
					OnUpdate(doc);
				}
			}
			break;
		}
		break;


	} // end switch (message)

	return DefWindowProc(hwnd, message, wParam, lParam);
}
//--------------------------------------------------------------------------
//
LRESULT CALLBACK Viewer::StaticWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	if (message == WM_CREATE)
		SetWindowLong(hwnd, GWL_USERDATA, 
			(long)((CREATESTRUCT *)lParam)->lpCreateParams);	// store "this" pointer in dialog data

	Viewer *viewer = (Viewer *) GetWindowLong(hwnd, GWL_USERDATA);

	if (viewer)
		return viewer->MainWndProc(hwnd, message, wParam, lParam);
	else
		return DefWindowProc(hwnd, message, wParam, lParam);
}
//--------------------------------------------------------------------------
//
GENRESULT Viewer::init (VIEWDESC *info)
{
	GENRESULT result = GR_OK;
	COMPTR<IDAConnectionPoint> connection;
	HMENU hMenu;

	if (strcmp(info->className, "VFX_SHAPE") != 0)
	{
		result = GR_INTERFACE_UNSUPPORTED;
		goto Done;
	}

	if (info->doc == 0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	if (info->doc->QueryOutgoingInterface("IDocumentClient", connection) != GR_OK)
	{
		result = GR_GENERIC;
		goto Done;
	}

	doc = info->doc;
	strcpy(szClassName, info->className);

	hMainWindow = CreateWindow("VFX_SHAPE_VIEWER", "No Name",
			WS_CAPTION|WS_POPUP|WS_THICKFRAME|WS_SYSMENU, 
			rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			(HWND)info->hOwnerWindow,
			0,
			hInstance,
			this);

	if (hMainWindow == 0)
	{
		result = GR_GENERIC;
		goto Done;
	}

	if ((hMenu = GetSystemMenu(hMainWindow, 0)) != 0)
	{
		MENUITEMINFO minfo;

		memset(&minfo, 0, sizeof(minfo));
		minfo.cbSize = sizeof(minfo);
		minfo.fMask = MIIM_ID | MIIM_TYPE;
		minfo.fType = MFT_STRING;
		minfo.wID = IDM_EDIT_COLOR;
		minfo.dwTypeData = const_cast<char *>("Transparent Color");
		minfo.cch = 17;		// strlen(ptr);
				
		InsertMenuItem(hMenu, 0x7FFE, 1, &minfo);
	}

	if (connection->Advise((IDADispatch *)this, &connHandle) != GR_OK)
	{
		result = GR_GENERIC;
		goto Done;
	}

	pLogPal = (LOGPALETTE *) 
		HEAP->ClearAllocateMemory(sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 256), "LOGPALETTE");
	
	if (pLogPal == NULL)
	{
		result = GR_GENERIC;
		goto Done;
	}
	
	pLogPal->palVersion    = 0x300;
	pLogPal->palNumEntries = 256;
	
	pbmi = (BITMAPINFO *) 
		HEAP->ClearAllocateMemory(sizeof (BITMAPINFOHEADER) + (sizeof (RGBQUAD) * 256), "BITMAPINFO");
	
	if (pbmi == NULL)
	{
		result = GR_GENERIC;
		goto Done;
	}
	
	pbmih = &(pbmi->bmiHeader);

	// done init, read in the data

	OnUpdate(info->doc);
	GetBase()->AddRef();		// keep an extra reference to ourself

Done:
	return result;
}

//--------------------------------------------------------------------------//
// Called by the DLL startup code, registers the ViewerFactory with the DACO manager.
//
void RegisterViewer (ICOManager * DACOM)
{
	IComponentFactory *factory;
	WNDCLASS wndclass;

	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = Viewer::StaticWndProc;
	wndclass.hInstance = hInstance;
	wndclass.hCursor = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));
	wndclass.lpszClassName = "VFX_SHAPE_VIEWER";
	wndclass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);

	if (RegisterClass(&wndclass) != 0 && (factory = new DAComponentFactory<DAComponent<Viewer>,VIEWDESC> ("IViewer")) != 0)
	{
		DACOM->RegisterComponent(factory, interface_name);
		factory->Release();
	}
}

//--------------------------------------------------------------------------
//  
void main (void)
{
}
//--------------------------------------------------------------------------
//  
static void SetDllHeapMsg (void)
{
	DWORD dwLen;
	char buffer[260];
	
	dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));
 
	while (dwLen > 0)
	{
		if (buffer[dwLen] == '\\')
		{
			dwLen++;
			break;
		}
		dwLen--;
	}

	SetDefaultHeapMsg(buffer+dwLen);
}
//--------------------------------------------------------------------------//
//
BOOL WINAPI DllMain (HINSTANCE hinstance, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			hInstance = hinstance;
			HEAP = HEAP_Acquire();
			SetDllHeapMsg();
			ICOManager * DACOM;
			
			if ((DACOM = DACOM_Acquire()) != 0)
			{
				RegisterViewer(DACOM);
			}
		}
	}

	return 1;
}

//------------------------------------------------------------------------------//
//---------------------------END VfxShapeViewer.cpp-----------------------------//
//------------------------------------------------------------------------------//


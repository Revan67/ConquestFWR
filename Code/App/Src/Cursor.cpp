//--------------------------------------------------------------------------//
//                                                                          //
//                                Cursor.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/Cursor.cpp 46    11/08/00 12:10p Jasony $

*/
//--------------------------------------------------------------------------//


#include "pch.h"
#include <globals.h>

#include "TResource.h"
#include "Cursor.h"
#include "resource.h"
#include "Hotkeys.h"
#include "DBHotkeys.h"
#include "EventPriority.h"
#include "IImageReader.h"
#include "DrawAgent.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "VideoSurface.h"

#include <RendPipeline.h>
#include <TSmartPointer.h>
#include <EventSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <IConnection.h>
#include <TConnContainer.h>
#include <TConnPoint.h>
#include <HeapObj.h>
#include <FileSys.h>
#include <WindowManager.h>

#include <DInput.h>

#include <commctrl.h>


#define DBLCLICK_PERIOD  250
#define MAX_CURSOR_SPEED 10
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------
//
static void create_tmp_name (char * buffer)
{
	WIN32_FIND_DATA fdata;
	HANDLE hFF;
	
	int len = GetCurrentDirectory(MAX_PATH, buffer);
	buffer[len++] = '\\';
	buffer[len] = 0;
	if ((hFF = FindFirstFile("Dump??.bmp", &fdata)) == INVALID_HANDLE_VALUE)
	{
		strcpy(buffer+len, "Dump01.bmp");
	}
	else
	{
		int maxNum = 0, fnum;
		do
		{
			fnum = atoi(fdata.cFileName+4);
			maxNum = __max(maxNum, fnum);
		} while (FindNextFile(hFF, &fdata));

		FindClose(hFF);
		fnum = maxNum+1;
		// write the next number back
		fdata.cFileName[4] = (fnum / 10) + '0';
		fdata.cFileName[5] = (fnum % 10) + '0';
		strcpy(buffer+len, fdata.cFileName);
	}
}
//--------------------------------------------------------------------------
//
#if 1
static void WriteScreenShot (void)
{
	char buffer[MAX_PATH+4];
	DAFILEDESC fdesc = buffer;
	COMPTR<IFileSystem> file;
	BITMAPINFO       *pbmi=0;               // BITMAPINFO structure
	BITMAPV4HEADER   *pbmih;              // Pointer to pbmi header
	DWORD dwWritten, dwNumber=0;
	WORD wBM = 'MB';   // "BM" header bytes
	COLORREF *pBits=0;

	create_tmp_name(buffer);

	fdesc.dwDesiredAccess |= GENERIC_WRITE;
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (DACOM->CreateInstance(&fdesc, file) != GR_OK)
	{
		CQERROR1("File create failed on %s", fdesc.lpFileName);
		goto Done;
	}

	pbmi = (BITMAPINFO *) calloc(sizeof (BITMAPV4HEADER) + sizeof(RGBQUAD), 1);
	pbmih = (BITMAPV4HEADER *) &(pbmi->bmiHeader);

	pbmih->bV4Size			=  sizeof(BITMAPV4HEADER);
	pbmih->bV4Width         =  SCREENRESX;
	pbmih->bV4Height        =  SCREENRESY;
	pbmih->bV4Planes        =  1;
	pbmih->bV4BitCount      =  24;
	pbmih->bV4V4Compression =  BI_RGB;
	pbmih->bV4SizeImage     =  0; // SCREENRESX * SCREENRESY * 3;

	pBits = (COLORREF *) calloc((SCREENRESX * SCREENRESY * 3) + sizeof(COLORREF), 1);
	memset(pBits, 0x34, (SCREENRESX * SCREENRESY * 3) + sizeof(COLORREF));

	if (SURFACE->LockFrontBuffer())
	{
		S32 i, j;
		COLORREF * buffer = pBits;
		const PANE & pane = SURFACE->GetPane();

		for (j = SCREENRESY-1; j >= 0; j--)
			for (i = 0; i < S32(SCREENRESX); i++)
			{
				COLORREF color = PixelToColorRef(VFX_pixel_read(&pane, i, j));
				*buffer = RGB(GetBValue(color),GetGValue(color),GetRValue(color));
				buffer = (COLORREF *) (((char *)buffer) + 3);
			}


		SURFACE->UnlockFrontBuffer();
	}
	else
	{
		S32 i, j;
		COLORREF * buffer = pBits;
		for (j = SCREENRESY-1; j >= 0; j--)
			for (i = 0; i < S32(SCREENRESX); i++)
			{
				COLORREF color = GetPixel(hMainDC, i, j);
				*buffer = RGB(GetBValue(color),GetGValue(color),GetRValue(color));
				buffer = (COLORREF *) (((char *)buffer) + 3);
			}
	}

	// write bitmap file header
	file->WriteFile(0, &wBM, sizeof(wBM), &dwWritten, 0);
	file->WriteFile(0, &dwNumber, sizeof(dwNumber), &dwWritten, 0);		// file size
	file->WriteFile(0, &dwNumber, sizeof(dwNumber), &dwWritten, 0);		// 0
	dwNumber = 2 + 4 + 4 + 4 + sizeof(BITMAPINFOHEADER) + 1*sizeof(DWORD);
	file->WriteFile(0, &dwNumber, sizeof(dwNumber), &dwWritten, 0);		// offset to bits
	// write bitmap image header
	pbmih->bV4Size	=  sizeof(BITMAPINFOHEADER);
	file->WriteFile(0, pbmi, sizeof(BITMAPINFOHEADER)+1*sizeof(DWORD), &dwWritten, 0);
	// write bitmap image bits
	file->WriteFile(0, pBits, SCREENRESX*SCREENRESY*3, &dwWritten, 0);
	// go back and finish the file header
	file->SetFilePointer(0,2);
	dwNumber = file->GetFileSize();
	file->WriteFile(0, &dwNumber, sizeof(dwNumber), &dwWritten, 0);

	CQTRACE11("Screen shot dumped to file '%s'", buffer);

Done:
	free(pbmi);
	free(pBits);
}
#else
static void WriteScreenShot (void)
{
	char buffer[MAX_PATH+4];
	DAFILEDESC fdesc = buffer;
	COMPTR<IFileSystem> file;
	BITMAPINFO       *pbmi=0;               // BITMAPINFO structure
	BITMAPINFOHEADER *pbmih;              // Pointer to pbmi header
	const VFX_WINDOW * window = &SURFACE->GetWindow();
	DWORD dwWritten, dwNumber=0;
	WORD wBM = 'MB';   // "BM" header bytes

	create_tmp_name(buffer);

	fdesc.dwDesiredAccess |= GENERIC_WRITE;
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (DACOM->CreateInstance(&fdesc, file) != GR_OK)
	{
		CQERROR1("File create failed on %s", fdesc.lpFileName);
		goto Done;
	}

	pbmi = (BITMAPINFO *) calloc(sizeof (BITMAPINFOHEADER) + (sizeof (RGBQUAD) * 256), 1);
	pbmih = &(pbmi->bmiHeader);

	pbmih->biSize          =  sizeof(BITMAPINFOHEADER);
	pbmih->biWidth         =  window->x_max+1;
	pbmih->biHeight        =  window->y_max+1;
	pbmih->biPlanes        =  1;
	pbmih->biBitCount      =  (U16) (window->bytes_per_pixel * 8);
	pbmih->biSizeImage     =  pbmih->biWidth * pbmih->biHeight * window->bytes_per_pixel;
	pbmih->biXPelsPerMeter =  0;
	pbmih->biYPelsPerMeter =  0;
	pbmih->biClrUsed       =  0;
	pbmih->biClrImportant  =  0;
	pbmih->biCompression   =  BI_BITFIELDS;

	*(U32 *) (&(pbmi->bmiColors[0])) = window->R_mask;
    *(U32 *) (&(pbmi->bmiColors[1])) = window->G_mask;
    *(U32 *) (&(pbmi->bmiColors[2])) = window->B_mask;

	// write bitmap file header
	file->WriteFile(0, &wBM, sizeof(wBM), &dwWritten, 0);
	file->WriteFile(0, &dwNumber, sizeof(dwNumber), &dwWritten, 0);		// file size
	file->WriteFile(0, &dwNumber, sizeof(dwNumber), &dwWritten, 0);		// 0
	dwNumber = 2 + 4 + 4 + 4 + sizeof(BITMAPINFOHEADER) + 3*sizeof(DWORD);
	file->WriteFile(0, &dwNumber, sizeof(dwNumber), &dwWritten, 0);		// offset to bits
	// write bitmap image header
	file->WriteFile(0, pbmi, sizeof(BITMAPINFOHEADER) + 3*sizeof(DWORD), &dwWritten, 0);
	// write bitmap image bits
	// write it upside down for proper windows behavior
	{
		U8 * buffer;
		U32 rowSize = pbmih->biWidth * window->bytes_per_pixel;
		U32 numRows = pbmih->biHeight;

		buffer = ((U8 *)window->buffer) + (rowSize * (numRows-1));

		while (numRows)
		{
			file->WriteFile(0, buffer, rowSize, &dwWritten, 0);
			buffer -= rowSize;
			numRows--;
		}
	}
	// go back and finish the file header
	file->SetFilePointer(0,2);
	dwNumber = file->GetFileSize();
	file->WriteFile(0, &dwNumber, sizeof(dwNumber), &dwWritten, 0);

	CQTRACE11("Screen shot dumped to file '%s'", buffer);

Done:
	free(pbmi);
}
#endif
//--------------------------------------------------------------------------//
//-----------------------Cursor file structure------------------------------//
//--------------------------------------------------------------------------//

#pragma pack (push, 1)

struct group_cursor_entry
{
	U16 reserved;
	U16 resType;
	U16 numEntries;

	U16 width;
	U16 height;
	U16 planes;
	U16 bitcount;
	U32 resSize;	// in bytes
	U16 cursorID;
};

struct cursor_entry
{
	S16 hotX;
	S16 hotY;
	BITMAPINFOHEADER header;
	// transparency mask follows
};

#pragma pack ( pop )

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE Cursor : public Resource<Cursor,ICursorResource>, 
								  ConnectionPointContainer<Cursor>,
								  IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(Cursor)
	DACOM_INTERFACE_ENTRY(IResource)
	DACOM_INTERFACE_ENTRY(ICursorResource)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	U32 handle;		// connection handle to event system
	U32 cursorRes;
	bool bIsValid;		// false if cursor in busy state, etc.
	POINT Oldpt;			// cursor position last frame
	MSG fakeMsg;
	S32 busyCount;
	bool bInMenuLoop;
	bool bEnabled;
	HCURSOR hCurrentCursor;
	bool bAppFocus;
	bool bForceHWToggle;		// true if we temporarily turned on hardware cursor support
	bool bUndrawValid;


	PANE		mousePane;
	VFX_WINDOW  mouseWindow;
	U16			mouseWindowBuffer[32][32];
	S32			undrawX, undrawY;		// location to undraw mouse

	COMPTR<IDrawAgent> drawAgent;

	//------------------------
	// data used to draw a cursor
	//------------------------

	short        hotX, hotY;

	//------------------------
	// DirectInput stuff
	//------------------------
	COMPTR<IDirectInputDevice> pMouseDevice;
	DWORD dwLastLBClick;		// time stamp of last click
	SINGLE cursorSpeed, dxExtra, dyExtra;
	U32  dwMouseState;		// for directInput mode (use for wParam)
	bool bSwapMouseButtons;
	bool bExclusiveMode;
	struct MEVENT
	{
		DWORD	dwTimeStamp;
		S32		dx, dy, dz;
		BYTE	buttonsUp;
		BYTE	buttonsDown;
	};
	//------------------------
	
	Cursor (void);

	~Cursor (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ICursorResource methods */

	virtual BOOL32 SetCursor (U32 resID);

	virtual U32 GetCursor (void);

	virtual void SetBusy (BOOL bEnterBusy);
	
	virtual void DrawMouse (S32 xPosMouse, S32 yPosMouse);

	virtual BOOL32 IsValid (void) const;

	virtual BOOL32 SetDefaultCursor (void);

	virtual void SetCursorSpeed (S32 cursorSpeed);

	virtual void EnableCursor (bool bEnable);

	virtual const bool IsCursorEnabled (void) const
	{
		return bEnabled;
	}
	
	virtual HCURSOR GetWindowsCursor (void) 
	{
		return hCurrentCursor;
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* Cursor methods */

	BOOL32 localLoadCursor (cursor_entry *cursor_data);

	BOOL32 loadCursor (U32 resID);

	void unownDirectInput (void);

	void OnNoOwner (void)
	{
		SetDefaultCursor();
	}

	void init (void);

	void pollMouse (void);

	IDAComponent * GetBase (void)
	{
		return (ICursorResource *) this;
	}

	void sendEvent (U32 event)
	{
		MSG msg;
		msg.hwnd = hMainWindow;
		msg.message = event;
		msg.wParam = (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT)) ? MK_SHIFT : 0;
		msg.wParam |= (HOTKEY->GetVkeyState(VK_LCONTROL)||HOTKEY->GetVkeyState(VK_RCONTROL)) ? MK_CONTROL : 0;
		msg.wParam |= dwMouseState;
		msg.lParam = (Oldpt.y << 16) | (Oldpt.x & 0xFFFF);

		WM->SetCursorPos(Oldpt.x, Oldpt.y);

		if (EVENTSYS)
		{
			if (HOTKEY)
				HOTKEY->SystemMessage((S32)hMainWindow,(S32)event,(S32)0,(S32)msg.lParam);
			EVENTSYS->Send(event, &msg);
		}
	}

	// generate windows messages based on DirectInput
	void processEvent (const MEVENT & mevent)
	{
		if (mevent.dx || mevent.dy)
		{
			SINGLE fdx = (mevent.dx * cursorSpeed * SCREENRESX / SCREEN_WIDTH) + dxExtra;
			SINGLE fdy = (mevent.dy * cursorSpeed * SCREENRESY / SCREEN_HEIGHT) + dyExtra;
			S32 idx = fdx;
			S32 idy = fdy;
			Oldpt.x += idx;
			Oldpt.y += idy;
			dxExtra = fdx - idx;
			dyExtra = fdy - idy;
			if (Oldpt.x < 0)
				Oldpt.x = 0;
			else
			if (Oldpt.x >= S32(SCREENRESX))
				Oldpt.x = SCREENRESX-1;
			if (Oldpt.y < 0)
				Oldpt.y = 0;
			else
			if (Oldpt.y >= S32(SCREENRESY))
				Oldpt.y = SCREENRESY-1;

			sendEvent(WM_MOUSEMOVE);
		}

		if (mevent.buttonsUp & 1)
			sendEvent(WM_LBUTTONUP);
		if (mevent.buttonsUp & 2)
			sendEvent(WM_RBUTTONUP);
		if (mevent.buttonsUp & 4)
			sendEvent(WM_MBUTTONUP);

		if (mevent.buttonsDown & 1)
		{
			// test for double-click
			if (dwLastLBClick)
			{
				int diff = mevent.dwTimeStamp - dwLastLBClick;

				if (diff > 0 && diff < DBLCLICK_PERIOD)
				{
					dwLastLBClick = 0;
					sendEvent(WM_LBUTTONDBLCLK);
					CQTRACE10("Sending WM_LBUTTONDBLCLK.");
				}
				else
					dwLastLBClick = mevent.dwTimeStamp;
			}
			else
				dwLastLBClick = mevent.dwTimeStamp;

			sendEvent(WM_LBUTTONDOWN);
		}
		if (mevent.buttonsDown & 2)
			sendEvent(WM_RBUTTONDOWN);
		if (mevent.buttonsDown & 4)
			sendEvent(WM_MBUTTONDOWN);

		if (mevent.dx || mevent.dy)
			sendEvent(WM_MOUSEMOVE);

		if (mevent.dz)
		{
			MSG msg;
			msg.hwnd = hMainWindow;
			msg.message = WM_MOUSEWHEEL;
			msg.wParam = mevent.dz << 16;
			msg.lParam = 0;
			EVENTSYS->Send(WM_MOUSEWHEEL, &msg);
		}
	}

	U32 colorToMonochromeCursor (U32 colorCursorID);

	void update (void);

	void undrawMouse (void);

	void centerCursor (void);
};
//--------------------------------------------------------------------------//
//
Cursor::Cursor (void)
{
	bIsValid = 1;
	::GetCursorPos(&Oldpt);

	bEnabled = true;
	bAppFocus = true;

	//
	// convert -10 to +10 to .50 to 2.0
	//
	cursorSpeed = pow(2, (double(DEFAULTS->GetDefaults()->mouseSpeed) / (MAX_CURSOR_SPEED))) + 0.2;
}
//--------------------------------------------------------------------------//
//
Cursor::~Cursor (void)
{
	if (FULLSCREEN)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(handle);
	}

	if (pMouseDevice)
		pMouseDevice->Unacquire();
}
//--------------------------------------------------------------------------//
//
void Cursor::unownDirectInput (void)
{
	if (bExclusiveMode && pMouseDevice!=0)
	{
		if (pMouseDevice)
		{
			pMouseDevice->Unacquire();
			HRESULT hr = pMouseDevice->SetCooperativeLevel(hMainWindow, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
			CQASSERT(hr == DI_OK);
			pMouseDevice->Acquire();
		}
		bExclusiveMode=0;
		dwMouseState = 0;
		sendEvent(WM_LBUTTONUP);
		sendEvent(WM_RBUTTONUP);
		sendEvent(WM_MBUTTONUP);
		if (bEnabled)
			loadCursor(cursorRes);
	}
}
//--------------------------------------------------------------------------//
//
void Cursor::EnableCursor (bool _bEnable)
{
	bEnabled = _bEnable;

	if (_bEnable == false)
	{
		::SetCursor(NULL);
		hCurrentCursor = NULL;

//		SetClassLong(hMainWindow, GCL_HCURSOR, 0);
	}
	else
	{
		U32 tempID = cursorRes;
		cursorRes = 0;
		SetCursor(tempID);
	}
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT Cursor::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;
	static BOOL32 bActive;

	switch (message)
	{
	case CQE_GET_MULTIPLAYER_VER:
		if (param)
		{
			U32 * pResult = (U32 *) param;
			*pResult = GetMultiplayerVersion();
		}
		break;
	case WM_SETFOCUS:
		bAppFocus = true;
		break;
	case WM_KILLFOCUS:
		bAppFocus = false;
		unownDirectInput();
		break;

	case WM_ACTIVATEAPP:
		if (msg->wParam)
		{
			bAppFocus = true;
		}
		else
		if (bActive)
		{
			unownDirectInput();
			bIsValid = 0;
		}
		bActive = msg->wParam;
		break;

	case CQE_START3DMODE:
		SetDefaultCursor();
		bUndrawValid = false;
		if (bExclusiveMode)
			centerCursor();
		break;

	case WM_MOUSEMOVE:
		bIsValid = 1;
		break;

	case CQE_UPDATE:
		updateResource();
		update();
		break;

	case CQE_ENDFRAME:
		bUndrawValid = false;
		if (bExclusiveMode && bEnabled && busyCount==0)
			DrawMouse(Oldpt.x, Oldpt.y);
		break;

	case CQE_HOTKEY:
		switch ((U32)param)
		{
		case IDH_TOGGLE_SCREENMODE:
			Oldpt.x = SCREENRESX / 2;
			Oldpt.y = SCREENRESY / 2;
			sendEvent(WM_MOUSEMOVE);
			break;
		case IDH_SCREEN_SHOT:
			WriteScreenShot();
			// bScreenShotRequested=1;
			break;
		}
		break;

	case WM_EXITMENULOOP:
		bInMenuLoop = false;
		if (bExclusiveMode && pMouseDevice!=0)
			::GetCursorPos(&Oldpt);
		{
			// flush all mouse click messages from the queue
			MSG msg;

			while (PeekMessage(&msg, hMainWindow, WM_LBUTTONDOWN, WM_MOUSELAST, PM_REMOVE) != 0)
				;
		}
		break;

	case WM_ENTERMENULOOP:
		bInMenuLoop = true;
		// fall through intentional
	case WM_ENTERSIZEMOVE:
		unownDirectInput();
		bIsValid = 0;
		break;
	}


	return GR_OK;
}
//--------------------------------------------------------------------------//
//
BOOL32 Cursor::SetDefaultCursor (void)
{
	if (CQFLAGS.b3DEnabled)
		return SetCursor(IDC_CURSOR_DEFAULT);	
	else
		return SetCursor(IDC_CURSOR_ARROW);
}
//--------------------------------------------------------------------------//
//
void Cursor::SetCursorSpeed (S32 speed)
{
	DEFAULTS->GetDefaults()->mouseSpeed = speed;
	cursorSpeed = pow(2, (double(DEFAULTS->GetDefaults()->mouseSpeed) / (MAX_CURSOR_SPEED))) + 0.2;
}
//--------------------------------------------------------------------------//
//
BOOL32 Cursor::SetCursor (U32 resID)
{
	if (cursorRes == resID)
		return 1;

	if (loadCursor(resID))
	{
		cursorRes = resID;
		return 1;
	}

	return 0;
}
//--------------------------------------------------------------------------//
//
U32 Cursor::GetCursor (void)
{
	return cursorRes;
}
//--------------------------------------------------------------------------//
//
void Cursor::SetBusy (BOOL bEnterBusy)
{
	if (bEnterBusy)
	{
		if (busyCount++ == 0)
		{
			if (DEFAULTS->GetDefaults()->bHardwareCursor==0 && CQFLAGS.b3DEnabled==0)
			{
				if (bUndrawValid)
					undrawMouse();

				bForceHWToggle = true;
				DEFAULTS->GetDefaults()->bHardwareCursor = 1;
				bExclusiveMode = false;
				if (pMouseDevice)	// don't call update(), since it sends events
				{
					pMouseDevice->Unacquire();
					HRESULT hr = pMouseDevice->SetCooperativeLevel(hMainWindow, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
					CQASSERT(hr == DI_OK);
					pMouseDevice->Acquire();
				}
			}

			HCURSOR hWait = ::LoadCursor(0, IDC_WAIT);
			::SetCursor(hWait);
			hCurrentCursor = hWait;

			bIsValid = 0;
		}
	}
	else
	{
		if (busyCount && --busyCount == 0)
		{
			if (bForceHWToggle)
			{
				bForceHWToggle = false;
				DEFAULTS->GetDefaults()->bHardwareCursor = 0;
				::GetCursorPos(&Oldpt);
			}
		
			U32 origRes = cursorRes;
			cursorRes = 0;
			SetCursor(origRes);
		}
	}
}
//--------------------------------------------------------------------------//
//
BOOL32 Cursor::localLoadCursor (cursor_entry *cursor_data)
{
	BITMAPINFOHEADER * header = &cursor_data->header;

	hotX = cursor_data->hotX;
	hotY = cursor_data->hotY;

	// 
	// check validity of bitmap
	//

	if (header->biWidth != 32 || header->biHeight != 32*2)
		CQBOMB0("Invalid cursor resource");
	if (header->biBitCount != 8 && header->biBitCount != 4 && header->biBitCount != 1)
		CQBOMB0("Invalid cursor resource");

	hotX = IDEAL2REALX(hotX);
	hotY = IDEAL2REALY(hotY);

	COMPTR<IImageReader> reader;
	CreateBMPReader(reader);
	reader->LoadImage(header, 0, 0);

	CreateDrawAgent(reader, drawAgent);
	return 1;
}
//--------------------------------------------------------------------------//
//
U32 Cursor::colorToMonochromeCursor (U32 colorCursorID)
{
	switch (colorCursorID)
	{
	case IDC_CURSOR_DEFAULT:
		return IDC_CURSOR_DEFAULT_BW;

	case IDC_CURSOR_LEFT:
		return IDC_CURSOR_LEFT_BW;

	case IDC_CURSOR_RIGHT:
		return IDC_CURSOR_RIGHT_BW;

	case IDC_CURSOR_UP:
		return IDC_CURSOR_UP_BW;

	case IDC_CURSOR_DOWN:
		return IDC_CURSOR_DOWN_BW;

	case IDC_CURSOR_UPRIGHT:
		return IDC_CURSOR_UPRIGHT_BW;

	case IDC_CURSOR_UPLEFT:
		return IDC_CURSOR_UPLEFT_BW;

	case IDC_CURSOR_DOWNRIGHT:
		return IDC_CURSOR_DOWNRIGHT_BW;

	case IDC_CURSOR_DOWNLEFT:
		return IDC_CURSOR_DOWNLEFT_BW;

	case IDC_CURSOR_PLACEOBJECT:
		return IDC_CURSOR_PLACEOBJECT_BW;

	case IDC_CURSOR_SELLOBJECT:
		return IDC_CURSOR_SELLOBJECT_BW;

	case IDC_CURSOR_MOVEOBJECT:
		return IDC_CURSOR_MOVEOBJECT_BW;

	case IDC_CURSOR_BAN:
		return IDC_CURSOR_BAN_BW;

	case IDC_CURSOR_MICRO_MOVE:
		return IDC_CURSOR_MICRO_MOVE_BW;

	case IDC_CURSOR_MICRO_GOTO:
		return IDC_CURSOR_MICRO_GOTO_BW;

	case IDC_CURSOR_POSTOOL:
		return IDC_CURSOR_POSTOOL_BW;

	case IDC_CURSOR_ATTACK:
		return IDC_CURSOR_ATTACK_BW;

	case IDC_CURSOR_SPECIAL_ATTACK:
		return IDC_CURSOR_SPECIAL_ATTACK_BW;

	case IDC_CURSOR_HARVEST:
		return IDC_CURSOR_HARVEST_BW;

	case IDC_CURSOR_RAM:
		return IDC_CURSOR_RAM_BW;

	case IDC_CURSOR_REPAIR:
		return IDC_CURSOR_REPAIR_BW;

	case IDC_CURSOR_PATROL:
		return IDC_CURSOR_PATROL_BW;

	case IDC_CURSOR_JUMP:
		return IDC_CURSOR_JUMP_BW;

	case IDC_CURSOR_BUILD:
		return IDC_CURSOR_BUILD_BW;

	case IDC_CURSOR_DEPLOY:
		return IDC_CURSOR_DEPLOY_BW;

	case IDC_CURSOR_ARROW:
		return IDC_CURSOR_ARROW_BW;

	case IDC_CURSOR_DEFEND:
		return IDC_CURSOR_DEFEND_BW;

	case IDC_CURSOR_RESUPPLY:
		return IDC_CURSOR_RESUPPLY_BW;

	case IDC_CURSOR_RALLY:
		return IDC_CURSOR_RALLY_BW;

	case IDC_CURSOR_MOVE_ADMIRAL:
		return IDC_CURSOR_MOVE_ADMIRAL_BW;

	case IDC_CURSOR_CAPTURE:
		return IDC_CURSOR_CAPTURE_BW;

	case IDC_CURSOR_MICRO_RALLY:
		return IDC_CURSOR_MICRO_RALLY_BW;

	case IDC_CURSOR_MIMIC:
		return IDC_CURSOR_MIMIC_BW;

	case IDC_CURSOR_INTERMEDIATE:
		return IDC_CURSOR_INTERMEDIATE_BW;

	case IDC_CURSOR_FABREPAIR:
		return IDC_CURSOR_FABREPAIR_BW;

	case IDC_CURSOR_WORM_ATTACK:
		return IDC_CURSOR_WORM_ATTACK_BW;

	case IDC_CURSOR_MICRO_WORMHOLE:
		return IDC_CURSOR_MICRO_WORMHOLE_BW;
	}

	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL32 Cursor::loadCursor (U32 resID)
{
	HRSRC hRes;

	if (busyCount == 0)
	{
		if (bExclusiveMode==0)
		{
			if (CQFLAGS.bUseBWCursors)
			{
				U32 cursorID = colorToMonochromeCursor(resID);
				HCURSOR hArrow = ::LoadCursor(hResource, MAKEINTRESOURCE(cursorID));
				::SetCursor(hArrow);
				hCurrentCursor = hArrow;
			}
			else
			{
				HCURSOR hArrow = ::LoadCursor(hResource, MAKEINTRESOURCE(resID));
				::SetCursor(hArrow);
				hCurrentCursor = hArrow;
			}
		}
		else
		if (bInMenuLoop==false)
		{
			::SetCursor(0);
			hCurrentCursor = NULL;
		}
	}

	//
	// find address of group cursor resource structure
	//
	if ((hRes = FindResource(hResource, MAKEINTRESOURCE(resID), RT_GROUP_CURSOR)) != 0)
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(hResource, hRes)) != 0)
		{
			group_cursor_entry * pData;

			if ((pData = (group_cursor_entry *) LockResource(hGlobal)) != 0)
			{
				//
				// now load the cursor, using cursor number in group structure
				//
				if ((hRes = FindResource(hResource, MAKEINTRESOURCE(pData->cursorID), RT_CURSOR)) != 0)
				{
					if ((hGlobal = LoadResource(hResource, hRes)) != 0)
					{
						cursor_entry * pData;

						if ((pData = (cursor_entry *) LockResource(hGlobal)) != 0)
							return localLoadCursor(pData);
					}
				}
			}
		}
	}

	CQBOMB1("Failed to load cursor %d", resID);
	return 0;
}
//--------------------------------------------------------------------------//
//
void Cursor::DrawMouse (S32 xPosMouse, S32 yPosMouse)
{
	xPosMouse -= hotX;
	yPosMouse -= hotY;

	if (CQFLAGS.b3DEnabled == 0)
	{
		const PANE & pane = SURFACE->GetPane();

		VFX_pane_copy(const_cast<PANE *>(&pane), xPosMouse, yPosMouse, &mousePane, 0, 0, NO_COLOR);

		undrawX = xPosMouse;
		undrawY = yPosMouse;
		bUndrawValid = true;
	}

	drawAgent->Draw(0, xPosMouse, yPosMouse);
}
//--------------------------------------------------------------------------//
//
void Cursor::undrawMouse (void)
{
	if (SURFACE->LockFrontBuffer())
	{
		const PANE & pane = SURFACE->GetPane();

		VFX_pane_copy(&mousePane, 0, 0, const_cast<PANE *>(&pane), undrawX, undrawY, NO_COLOR);
		SURFACE->UnlockFrontBuffer();
	}
}
//--------------------------------------------------------------------------//
//
BOOL32 Cursor::IsValid (void) const
{
	return bIsValid;
}
//--------------------------------------------------------------------------//
//
void Cursor::update (void)
{
	//
	// make exclusive mode match screen mode
	//
	HRESULT hr;
	const USER_DEFAULTS * const defaults = DEFAULTS->GetDefaults();
	const bool bModeNeeded = bAppFocus && CQFLAGS.bFullScreen && (defaults->bHardwareCursor==0||CQFLAGS.bPrimaryDevice==0);

	if (bExclusiveMode==0 && bModeNeeded)
	{
		if (pMouseDevice)
		{
			pMouseDevice->Unacquire();
			hr = pMouseDevice->SetCooperativeLevel(hMainWindow, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
			CQASSERT(hr == DI_OK);
			pMouseDevice->Acquire();
			// flush the mouse buffer
			DWORD dwElements = INFINITE;
		    hr = pMouseDevice->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), NULL, &dwElements, 0);
		}
		WM->SetCursorPos(Oldpt.x, Oldpt.y);
		bExclusiveMode=1;
		dwMouseState = 0;
	}
	else
	if (bExclusiveMode && bModeNeeded==false)
	{
		unownDirectInput();
	}

	if (bExclusiveMode)
	{
		POINT pt = Oldpt;

		pollMouse();

		if (pt.x != Oldpt.x || pt.y != Oldpt.y)
		{
			fakeMsg.hwnd = hMainWindow;
			fakeMsg.message = WM_MOUSEMOVE;
			fakeMsg.wParam = 0;
			fakeMsg.lParam = (Oldpt.y << 16) | (Oldpt.x & 0xFFFF);
			EVENTSYS->Send(WM_MOUSEMOVE, &fakeMsg);
			WM->SetCursorPos(Oldpt.x, Oldpt.y);
		}
	}
	else
	{
		POINT pt;
		
		::GetCursorPos(&pt);
		if (CQFLAGS.bFullScreen==0)
			ScreenToClient(hMainWindow, &pt);
		if (pt.x != Oldpt.x || pt.y != Oldpt.y)
		{
			fakeMsg.hwnd = hMainWindow;
			fakeMsg.message = WM_MOUSEMOVE;
			fakeMsg.wParam = 0;
			fakeMsg.lParam = (pt.y << 16) | (pt.x & 0xFFFF);
			EVENTSYS->Send(WM_MOUSEMOVE, &fakeMsg);
			WM->SetCursorPos(pt.x, pt.y);
		}
		Oldpt = pt;
	}
}
//--------------------------------------------------------------------------//
//
void Cursor::centerCursor (void)
{
	POINT pt;
	pt.x = SCREENRESX / 2;
	pt.y = SCREENRESY / 2;
	EVENTSYS->Send(WM_MOUSEMOVE, &pt);
	Oldpt = pt;

	WM->SetCursorPos(pt.x, pt.y);
}
//--------------------------------------------------------------------------//
//
void Cursor::init (void)
{
	COMPTR<IDAConnectionPoint> connection;
    HRESULT hr;

	if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		connection->Advise(CURSOR, &handle);
		FULLSCREEN->SetCallbackPriority(this, EVENT_PRIORITY_CURSOR);
	}

	//
	// now initialize DirectInput stuff
	//
	COMPTR<IDirectInput> pDI;
	if( FAILED( DirectInputCreate( GetModuleHandle(0), DIRECTINPUT_VERSION, pDI, NULL ) ) ) 
	{
		if (CQFLAGS.bNoGDI)
			CQBOMB0("DirectInput failed.");
	}
	else
	if( FAILED( pDI->CreateDevice( GUID_SysMouse, pMouseDevice, NULL ) ) ) 
	{
		if (CQFLAGS.bNoGDI)
			CQBOMB0("DirectInput failed.");
	}

	if (pMouseDevice)
	{
		//
		// use the default DIMOUSESTATE structure
		//
		hr = pMouseDevice->SetDataFormat( &c_dfDIMouse );
		CQASSERT(hr == DI_OK);

		hr = pMouseDevice->SetCooperativeLevel( hMainWindow, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
		CQASSERT(hr == DI_OK);

		// setup the buffer size for the mouse data
		DIPROPDWORD dipdw;

		dipdw.diph.dwSize = sizeof(DIPROPDWORD);
		dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dipdw.diph.dwObj = 0;
		dipdw.diph.dwHow = DIPH_DEVICE;
		dipdw.dwData = 16;		//BUFFER_SIZE;

		hr = pMouseDevice->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph );
		CQASSERT(hr == DI_OK);
	}

    // not necessary, but nice for left handed users that have
    // their swapped mouse buttons
    bSwapMouseButtons = (GetSystemMetrics( SM_SWAPBUTTON ) != 0);

	//
	// initialize mousePane stuff
	//

	mousePane.window = &mouseWindow;
	mousePane.x0 = mousePane.y0 = 0;
	mousePane.x1 = mousePane.y1 = 31;

	mouseWindow.buffer = mouseWindowBuffer;
	mouseWindow.x_max = mouseWindow.y_max = 31;
	mouseWindow.pixel_pitch = 2;           // # of bytes between adjacent pixels
	mouseWindow.bytes_per_pixel = 2;       // # of bytes to write per pixel
}
//--------------------------------------------------------------------------//
//
void Cursor::pollMouse (void)
{
    DIDEVICEOBJECTDATA  od;
    DWORD               dwElements;
    HRESULT             hr;
	MEVENT				mevent;


    // Attempt to read one data element.  Continue as long as
    // device data is available.
	memset(&mevent, 0, sizeof(mevent));

	if (pMouseDevice)
    while (1) 
    {
        dwElements = 1;
        hr = pMouseDevice->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);

        if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) 
        {
			pMouseDevice->Acquire();
            break;
        }

        // Unable to read data or no data available
        if ( FAILED(hr) || dwElements == 0) 
        {
            break;
        }

		if (mevent.dwTimeStamp && od.dwTimeStamp != mevent.dwTimeStamp)
		{
			processEvent(mevent);
			memset(&mevent, 0, sizeof(mevent));
		}
		
		mevent.dwTimeStamp = od.dwTimeStamp;

        // look at the element to see what happened
        switch (od.dwOfs) 
        {     
            case DIMOFS_X:       // Mouse horizontal motion 
				mevent.dx = od.dwData;
                break;

            case DIMOFS_Y:       // Mouse vertical motion 
				mevent.dy = od.dwData;
                break;

			case DIMOFS_Z:		// mouse wheel
				mevent.dz = od.dwData;
                break;
        
            case DIMOFS_BUTTON0: // left button pressed or released 
            case DIMOFS_BUTTON1: // right button pressed or released 
                // is the left or a swapped left button down?
                if ( ( bSwapMouseButtons  && DIMOFS_BUTTON1 == od.dwOfs ) ||
                     ( !bSwapMouseButtons && DIMOFS_BUTTON0 == od.dwOfs ) )
                {
                    if ( od.dwData & 0x80 ) 
					{
						mevent.buttonsDown |= 1;
						dwMouseState |= MK_LBUTTON;
					}
					else
					{
						mevent.buttonsUp |= 1;
						dwMouseState &= ~MK_LBUTTON;
					}
                }
				else  // is the right or a swapped right button down?
                if ( ( bSwapMouseButtons  && DIMOFS_BUTTON0 == od.dwOfs ) ||
                     ( !bSwapMouseButtons && DIMOFS_BUTTON1 == od.dwOfs ) )
                {
                    if ( od.dwData & 0x80 ) 
					{
						mevent.buttonsDown |= 2;
						dwMouseState |= MK_RBUTTON;
					}
					else
					{
						mevent.buttonsUp |= 2;
						dwMouseState &= ~MK_RBUTTON;
					}
                }
                break;

			case DIMOFS_BUTTON2:  // middle / wheel button
                if ( od.dwData & 0x80 ) 
				{
					mevent.buttonsDown |= 4;
					dwMouseState |= MK_MBUTTON;
				}
				else
				{
					mevent.buttonsUp |= 4;
					dwMouseState &= ~MK_MBUTTON;
				}
				break;

			default:
				CQTRACE10("Huh1?");
				break;
        }  // end switch ()
    }  // end while (1)

	// send any unprocessed packets
	if (mevent.dwTimeStamp)
		processEvent(mevent);
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _cursor : GlobalComponent
{
	Cursor * cursor;

	virtual void Startup (void)
	{
		CURSOR = cursor = new DAComponent<Cursor>;
		AddToGlobalCleanupList((IDAComponent **) &CURSOR);
	}

	virtual void Initialize (void)
	{
		cursor->init();
	}
};

static _cursor startup;

//--------------------------------------------------------------------------//







//--------------------------------------------------------------------------//
//-----------------------------End Cursor.cpp-------------------------------//
//--------------------------------------------------------------------------//

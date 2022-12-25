//--------------------------------------------------------------------------//
//                                                                          //
//                               System.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

  $Author: Tmauer $
  
    $Header: /Conquest/App/Src/System.cpp 189   7/12/02 3:21p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Resource.h"
#include "Cursor.h"
//#include "IImageReader.h"
#include "Startup.h"
#include "CQTrace.h"
#include "VideoSurface.h"
#include "UserDefaults.h"
#include "DrawAgent.h"
#include "IHardpoint.h"
#include "ArchHolder.h"
#include "MyVertex.h"
#include "IGammaControl.h"
#include "TManager.h"
#include "HPEnum.h"
#include "BaseHotRect.h"
#include "CQBatch.h"
#include "DEffectOpts.h"

#include <heapobj.h>
#include <TComponent.h>
#include <WindowManager.h>
#include <EventSys.h>
#include <TSmartPointer.h>
#include <FileSys.h>
#include <ILight.h>
#include <RendPipeline.h>
#include <IRenderPrimitive.h>
#include <IProfileParser.h>
#include <ITexturelibrary.h>
#include <inv_sqrt.h>
#include <IVertexBufferManager.h>
#include <IDDBackDoor.h>
#include <RPUL\RPUL_Misc.h>

//#include <ddraw.h>
#include <stdlib.h>
#include <stdio.h>

#define WIDTH_REG_KEY  "CQWindowWidth"
#define HEIGHT_REG_KEY "CQWindowHeight"
#define DEPTH_REG_KEY  "CQPixelDepth"
#define RENDERDEV_REG_KEY "RenderDevice"


ISQRT SQRT;

PrimitiveBuilder2 PB;

struct DumpText;

static U8 gendataSum, g_bHasBuffers, gametypesSum, conquestSum, unusedSum2, missionSum;

extern bool bInScene;

//---------------------------------------------------------------------
//
static bool isInBounds (S32 x, S32 y)
{
	return ( x >= 0 &&
			 x < S32(SCREENRESX) &&
			 y >= 0 &&
			 y < S32(SCREENRESY));
}

//---------------------------------------------------------------------------
//
void BaseHotRect::sendPostedMessages (MESSAGE * pNode)
{
	MESSAGE node = *pNode;

	delete pNode;
	if (node.pNext)
		sendPostedMessages(node.pNext);

	Notify(node.message, node.param);
}
//--------------------------------------------------------------------------
//
BaseHotRect::~BaseHotRect (void)
{
	releaseResources();
	if (parentEventConnection)
		parentEventConnection->Unadvise(eventHandle);
	if (parentKeyboardFocusConnection)
		parentKeyboardFocusConnection->Unadvise(keyboardFocusHandle);

	if (pMessageList)
	{
		MESSAGE *pNode;

		while ((pNode = pMessageList) != 0)
		{
			pMessageList = pMessageList->pNext;
			if (pNode->message == CQE_DELETE_HOTRECT)
				removeClient((BaseHotRect *)pNode->param);
			delete pNode;
		}
	}
}
//--------------------------------------------------------------------------
//
void BaseHotRect::PostMessage (U32 message, void * param)
{
	MESSAGE *pNode;

	if ((pNode = new MESSAGE) != 0)
	{
		pNode->pNext = pMessageList;
		pMessageList = pNode;
		pNode->message = message;
		pNode->param = param;
	}
}
//--------------------------------------------------------------------------
//
PrimitiveBuilder2::~PrimitiveBuilder2( )
{
	if( render_pipeline ) {
		render_pipeline->Release();
		render_pipeline = NULL;
	}

	if( render_primitive ) {
		render_primitive->Release();
		render_primitive = NULL;
	}

	free (vertex_buffer_real);
	vertex_buffer_real = NULL;
	vertex_buffer = NULL;
}
// --------------------------------------------------------------------------
//
void PrimitiveBuilder2::VerifyBuffer( void )
{
	if( current_vertex >= num_vertex ) {
		FVFVERTEXTYPE *v;
		U8 *v_real;
		if( (v_real = (U8 *)malloc(sizeof(FVFVERTEXTYPE)*(num_vertex+block_size)+31)) != NULL ) {
			v = (FVFVERTEXTYPE *)((U32(v_real)+31) & ~31);
			if (vertex_buffer != NULL) {
				memcpy( v, vertex_buffer, sizeof(FVFVERTEXTYPE)*num_vertex );			
			}
			free(vertex_buffer_real);
			current_vertex_ptr = &v[current_vertex];
			vertex_buffer_real = v_real;
			vertex_buffer = v;
			num_vertex += block_size;
		}
	}
}
// --------------------------------------------------------------------------
//
void PrimitiveBuilder2::Begin( PBenum _type )
{
	Reset();
	switch( _type ) {
	case PB_POINTS:		
		type = D3DPT_POINTLIST;		
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_LINES:		
		type = D3DPT_LINELIST;		
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_LINE_LOOP:
		type = D3DPT_LINESTRIP;		
		insert_at_end = true;
		watch_quad = false;
		break;
	case PB_LINE_STRIP:
		type = D3DPT_LINESTRIP;		
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_TRIANGLES:
		type = D3DPT_TRIANGLELIST;
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_TRIANGLE_STRIP:
		type = D3DPT_TRIANGLESTRIP;
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_POLYGON:
	case PB_TRIANGLE_FAN:
		type = D3DPT_TRIANGLEFAN;
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_QUAD_STRIP:
	case PB_QUADS:
		type = D3DPT_TRIANGLELIST;
		insert_at_end = false;
		watch_quad = true;
		break;

	default:
		break;
	}
}
// --------------------------------------------------------------------------
//
void PrimitiveBuilder2::Begin( PBenum _type, U32 _vert_cnt)
{
	Reset();
	switch( _type ) {
	case PB_POINTS:		
		type = D3DPT_POINTLIST;		
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_LINES:		
		type = D3DPT_LINELIST;		
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_LINE_LOOP:
		type = D3DPT_LINESTRIP;		
		insert_at_end = true;
		watch_quad = false;
		break;
	case PB_LINE_STRIP:
		type = D3DPT_LINESTRIP;		
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_TRIANGLES:
		type = D3DPT_TRIANGLELIST;
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_TRIANGLE_STRIP:
		type = D3DPT_TRIANGLESTRIP;
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_POLYGON:
	case PB_TRIANGLE_FAN:
		type = D3DPT_TRIANGLEFAN;
		insert_at_end = false;
		watch_quad = false;
		break;
	case PB_QUAD_STRIP:
	case PB_QUADS:
		type = D3DPT_TRIANGLELIST;
		insert_at_end = false;
		watch_quad = true;
		break;

	default:
		break;
	}

	if( _vert_cnt >= num_vertex ) {
		//FVFVERTEXTYPE *v;
		U8 * v_real;
	//	if( (v= new FVFVERTEXTYPE[_vert_cnt]) != NULL ) {
		if( (v_real = (U8 *)malloc(sizeof(FVFVERTEXTYPE)*_vert_cnt+31)) != NULL ) {
			CQASSERT(current_vertex == 0);
			free(vertex_buffer_real);
			current_vertex_ptr = vertex_buffer = (FVFVERTEXTYPE *)((U32(v_real)+31) & ~31);
			vertex_buffer_real = v_real;
			num_vertex = _vert_cnt;
		}
	}
}

//---------------------------------------------------------------------
//-----------------Main window procedure-------------------------------
//---------------------------------------------------------------------
//
static LONG CALLBACK wndProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	MSG msg;
	static BOOL32 bInactive,bRecurse,bHasFocus,bLeadChar;
	static C8 charBuffer[3];

	if (CQFLAGS.bExceptionHappened)
		goto Done;

	switch (message)
	{
	case CQE_DATA_CHECKSUM:
		{
			switch (wParam)
			{
			case 0:
				gametypesSum = (lParam & 0xFF);
				break;
			case 1:
				gendataSum = (lParam & 0xFF);
				break;
			case 2:
				conquestSum = (lParam & 0xFF);
				break;
			case 3:
				missionSum = (lParam & 0xFF);
				break;
			} // end switch wParam
		}
		return 0;
	case WM_CHAR:
		{
			if (bLeadChar)
			{
				wchar_t result[2];
				charBuffer[1] = wParam;
				if (EVENTSYS && _localAnsiToWide(charBuffer, result, sizeof(result)))
					EVENTSYS->Send(CQE_UNICHAR, (void*)result[0]);
				bLeadChar = 0;
			}
			else
			if (IsDBCSLeadByte(U8(wParam)))
			{
				bLeadChar = 1;
				charBuffer[0] = wParam;
			}
			else
			{
				wchar_t result[2];
				charBuffer[0] = wParam;
				charBuffer[1] = 0;
				if (EVENTSYS && _localAnsiToWide(charBuffer, result, sizeof(result)))
					EVENTSYS->Send(CQE_UNICHAR, (void*)result[0]);
			}
		}
		break;

	case WM_ACTIVATEAPP:
		bInactive = (wParam==0);
		if (CQFLAGS.bNoGDI)
		{
			if (bInactive == 0)
			{
				// we are active
				SetWindowPos(hMainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
			}
			else
			{
				// we are inactive
				SetWindowPos(hMainWindow, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
				ShowWindow(hMainWindow, SW_MINIMIZE);
				InvalidateRect(0, 0, 1);
			}
		}
		if (!bInactive && CQFLAGS.b3DEnabled && g_bHasBuffers)
		{
			RestoreAllSurfaces();
		}
		break;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == 1)     // if in the client area
		{
			::SetCursor(CURSOR->GetWindowsCursor());
			return 1;
		}
		break;
		
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			//			HDC hdc = BeginPaint(hWindow, &ps);
			BeginPaint(hWindow, &ps);
			
			//	if (GL && hGLRC && (bInactive==0 || CQFLAGS.bFullScreen==0))
			//	SwapBuffers(hdc);
			
			if (CQFLAGS.bInProgressAnimActive==0 && g_bHasBuffers && (bInScene == 0 || CQFLAGS.b3DEnabled==0) && PIPE && (bInactive==0 || CQFLAGS.bFullScreen==0))
			{
				PIPE->swap_buffers();
			}
			
			EndPaint(hWindow, &ps);
			return 0;
		}
		break;

	case WM_CLOSE:
		if (EVENTSYS)
			EVENTSYS->Send(message, &msg);
		PostQuitMessage(0);	   // always quit with window valid
		WM->ServeMessageQueue();
		return 0;

	case WM_MOUSEACTIVATE:
		return MA_ACTIVATEANDEAT;	// activate window, but discard mouse message

	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
		wndProc(hWindow,
			    (message == WM_LBUTTONDBLCLK ? WM_LBUTTONDOWN :
					(message == WM_RBUTTONDBLCLK ? WM_RBUTTONDOWN : WM_MBUTTONDOWN)),
				wParam, lParam);		// don't eat the second click
		break;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		if (bRecurse == 0)
		{
			bRecurse++;
			wndProc(hWindow, WM_MOUSEMOVE, wParam, lParam);	// send a fake mouse move
			wndProc(hWindow, message, wParam, lParam);		// send the real message
			wndProc(hWindow, WM_MOUSEMOVE, wParam, lParam);	// send a fake mouse move
			bRecurse--;
			return 0;
		}
		break;

	case WM_KILLFOCUS:
		bHasFocus = 0;
		break;

	case WM_SETFOCUS:
		bHasFocus = 1;
		break;

	case WM_MOUSEMOVE:
		if (CQFLAGS.b3DEnabled && bHasFocus==0 && bInactive==0 && isInBounds(short(LOWORD(lParam)), short(HIWORD(lParam))))
			SetFocus(hWindow);
		break;

	case WM_SYSCHAR:
		if (GetMenu(hWindow) == 0)
			return 0;
		break;

	case WM_POWERBROADCAST:
		switch( wParam )
		{
		case PBT_APMQUERYSUSPEND:
			CQTRACE10("denying APM suspend");
			return BROADCAST_QUERY_DENY ;
		}
		break;

	case WM_SYSCOMMAND:
		switch (wParam & 0xFFF0) 
		{
		case SC_MONITORPOWER:
		case SC_SCREENSAVE:
			CQTRACE10("skipping screensaver activation");
			return FALSE;
		}
		break;

	}

	if (bInactive==0 || CQFLAGS.bFullScreen==0 || message == WM_ACTIVATEAPP)
	{
		msg.hwnd = hWindow;
		msg.message = message;
		msg.wParam = wParam;
		msg.lParam = lParam;

		if (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP)
		{
			if (GetMenu(hWindow)!=0)
			{
				// send to windows, and to local event queue
				if (EVENTSYS)
					EVENTSYS->Send(message, &msg);
			}
			else
			{
				// send to local queue, not to windows
				if (EVENTSYS)
					EVENTSYS->Send(message, &msg);
				return 0;
			}
		}
		else
		if (EVENTSYS)
			EVENTSYS->Send(message, &msg);
	}

Done:
	return DefWindowProc(hWindow, message, wParam, lParam);
}
//
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//
void SetupWindowCallback (void)
{
	WM->SetCallback(wndProc);
}
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//
#define NUM_CLEANUP_PTRS 26
struct CLEANUP_NODE
{
	struct CLEANUP_NODE * pNext;
	U32 numUsed;
	IDAComponent ** component[NUM_CLEANUP_PTRS];

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
};
static CLEANUP_NODE * cleanupList;
//------------------------------------------------------------------------
//
template <> void AddToGlobalCleanupList (IDAComponent ** component)
{
	//
	// find an empty place on the list
	//
	if (cleanupList == 0 || cleanupList->numUsed >= NUM_CLEANUP_PTRS)
	{
		CLEANUP_NODE * node = new CLEANUP_NODE;
		node->pNext = cleanupList;
		cleanupList = node;
	}

	cleanupList->component[cleanupList->numUsed++] = component;
}
//-----------------------------------------------------------------------------
// delete everyone in the cleanup list
//
void CleanupGlobals (void)
{
	S32 numUsed;
	CLEANUP_NODE * node = cleanupList;

	while (node)
	{
		numUsed = node->numUsed;
		while (numUsed-- > 0)
		{
			if (*node->component[numUsed])
				(*node->component[numUsed])->Release();
			*node->component[numUsed] = 0;
		}
		node = node->pNext;
		delete cleanupList;
		cleanupList = node;
	}
}
//------------------------------------------------------------------------------
//
BOOL32 __stdcall CheckDEBUGHotkeyPressed (U32 hotkeyID)
{
	U32 index=0;
	U32 message;
	U32 param;

	while (EVENTSYS->Peek(index++, &message, (void **) &param) == GR_OK)
		if (message == CQE_DEBUG_HOTKEY && param == hotkeyID)
			return 1;

	return 0;
}
//------------------------------------------------------------------------------
//
BOOL32 __stdcall CheckHotkeyPressed (U32 hotkeyID)
{
	U32 index=0;
	U32 message;
	U32 param;

	while (EVENTSYS->Peek(index++, &message, (void **) &param) == GR_OK)
		if (message == CQE_HOTKEY && param == hotkeyID)
			return 1;

	return 0;
}
//------------------------------------------------------------------------------
//
#define NUM_STARTUP_PTRS 26
struct STARTUP_NODE
{
	struct STARTUP_NODE * pNext;
	U32 numUsed;
	GlobalComponent * component[NUM_STARTUP_PTRS];

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

};
static STARTUP_NODE startupNode;
static STARTUP_NODE *startupList = &startupNode;
//------------------------------------------------------------------------
//
void __stdcall AddToGlobalStartupList (struct GlobalComponent & component)
{
	//
	// find an empty place on the list
	//
	if (startupList->numUsed >= NUM_STARTUP_PTRS)
	{
		STARTUP_NODE * node = new STARTUP_NODE;
		node->pNext = startupList;
		startupList = node;
	}

	startupList->component[startupList->numUsed++] = &component;
}
//------------------------------------------------------------------------
//
static void __stdcall initializeGlobalComponents (void)
{
	S32 numUsed;
	STARTUP_NODE * node = startupList;

	while (node)
	{
		numUsed = node->numUsed;
		while (numUsed-- > 0)
	 		node->component[numUsed]->Initialize();
		if ((node = node->pNext) != 0)	// don't delete the last node (it's static)
		{
			delete startupList;
			startupList = node;
		}
	}
}
//------------------------------------------------------------------------
//
void __stdcall CreateGlobalComponents (void)
{
	S32 numUsed;
	STARTUP_NODE * node = startupList;

	while (node)
	{
		numUsed = node->numUsed;
		while (numUsed-- > 0)
	 		node->component[numUsed]->Startup();
		node = node->pNext;
	}

	initializeGlobalComponents();
}
//------------------------------------------------------------------------
#define MIN_DSVOLUME (-3000)
//------------------------------------------------------------------------
// convertion between floating point and DSOUND volume
//
SINGLE __fastcall ConvertDSoundToFloat (S32 dsvolume)
{
	SINGLE result = (SINGLE(dsvolume) / -MIN_DSVOLUME) + 1.0F;

	if (result > 1.0)
		result = 1.0;
	else
	if (result < 0.0)
		result = 0.0;
	
	return result;
}
//---------------------------------------------------------------------
//
S32 __fastcall ConvertFloatToDSound (SINGLE volume)
{
	if (volume > 1.0)
		volume = 1.0;
	else
	if (volume < 0.0)
		volume = 0.0;
	if (volume == 0.0)
		return -10000;
	else
		return S32((1.0F - volume) * MIN_DSVOLUME);
}
//---------------------------------------------------------------------
//
S32 __stdcall AllocateCollisionRegion (void)
{
	static S32 region;
	return ++region;
}

//---------------------------------------------------------------------
// convert __DATE__ string into SYSTEMTIME structure
//
#define CONVERTIT(d0,d1,d2,d3) ((long(d3)<<24)+(long(d2)<<16)+(d1<<8)+d0)
SYSTEMTIME __stdcall GarblyWarbly (const char * _date)
{
	const U8 * date = (const U8 *) _date;
	DWORD month = CONVERTIT(date[3],date[2],date[1],date[0]);
	SYSTEMTIME result;

	switch (month)
	{
	case 'Jan ':
		month = 1;
		break;
	case 'Feb ':
		month = 2;
		break;
	case 'Mar ':
		month = 3;
		break;
	case 'Apr ':
		month = 4;
		break;
	case 'May ':
		month = 5;
		break;
	case 'Jun ':
		month = 6;
		break;
	case 'Jul ':
		month = 7;
		break;
	case 'Aug ':
		month = 8;
		break;
	case 'Sep ':
		month = 9;
		break;
	case 'Oct ':
		month = 10;
		break;
	case 'Nov ':
		month = 11;
		break;
	case 'Dec ':
		month = 12;
		break;
	default:
		month = 0;
		break;
	}

	memset(&result, 0, sizeof(result));

	result.wYear = atoi(_date+7);
	result.wMonth = month;
	if (date[4] == ' ')
		result.wDay = date[5] - '0';
	else
		result.wDay = ((date[4]-'0')*10) + date[5] - '0';

	return result;
}
//---------------------------------------------------------------------
// "Arial, 10, 400, 0"  -- facename, pixel size, weight, char_set(0=default)
// facename may include blank spaces, and be in MBCS.
//
bool  __stdcall CQCreateLogFont (U32 fontDescID, LOGFONT & logFont, bool bNoScaling)
{
	bool result=false;
	long point, weight, charSet=DEFAULT_CHARSET;

	const wchar_t * buffer = _localLoadStringW(fontDescID);
	char lpFontDesc[256];
	_localWideToAnsi(buffer,lpFontDesc,sizeof(lpFontDesc));
	const char * ptr;
	const char * altPtr;

	memset(&logFont, 0, sizeof(logFont));
	//
	// get the face name
	// 
	if((ptr = strchr(lpFontDesc, '#')) == 0)
	{
		if ((ptr = strchr(lpFontDesc, ',')) == 0)
			goto Done;
		memcpy(logFont.lfFaceName, lpFontDesc, (ptr-lpFontDesc));
		logFont.lfFaceName[(ptr-lpFontDesc)] = 0;	// NULL terminate
	}
	else
	{
		if(CQFLAGS.bAltFontName)
		{
			altPtr = ptr+1;
			if ((ptr = strchr(lpFontDesc, ',')) == 0)
				goto Done;
			memcpy(logFont.lfFaceName, altPtr, (ptr-altPtr));
			logFont.lfFaceName[(ptr-altPtr)] = 0;	// NULL terminate
		}
		else
		{
			altPtr = ptr;
			if ((ptr = strchr(lpFontDesc, ',')) == 0)
				goto Done;
			memcpy(logFont.lfFaceName, lpFontDesc, (altPtr-lpFontDesc));
			logFont.lfFaceName[(altPtr-lpFontDesc)] = 0;	// NULL terminate
		}
	}

	//
	// get the point size
	// 
	point = strtol(ptr+1, (char **) &ptr, 10);
	if (point == 0)
		goto Done;

	//
	// get the weight size
	// 
	weight = (*ptr) ? (strtol(ptr+1, (char **) &ptr, 10)) : 0;
	if (ptr == 0)
		goto OK;

	//
	// get the character set
	// 
	if (*ptr)
		charSet = strtol(ptr+1, (char **) &ptr, 10);

OK:
	if (bNoScaling)
		logFont.lfHeight =  -S32(point);					// specify pixels
	else
		logFont.lfHeight =  -S32(IDEAL2REALY(point));		// specify pixels
	logFont.lfCharSet = charSet;
	logFont.lfWeight = weight;
	logFont.lfPitchAndFamily = FF_DONTCARE;

	result = true;
Done:
	if (result==false)
		CQERROR1("Invalid font description, #id=%d", fontDescID);
	return result;
}
//--------------------------------------------------------------------------
//
HFONT __stdcall CQCreateFont (U32 fontDescID, bool bNoScaling)
{
	LOGFONT logFont;

	if (CQCreateLogFont(fontDescID, logFont, bNoScaling))
		return CreateFontIndirect(&logFont);

	return 0;
}
//--------------------------------------------------------------------------//
// of the format "#.#.#.#"
//
U32 __stdcall GetMultiplayerNumberFromString (const char * buffer)
{
	U32 result;

	result  = strtol(buffer, (char **) &buffer, 10) << 24;
	if (buffer && buffer[0])
		result |= (strtol(buffer+1, (char **) &buffer, 10) & 0xFF) << 16;
	if (buffer && buffer[0])
		result |= (strtol(buffer+1, (char **) &buffer, 10) & 0xFF) <<  8;
	if (buffer && buffer[0])
		result |= (strtol(buffer+1, (char **) &buffer, 10) & 0xFF) <<  0;

	return result;
}
//--------------------------------------------------------------------------//
// of the format "#.#.#.#"
//
U32 __stdcall GetMultiplayerStringFromNumber (U32 number, wchar_t * buffer, U32 bufferSize)
{
	U32 result;

	CQASSERT(bufferSize > 15);

	_ltow(number>>24, buffer, 10);
	result = wcslen(buffer);
	buffer[result++] = '.';
	buffer[result] = 0;

	_ltow((number>>16)&0xFF, buffer+result, 10);
	result = wcslen(buffer);
	buffer[result++] = '.';
	buffer[result] = 0;

	_ltow((number>>8)&0xFF, buffer+result, 10);
	result = wcslen(buffer);
	buffer[result++] = '.';
	buffer[result] = 0;

	_ltow(number&0xFF, buffer+result, 10);
	result = wcslen(buffer);

	return result;
}
//--------------------------------------------------------------------------//
// of the format "#.#.#.#"
//
U32 __stdcall GetMultiplayerStringFromNumber (U32 number, char * buffer, U32 bufferSize)
{
	U32 result;

	CQASSERT(bufferSize > 15);

	_ltoa(number>>24, buffer, 10);
	result = strlen(buffer);
	buffer[result++] = '.';
	buffer[result] = 0;

	_ltoa((number>>16)&0xFF, buffer+result, 10);
	result = strlen(buffer);
	buffer[result++] = '.';
	buffer[result] = 0;

	_ltoa((number>>8)&0xFF, buffer+result, 10);
	result = strlen(buffer);
	buffer[result++] = '.';
	buffer[result] = 0;

	_ltoa(number&0xFF, buffer+result, 10);
	result = strlen(buffer);

	return result;
}
//--------------------------------------------------------------------------//
//
U32 __stdcall GetMultiplayerVersion (char * buffer, U32 bufferSize)
{
	U32 result = GetMultiplayerVersion();
	result = GetMultiplayerStringFromNumber(result, buffer, bufferSize);
	return result;
}
//--------------------------------------------------------------------------//
//
U32 __stdcall GetMultiplayerVersion (void)
{
#ifdef _DEBUG
	U32 result = RGB(gendataSum, gametypesSum, 0) | (U32(0) << 24);
#else
	U32 result = RGB(gendataSum, gametypesSum, conquestSum) | (U32(missionSum) << 24);
#endif
	return result;
}
//--------------------------------------------------------------------------
//
U32 __stdcall GetBuildVersion (char * buffer, U32 bufferSize)
{
	char mname[MAX_PATH+4];
	char * tmp;
	DWORD dwHandle, dwSize;
	U32 result = 0;
	void * versionBuffer = 0;
	UINT numChars;
	void * pString=0;
	
	*buffer = 0;

	::GetModuleFileName(0, mname, sizeof(mname));
	if ((tmp = strrchr(mname, '\\')) == 0)
		goto Done;
	tmp++;

	if ((dwSize = GetFileVersionInfoSize(tmp, &dwHandle)) == 0)
		goto Done;

	versionBuffer = malloc(dwSize);

	if (GetFileVersionInfo(tmp, dwHandle, dwSize, versionBuffer) == 0)
		goto Done;

	if (VerQueryValue(versionBuffer, const_cast<char *>(_localLoadString(IDS_BUILD_VERSION)), &pString, &numChars) == 0)
		goto Done;

	if (numChars == 0 || pString==0 || bufferSize<=2)
		goto Done;

	if ((numChars+1) > bufferSize)
		numChars = bufferSize-1;

	memcpy(buffer, pString, numChars);
	buffer[numChars] = 0;
	result = numChars;
Done:
	::free(versionBuffer);
	return result;
}
//--------------------------------------------------------------------------
//
U32 __stdcall GetBuildVersion (void)
{
	char buffer[MAX_PATH+4];
	U32 result = 0;
	if (GetBuildVersion(buffer, sizeof(buffer)))
		result = GetMultiplayerNumberFromString(buffer);
	return result;
}
//--------------------------------------------------------------------------
//
U32 __stdcall GetProductVersion (char * buffer, U32 bufferSize)
{
	char mname[MAX_PATH+4];
	char * tmp;
	DWORD dwHandle, dwSize;
	U32 result = 0;
	void * versionBuffer = 0;
	UINT numChars;
	void * pString=0;
	
	*buffer = 0;

	::GetModuleFileName(0, mname, sizeof(mname));
	if ((tmp = strrchr(mname, '\\')) == 0)
		goto Done;
	tmp++;

	if ((dwSize = GetFileVersionInfoSize(tmp, &dwHandle)) == 0)
		goto Done;

	versionBuffer = malloc(dwSize);

	if (GetFileVersionInfo(tmp, dwHandle, dwSize, versionBuffer) == 0)
		goto Done;

	if (VerQueryValue(versionBuffer, const_cast<char *>(_localLoadString(IDS_PRODUCT_VERSION)), &pString, &numChars) == 0)
		goto Done;

	if (numChars == 0 || pString==0 || bufferSize<=2)
		goto Done;

	if ((numChars+1) > bufferSize)
		numChars = bufferSize-1;

	memcpy(buffer, pString, numChars);
	buffer[numChars] = 0;
	result = numChars;
Done:
	::free(versionBuffer);
	return result;
}
//--------------------------------------------------------------------------
//
U32 __stdcall GetProductVersion (void)
{
	char buffer[MAX_PATH+4];
	U32 result = 0;
	if (GetProductVersion(buffer, sizeof(buffer)))
		result = GetMultiplayerNumberFromString(buffer);
	return result;
}
//--------------------------------------------------------------------------
//
#define BEGIN_MAPPING(parent, name) \
	{								\
		HANDLE hFile, hMapping;		\
		DAFILEDESC fdesc = name;	\
		void *pImage;				\
		if ((hFile = parent->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)	\
			CQFILENOTFOUND(fdesc.lpFileName);	\
		hMapping = parent->CreateFileMapping(hFile);	\
		pImage = parent->MapViewOfFile(hMapping)

#define END_MAPPING(parent) \
		parent->UnmapViewOfFile(pImage); \
		parent->CloseHandle(hMapping); \
		parent->CloseHandle(hFile); }
//--------------------------------------------------------------------------
//

static void init_debug_font (void)
{
	VFX_FONT * lpFont = 0;
	U32 size = 0;

	//
	// get default font from resource file
	//

	HRSRC hRes;

	if ((hRes = FindResource(hResource, MAKEINTRESOURCE(IDR_DAFONT_DEFAULT), "DAFONT")) != 0)
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(hResource, hRes)) != 0)
		{
			lpFont = (VFX_FONT *) LockResource(hGlobal);
			size = SizeofResource(hResource, hRes);
		}
	}

	CQASSERT(lpFont);
	
	DEBUGCreateFontDrawAgent(lpFont, size, &DEBUGFONT, 0);
}
//---------------------------------------------------------------------
//
static void enableIMC (bool bEnable)
{
	if (bEnable)
	{
		if (hIMC)
			ImmAssociateContext(hMainWindow, hIMC);
	}
	else
	{
		HIMC _hIMC = ImmAssociateContext(hMainWindow, 0);

		if (hIMC==0)
			hIMC = _hIMC;
	}
}
//---------------------------------------------------------------------
//
static bool isTrue (const char * buffer)
{
	if (strncmp(buffer, "1", 1) == 0 || strnicmp(buffer, "on", 2)==0 || strnicmp(buffer, "true", 4)==0)
		return true;
	else
		return false;
}
//---------------------------------------------------------------------
//
static bool isFalse (const char * buffer)
{
	if (strncmp(buffer, "0", 1) == 0 || strnicmp(buffer, "off", 3)==0 || strnicmp(buffer, "false", 5)==0)
		return true;
	else
		return false;
}
//---------------------------------------------------------------------
//
void __stdcall ReadRenderOptions (void)
{
	COMPTR<IProfileParser> parser;
				
	CQRENDERFLAGS.bFSAA = 0;
	if (DACOM->QueryInterface("IProfileParser", parser) == GR_OK)
	{
		HANDLE hSection;
		
		if ((hSection = parser->CreateSection("RenderOptions")) != 0)
		{
			U32 len;
			char buffer[256];
			
			if ((len = parser->ReadKeyValue(hSection, "NoPerVertexAlpha", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQRENDERFLAGS.bNoPerVertexAlpha = true;
			}
			
			if ((len = parser->ReadKeyValue(hSection, "32BitTextures", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQRENDERFLAGS.b32BitTextures = true;
			}
		}
	}
}
//--------------------------------------------------------------------------
//
static S32 get_desktop_bpp (void)
{
	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetWindowDC(hwnd);
	S32 result;

	result = GetDeviceCaps(hdc, BITSPIXEL);

	ReleaseDC(hwnd, hdc);
	return result;
}
//---------------------------------------------------------------------
//
static void set_gamma (bool b3DEnabled)
{
	COMPTR<IGammaControl> gamma;
	GS->QueryInterface(IID_IGammaControl, gamma);

	if (b3DEnabled)
	{
		// we are in 3D mode, get the gamma out of user defaults
		S32 value = DEFAULTS->GetDefaults()->gammaCorrection;
		SINGLE gamma_value = SINGLE(value + 10)/SINGLE(10);

		gamma->set_gamma_function(IGC_ALL, gamma_value, 0, 1.0, 0);
	}
	else
	{
		// we are in desktop mode, switch gamma back to 1.0
		gamma->set_gamma_function(IGC_ALL, 1.0, 0, 1.0, 0);
	}
}
//---------------------------------------------------------------------
//
void __stdcall SetGamma (bool b3DEnabled)
{
	set_gamma(b3DEnabled);
}
//---------------------------------------------------------------------
//
static U32 iniResX, iniResY;
static U32 bitDepth;
static bool bForceMonochromeOff, bForceMonochromeOn;
//---------------------------------------------------------------------
//
void __stdcall ParseVideoINI (void)
{
	// read desired screen res from INI file

	iniResX = SCREEN_WIDTH;
	iniResY = SCREEN_HEIGHT;
	bitDepth = 16;

	COMPTR<IProfileParser> parser;

	if (DACOM->QueryInterface("IProfileParser", parser) == GR_OK)
	{
		HANDLE hSection;
		U32 len;
		char buffer[256];

		if ((hSection = parser->CreateSection("Video")) != 0)
		{
			// let's first try to get the resolution out of the registry key
			U32 regWidth, regHeight, regDepth;
			U32 res1, res2, res3;
			res1 = DEFAULTS->GetDataFromRegistry(WIDTH_REG_KEY, &regWidth, sizeof(U32));
			res2 = DEFAULTS->GetDataFromRegistry(HEIGHT_REG_KEY, &regHeight, sizeof(U32));
			res3 = DEFAULTS->GetDataFromRegistry(DEPTH_REG_KEY, &regDepth, sizeof(U32));

			if (res1 && res2 && res3 && regWidth && regHeight && regDepth)
			{
				CQTRACE13("Read video resolution from registry =%dx%dx%d from INI.", regWidth, regHeight, regDepth);
				iniResX = regWidth;
				iniResY = regHeight;
				bitDepth = regDepth;
			}

			if ((len = parser->ReadKeyValue(hSection, "TextureBias", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQFLAGS.bTextureBias = true;
			}

			if ((len = parser->ReadKeyValue(hSection, "HardwareGeometry", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQFLAGS.bHardwareGeometry = true;
			}

			if ((len = parser->ReadKeyValue(hSection, "ForcePipelineStall", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQRENDERFLAGS.bStallPipeline = true;
			}

			if ((len = parser->ReadKeyValue(hSection, "MonochromeCursor", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					bForceMonochromeOn = true;
				else
				if (isFalse(buffer))
					bForceMonochromeOff = true;
			}

#ifndef FINAL_RELEASE
			if ((len = parser->ReadKeyValue(hSection, "GDI", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer) == 0)
				{
					CQFLAGS.bNoGDI = true;
					CQFLAGS.bFullScreen = 1;
					CQFLAGS.bWindowModeAllowed = 0;
					DEFAULTS->GetDefaults()->bWindowMode=0;
					SetWindowPos(hMainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
				}
			}
			else
#endif  // end !FINAL_RELEASE
			{
				CQFLAGS.bNoGDI = true;
				CQFLAGS.bFullScreen = 1;
				CQFLAGS.bWindowModeAllowed = 0;
				DEFAULTS->GetDefaults()->bWindowMode=0;
				SetWindowPos(hMainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);
			}

			parser->CloseSection(hSection);
		}

		if ((hSection = parser->CreateSection("Features")) != 0)
		{
			if ((len = parser->ReadKeyValue(hSection, "LimitResolutions", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQFLAGS.bLimitResolutions = 1;
			}

			if ((len = parser->ReadKeyValue(hSection, "LimitWinningConditions", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQFLAGS.bLimitGameTypes = 1;
			}

			if ((len = parser->ReadKeyValue(hSection, "LimitDirectPlayConnections", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQFLAGS.bLimitDPConnections = 1;
			}

			if ((len = parser->ReadKeyValue(hSection, "LimitMapSettings", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQFLAGS.bLimitMapSettings = 1;
			}

			if ((len = parser->ReadKeyValue(hSection, "LimitMaxPlayers", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQFLAGS.bLimitMaxPlayers = 1;
			}
			if ((len = parser->ReadKeyValue(hSection, "ExtendedZoom", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQFLAGS.bExtCameraZoom = 1;
			}
			if ((len = parser->ReadKeyValue(hSection, "LanguageSelect", buffer, sizeof(buffer))) != 0)
			{
				if(strncmp(buffer, "ENGLISH", 7) == 0)
				{
					SetLangID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US));
				}
				else if(strncmp(buffer, "FRENCH", 6) == 0)
				{
					SetLangID(MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH));
				}
				else if(strncmp(buffer, "GERMAN", 6) == 0)
				{
					SetLangID(MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN));
				}
				else if(strncmp(buffer, "ITALIAN", 7) == 0)
				{
					SetLangID(MAKELANGID(LANG_ITALIAN,SUBLANG_ITALIAN));
				}
				else if(strncmp(buffer, "SPANISH", 7) == 0)
				{
					SetLangID(MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH));
				}
				else if(strncmp(buffer, "POLISH", 6) == 0)
				{
					SetLangID(MAKELANGID(LANG_POLISH,SUBLANG_DEFAULT));
				}
				else if(strncmp(buffer, "TAI", 3) == 0)
				{
					SetLangID(MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL));
				}
				else if(strncmp(buffer, "KOREAN", 6) == 0)
				{
					SetLangID(MAKELANGID(LANG_KOREAN,SUBLANG_KOREAN));
				}
			}
			if ((len = parser->ReadKeyValue(hSection, "ForceAltFont", buffer, sizeof(buffer))) != 0)
			{
				if (isTrue(buffer))
					CQFLAGS.bAltFontName = 1;
				else
					CQFLAGS.bAltFontName = 0;
			}
			else
				CQFLAGS.bAltFontName = 0;


			parser->CloseSection(hSection);
		}
	}
}
//---------------------------------------------------------------------
//
void __stdcall Set3DVarialbes (const U32 width, const U32 height, const U32 depth)
{
	iniResX = width;
	iniResY = height;
	bitDepth = depth;
}
//---------------------------------------------------------------------
//
static const char * getRenderSection (const char * regValue)
{
	static char section[64];
	char * result = 0;
	char buffer[64];
	HANDLE hSection;
	COMPTR<IProfileParser> parser;
	int i;

	if (regValue[0]==0)
		goto Done;

	DACOM->QueryInterface(IID_IProfileParser, parser);

	for (i = 0; i < 4; i++)
	{
		sprintf(section, "Rend%d", i);
		if ((hSection = parser->CreateSection(section)) != 0)
		{
			if (parser->ReadKeyValue(hSection, "DeviceId", buffer, sizeof(buffer)) != 0)
			{
				if (strcmp(buffer, regValue) == 0)		// guid matches regkey
				{
					parser->CloseSection(hSection);
					result = section;
					break;
				}
			}

			parser->CloseSection(hSection);
		}
	}

Done:
	return result;
}
static bool bStartupCalledOnce;
//---------------------------------------------------------------------
//
void __stdcall FlipToGDI (void)
{
	if (bStartupCalledOnce && PIPE!=0 && g_bHasBuffers && CQFLAGS.bNoGDI)
	{
		COMPTR<IDDBackDoor> pBackDoor;
		COMPTR<IUnknown> pDD1;
		COMPTR<IDirectDraw4> pDD4;
		HRESULT hr;

		hr = PIPE->QueryInterface(IID_IDDBackDoor, pBackDoor);
		if (hr != GR_OK)
			goto Done;
		hr = pBackDoor->get_dd_provider(DDBD_P_DIRECTDRAW, pDD1);
		if (hr != GR_OK)
			goto Done;
		hr = pDD1->QueryInterface(IID_IDirectDraw4, pDD4);    
		if (hr != DD_OK)
			goto Done;

		pDD4->FlipToGDISurface();
	}
Done:
	return;
}
//---------------------------------------------------------------------
//
bool verifyMultiTex (void)
{
	return true;//force it to be ok for now.

	//voodoo cards do not do any of the multitex effects we want to do
	RPDEVICEINFO rpdi;
	PIPE->get_device_info(&rpdi);
	if (rpdi.device_chipset_id >= RP_D_VOODOO_1 && rpdi.device_chipset_id <= RP_D_VOODOO_BANSHEE)
		return false;

	U32 testTex = TMANAGER->CreateTextureFromFile("cloak.tga",TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
	CQASSERT(testTex && (testTex != -1));
	
	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
	PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);

	PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	PIPE->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

	// filtering - bilinear with mips
	PIPE->set_sampler_state( 0, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR );
	PIPE->set_sampler_state( 0, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR );
//	PIPE->set_sampler_state( 0, D3DSAMP_MIPFILTER,		D3DTEXF_POINT );

	PIPE->set_texture_stage_texture( 0, testTex);
	
	// addressing - clamped
	PIPE->set_sampler_state( 0, D3DSAMP_ADDRESSU,  D3DTADDRESS_WRAP);
	PIPE->set_sampler_state( 0, D3DSAMP_ADDRESSV,  D3DTADDRESS_WRAP);
	
	PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
	
	PIPE->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_ADD );
	PIPE->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	PIPE->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
	PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
	PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );

	// filtering - bilinear with mips
	PIPE->set_sampler_state( 1, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR );
	PIPE->set_sampler_state( 1, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR );
//	PIPE->set_sampler_state( 1, D3DSAMP_MIPFILTER,		D3DTEXF_POINT );
	
	PIPE->set_texture_stage_texture(1,testTex);
	
	// addressing - clamped
	PIPE->set_sampler_state( 1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
	PIPE->set_sampler_state( 1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
	
	PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);

	TMANAGER->ReleaseTextureRef(testTex);
	
	return (PIPE->verify_state() == GR_OK);
}
//---------------------------------------------------------------------
// see if hardware can support colored cursors
//
static void testHardwareCursorSupport (void)
{
	return;
	COMPTR<IDDBackDoor> pBackDoor;
	COMPTR<IUnknown> pDD1;
	COMPTR<IDirectDraw4> pDD4;
	HRESULT hr;

	hr = PIPE->QueryInterface(IID_IDDBackDoor, pBackDoor);
	if (hr != GR_OK)
		goto Done;
	hr = pBackDoor->get_dd_provider(DDBD_P_DIRECTDRAW, pDD1);
	if (hr != GR_OK)
		goto Done;
	hr = pDD1->QueryInterface(IID_IDirectDraw4, pDD4);    
	if (hr != DD_OK)
		goto Done;

	DDCAPS caps;

	memset(&caps, 0, sizeof(caps));
	caps.dwSize = sizeof(caps);

	if (pDD4->GetCaps(&caps, NULL) != DD_OK)
		goto Done;
	if ((caps.dwCaps & DDCAPS_OVERLAY) == 0)
		goto Done;
	if (caps.dwCurrVisibleOverlays == caps.dwMaxVisibleOverlays)
		goto Done;		// out of overlays
	if (caps.dwVidMemTotal < 0xA00000)
		goto Done;

	CQFLAGS.bUseBWCursors = 0;		// support detected!

Done:
	return;
}
//---------------------------------------------------------------------
//
//this may be called when the percived resolution has changed for interface
static InterfaceRes lastRes = IR_NO_RESOLUTION;

InterfaceRes __stdcall GetCurrentInterfaceRes()
{
	return lastRes;
};

void __stdcall ChangeInterfaceRes(enum InterfaceRes res)
{
	if(lastRes == res)
		return;
	switch(lastRes)
	{
	case IR_IN_GAME_RESOLUTION:
		{
			EVENTSYS->Send(CQE_LEAVING_INGAMEMODE);
		}
		break;
	case IR_FRONT_END_RESOLUTION:
		{
			EVENTSYS->Send(CQE_LEAVING_FRONTENDMODE);
		}
		break;
	}
	lastRes = res;
	switch(res)
	{
	case IR_IN_GAME_RESOLUTION:
		{
			SCREEN_WIDTH = 640;
			SCREEN_HEIGHT = 480;
			EVENTSYS->Send(CQE_ENTERING_INGAMEMODE);
		}
		break;
	case IR_FRONT_END_RESOLUTION:
		{
			SCREEN_WIDTH = 800;
			SCREEN_HEIGHT = 600;
			EVENTSYS->Send(CQE_ENTERING_FRONTENDMODE);
		}
		break;
	}
	EVENTSYS->Send(CQE_INTERFACE_RES_CHANGE);
}
//---------------------------------------------------------------------
//
void __stdcall Start3DMode()
{

	//hardcoded temporary settings
	CQEFFECTS.bExpensiveTerrain = GlobalEffectsOptions::on;
	CQFLAGS.bFrameLockEnabled = 0;
	CQFLAGS.bFullScreen=0; //we are forcing to a window for now

	//enable 3D
	CQFLAGS.b3DEnabled = true;

	g_bHasBuffers = 0;  //at this point we can't make assumptions

	CQRENDERFLAGS.bSoftwareRenderer = 0;
	DEFAULTS->GetDefaults()->bHardwareRender = 1;

	CQFLAGS.bPrimaryDevice = 1;
	GENRESULT result = PIPE->startup(NULL);			// switch back to primary device
	ASSERT(result == GR_OK);

	VB_MANAGER->initialize(NULL);
	if (CQFLAGS.bFPUExceptions)
	{
		unsigned int cw = _controlfp(0, 0);
		// Set the exception masks OFF, which turns exceptions on.
		cw &= ~(EM_OVERFLOW|EM_ZERODIVIDE|EM_DENORMAL|EM_INVALID);
		_controlfp( cw, MCW_EM );
	}

	S32 nextMode = bitDepth;
	PIPE->set_pipeline_state(RP_BUFFERS_COLOR_BPP,nextMode);
	PIPE->set_pipeline_state(RP_BUFFERS_DEPTH_BPP,16);
	if (CQFLAGS.bNoGDI)
		PIPE->set_pipeline_state(RP_BUFFERS_HWFLIP,1);
	else
		PIPE->set_pipeline_state(RP_BUFFERS_HWFLIP,0);

	if (CQRENDERFLAGS.bStallPipeline)
		PIPE->set_pipeline_state(RP_BUFFERS_SWAP_STALL,1);


	BATCH->set_state(RPR_BATCH_POOLS, RPR_TRANSLUCENT_DEPTH_SORTED|RPR_TRANSLUCENT_UNSORTED |RPR_OPAQUE);
	BATCH->set_state(RPR_BATCH_TRANSLUCENT_MODE, RPR_TRANSLUCENT_UNSORTED );


	//for now hard coded to 800x600
//	if (bEnable)
//	{
		// get these from the INI file
//		SCREENRESX = iniResX;
//		SCREENRESY = iniResY;
//		CQFLAGS.bFrameLockEnabled = (SCREENRESX == SCREEN_WIDTH) && (SCREENRESY == SCREEN_HEIGHT) && (bitDepth == 16);
//	}
//	else
	{
		SCREENRESX = 1024;// SCREEN_WIDTH;
		SCREENRESY = 768;//SCREEN_HEIGHT;
		ChangeInterfaceRes(IR_FRONT_END_RESOLUTION);
	}



	if (CQFLAGS.bFullScreen==0)
	{
		if (DEFAULTS->GetDefaults()->iMainWidth)
		{
			WM->SetWindowPos(SCREENRESX, SCREENRESY, 0);
			SetWindowPos(hMainWindow, HWND_TOPMOST,
				DEFAULTS->GetDefaults()->iMainX,
				DEFAULTS->GetDefaults()->iMainY,
				DEFAULTS->GetDefaults()->iMainWidth,
				DEFAULTS->GetDefaults()->iMainHeight,
				SWP_NOZORDER|SWP_NOSIZE);
		}
		else
			WM->SetWindowPos(SCREENRESX, SCREENRESY, WMF_CENTER);
	}
	else
	{
		U32 flag = (CQFLAGS.bNoGDI) ? (WMF_ALWAYS_ON_TOP|WMF_FULL_SCREEN) : WMF_FULL_SCREEN;
		WM->SetWindowPos(SCREENRESX, SCREENRESY, flag);
	}

	ShowWindow(hMainWindow, SW_SHOWNORMAL);

	U32 hasFSAA=0;

	if (CQRENDERFLAGS.bFSAA)
	{
		PIPE->query_device_ability( RP_A_DEVICE_FULLSCENE_ANTIALIAS, &hasFSAA);
		if (hasFSAA)
		{
			PIPE->set_pipeline_state(RP_BUFFERS_ANTIALIAS,TRUE);
			CQTRACE10("Enabling FSAA");
		}
	}

	GENRESULT gr = PIPE->create_buffers(hMainWindow,SCREENRESX, SCREENRESY);
	//	U32 depth;
	//	PIPE->get_pipeline_state(RP_BUFFERS_DEPTH_BPP,&depth);

	g_bHasBuffers = 1;

	// force window to be on top
	if (CQFLAGS.bNoGDI)
		SetWindowPos(hMainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);

	CQASSERT(prevExceptionHandler==0);
	prevExceptionHandler = SetUnhandledExceptionFilter(cqExceptionHandler);

	CQBATCH->Startup();
	BATCH->set_sampler_state( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	BATCH->set_sampler_state( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	BATCH->set_sampler_state( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	BATCH->set_sampler_state( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

	bool bNoMips=0;
	U32 yes;
	PIPE->query_device_ability( RP_A_TEXTURE_LOD, &yes);
	if (yes)
	{
		TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD, TRUE );
		SINGLE dummy = 1.0f;
		TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD_SCALE, *((U32 *)&dummy));
		/*	PIPE->query_device_ability( RP_A_TEXTURE_TRILINEAR, &yes);
		if( yes ) {
		//enable trilinear mip.
		BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
		BATCH->set_sampler_state( 1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
		}
		else {*/
		//enable bilinear mip.
		BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		BATCH->set_sampler_state( 1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		//}
	}

	CQRENDERFLAGS.bNoPerVertexAlpha = true;
	PIPE->query_device_ability( RP_A_ALPHA_ITERATED, &yes);
	if (yes)
		CQRENDERFLAGS.bNoPerVertexAlpha = false;

	CQRENDERFLAGS.bMultiTexture = false;
	U32 tex;
	PIPE->query_device_ability( RP_A_TEXTURE_SIMULTANEOUS, &tex);
	if (tex > 1)
	{
		PIPE->query_device_ability( RP_A_TEXTURE_COORDINATES, &tex);
		if (tex > 1 && verifyMultiTex())
			CQRENDERFLAGS.bMultiTexture = true;
	}

	CQRENDERFLAGS.bHardwareGeometry = false;
	PIPE->query_device_ability( RP_A_DEVICE_GEOMETRY, &tex);
	if (tex && CQFLAGS.bHardwareGeometry)
		CQRENDERFLAGS.bHardwareGeometry = true;

	SINGLE bias=0.0f;
	TEXLOD = TL_LOW;
	PIPE->query_device_ability( RP_A_DEVICE_MEMORY, &tex);
	if (tex > (8*1024*1024))
	{
		bias = -1.0f;
		TEXLOD = TL_MEDIUM;
		if (tex > (16*1024*1024))
		{
			bias = 0.0f;
			TEXLOD = TL_HIGH;
		}
	}

	MEMORYSTATUS memoryStatus;
	GlobalMemoryStatus(&memoryStatus);
	//if (memoryStatus.dwTotalPhys < 72*1024*1024)
	//	TEXLOD = TEX_LOD(U32(TEXLOD)-1);

	switch (TEXLOD)
	{
	case TL_ULTRA_LOW:
		bias = -3.0f;
		break;
	case TL_LOW:
		bias = -2.0f;
		break;
	case TL_MEDIUM:
		bias = -1.0f;
		break;
	case TL_HIGH:
		bias = 0.0f;
		break;
	}

	if (bNoMips) //humoring the algorithm in TEXTURELIBRARY
	{
		if (CQRENDERFLAGS.bSoftwareRenderer)
			bias = 0.0f;
		else
			bias = 0.6f;
	}

	TEXLIB->set_library_state(ITL_STATE_TEXTURE_LOD_LOAD_BIAS,*((U32 *)&bias));

	if (CQRENDERFLAGS.bMultiTexture==false)
	{
		CQEFFECTS.bEmissiveTextures = GlobalEffectsOptions::OPTVAL(0);
	}
	if (CQEFFECTS.bExpensiveTerrain==0)
	{
		CQEFFECTS.bBackground = GlobalEffectsOptions::OPTVAL(0);
		CQEFFECTS.bHighBackground = GlobalEffectsOptions::OPTVAL(0);
	}

	//			if (hasFSAA)
	//				PIPE->set_render_state(D3DRS_ANTIALIAS,TRUE);

	U32 modes[4];

	BATCH->set_state(RPR_BATCH,0);
	BATCH->set_state(RPR_BATCH_TRANSLUCENT_POOL,262144);
	BATCH->set_state(RPR_BATCH_TRANSLUCENT_NONZ_POOL,1024*1024);
	PIPE->set_pipeline_state(RP_CLEAR_COLOR,0xff000000);  // a,r,g,b
	PIPE->set_render_state(D3DRS_ZENABLE,TRUE);

	static bool bFirstTime = true;
	if (bFirstTime && DEFAULTS->GetDefaults()->bWindowMode == false)
	{
		set_gamma(true);
		bFirstTime = false;
	}
	else
	{
		set_gamma(!DEFAULTS->GetDefaults()->bWindowMode);
	}

	BATCH->set_state(RPR_BATCH,1);

	init_debug_font();

	EVENTSYS->Send(CQE_START3DMODE, (void*)true);

	CQFLAGS.bNoToolbar = CQFLAGS.bMovieMode = 0;		// turn this back on by default
	// might cuase chaos to comment this out
	//		EVENTSYS->Send(CQE_SET_FOCUS, 0);

	CQFLAGS.bUseBWCursors = 1;
	if (bForceMonochromeOff)
		CQFLAGS.bUseBWCursors = 0;
	else
	{
		if (bForceMonochromeOn)
			CQFLAGS.bUseBWCursors = 1;
		else
		{
			CQFLAGS.bUseBWCursors = 0;
			//test called direct draw, commented out by tom
			//testHardwareCursorSupport();
		}
	}

	if (CQFLAGS.bNoGDI)
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
}
//---------------------------------------------------------------------
//
void __stdcall Shutdown3DMode()
{
	if (prevExceptionHandler)
	{
		SetUnhandledExceptionFilter(prevExceptionHandler);
		prevExceptionHandler = 0;
	}
	//clean up code
	enableIMC(false);

	if (DEBUGFONT)
	{
		((IDAComponent *)DEBUGFONT)->Release();
		DEBUGFONT = 0;
	}

	TEXLIB->free_library(true);			// release everything!

	if (TMANAGER)
		TMANAGER->Flush();

	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	EVENTSYS->Send(CQE_END3DMODE, (void*)false);
}
//---------------------------------------------------------------------
//
void __stdcall Enable3DMode (bool bEnable)
{
	CQASSERT(0 && "should not get here");
/*	CQEFFECTS.bExpensiveTerrain = GlobalEffectsOptions::on;
	//CQEFFECTS.bBackground = GlobalEffectsOptions::on;
	CQFLAGS.bWindowModeAllowed = 0;
	static bool firstRun = 1;
	if (!firstRun)
	{
		return;
	}
	firstRun = 0;
	bEnable = true;
	if (bStartupCalledOnce==0 || (CQFLAGS.b3DEnabled!=0) != bEnable)
	{
		if (prevExceptionHandler)
		{
			SetUnhandledExceptionFilter(prevExceptionHandler);
			prevExceptionHandler = 0;
		}

		enableIMC(false);

		if (DEBUGFONT)
		{
			((IDAComponent *)DEBUGFONT)->Release();
			DEBUGFONT = 0;
		}

		TEXLIB->free_library(true);			// release everything!

		CQRENDERFLAGS.bSoftwareRenderer = true;

		if ((CQFLAGS.b3DEnabled = bEnable) != 0)
		{
			g_bHasBuffers = 0;  //at this point we can't make assumptions

			CQRENDERFLAGS.bSoftwareRenderer = 0;
			DEFAULTS->GetDefaults()->bHardwareRender = 1;

			CQFLAGS.bPrimaryDevice = 1;
			GENRESULT result = PIPE->startup(NULL);			// switch back to primary device
			ASSERT(result == GR_OK);

			VB_MANAGER->initialize(NULL);
			if (CQFLAGS.bFPUExceptions)
			{
				unsigned int cw = _controlfp(0, 0);
				// Set the exception masks OFF, which turns exceptions on.
				cw &= ~(EM_OVERFLOW|EM_ZERODIVIDE|EM_DENORMAL|EM_INVALID);
				_controlfp( cw, MCW_EM );
			}

			bStartupCalledOnce=true;
			S32 nextMode = bEnable ? bitDepth:16;
			DEFAULTS->GetDefaults()->bWindowMode=1;
			PIPE->set_pipeline_state(RP_BUFFERS_COLOR_BPP,nextMode);
			PIPE->set_pipeline_state(RP_BUFFERS_DEPTH_BPP,16);
			if (CQFLAGS.bNoGDI)
				PIPE->set_pipeline_state(RP_BUFFERS_HWFLIP,1);
			else
				PIPE->set_pipeline_state(RP_BUFFERS_HWFLIP,0);

			if (CQRENDERFLAGS.bStallPipeline)
				PIPE->set_pipeline_state(RP_BUFFERS_SWAP_STALL,1);


			BATCH->set_state(RPR_BATCH_POOLS, RPR_TRANSLUCENT_DEPTH_SORTED|RPR_TRANSLUCENT_UNSORTED |RPR_OPAQUE);
			BATCH->set_state(RPR_BATCH_TRANSLUCENT_MODE, RPR_TRANSLUCENT_UNSORTED );


		//	if (bEnable)
		//	{
				// get these from the INI file
		//		SCREENRESX = iniResX;
		//		SCREENRESY = iniResY;
		//		CQFLAGS.bFrameLockEnabled = (SCREENRESX == SCREEN_WIDTH) && (SCREENRESY == SCREEN_HEIGHT) && (bitDepth == 16);
		//	}
		//	else
			{
				SCREENRESX = 800;// SCREEN_WIDTH;
				SCREENRESY = 600;//SCREEN_HEIGHT;
				CQFLAGS.bFrameLockEnabled = 1;
			}

			CQFLAGS.bFrameLockEnabled = 0;

			CQFLAGS.bFullScreen=0;

			if (CQFLAGS.bFullScreen==0)
			{
				if (DEFAULTS->GetDefaults()->iMainWidth)
				{
					WM->SetWindowPos(SCREENRESX, SCREENRESY, 0);
					SetWindowPos(hMainWindow, HWND_TOPMOST,
						DEFAULTS->GetDefaults()->iMainX,
						DEFAULTS->GetDefaults()->iMainY,
						DEFAULTS->GetDefaults()->iMainWidth,
						DEFAULTS->GetDefaults()->iMainHeight,
						SWP_NOZORDER|SWP_NOSIZE);
				}
				else
					WM->SetWindowPos(SCREENRESX, SCREENRESY, WMF_CENTER);
			}
			else
			{
				U32 flag = (CQFLAGS.bNoGDI) ? (WMF_ALWAYS_ON_TOP|WMF_FULL_SCREEN) : WMF_FULL_SCREEN;
				WM->SetWindowPos(SCREENRESX, SCREENRESY, flag);
			}

			ShowWindow(hMainWindow, SW_SHOWNORMAL);

			U32 hasFSAA=0;

			if (CQRENDERFLAGS.bFSAA)
			{
				PIPE->query_device_ability( RP_A_DEVICE_FULLSCENE_ANTIALIAS, &hasFSAA);
				if (hasFSAA)
				{
					PIPE->set_pipeline_state(RP_BUFFERS_ANTIALIAS,TRUE);
					CQTRACE10("Enabling FSAA");
				}
			}

			GENRESULT gr = PIPE->create_buffers(hMainWindow,SCREENRESX, SCREENRESY);
			//	U32 depth;
			//	PIPE->get_pipeline_state(RP_BUFFERS_DEPTH_BPP,&depth);

			g_bHasBuffers = 1;

			// force window to be on top
			if (CQFLAGS.bNoGDI)
				SetWindowPos(hMainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);

			CQASSERT(prevExceptionHandler==0);
			prevExceptionHandler = SetUnhandledExceptionFilter(cqExceptionHandler);

			CQBATCH->Startup();
			BATCH->set_sampler_state( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
			BATCH->set_sampler_state( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
			BATCH->set_sampler_state( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
			BATCH->set_sampler_state( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

			bool bNoMips=0;
			U32 yes;
			PIPE->query_device_ability( RP_A_TEXTURE_LOD, &yes);
			if (yes)
			{
				TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD, TRUE );
				SINGLE dummy = 1.0f;
				TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD_SCALE, *((U32 *)&dummy));
				/*	PIPE->query_device_ability( RP_A_TEXTURE_TRILINEAR, &yes);
				if( yes ) {
				//enable trilinear mip.
				BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
				BATCH->set_sampler_state( 1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
				}
				else {*/
/*				//enable bilinear mip.
				BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
				BATCH->set_sampler_state( 1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
				//}
			}

			CQRENDERFLAGS.bNoPerVertexAlpha = true;
			PIPE->query_device_ability( RP_A_ALPHA_ITERATED, &yes);
			if (yes)
				CQRENDERFLAGS.bNoPerVertexAlpha = false;

			CQRENDERFLAGS.bMultiTexture = false;
			U32 tex;
			PIPE->query_device_ability( RP_A_TEXTURE_SIMULTANEOUS, &tex);
			if (tex > 1)
			{
				PIPE->query_device_ability( RP_A_TEXTURE_COORDINATES, &tex);
				if (tex > 1 && verifyMultiTex())
					CQRENDERFLAGS.bMultiTexture = true;
			}

			CQRENDERFLAGS.bHardwareGeometry = false;
			PIPE->query_device_ability( RP_A_DEVICE_GEOMETRY, &tex);
			if (tex && CQFLAGS.bHardwareGeometry)
				CQRENDERFLAGS.bHardwareGeometry = true;

			SINGLE bias=0.0f;
			TEXLOD = TL_LOW;
			PIPE->query_device_ability( RP_A_DEVICE_MEMORY, &tex);
			if (tex > (8*1024*1024))
			{
				bias = -1.0f;
				TEXLOD = TL_MEDIUM;
				if (tex > (16*1024*1024))
				{
					bias = 0.0f;
					TEXLOD = TL_HIGH;
				}
			}

			MEMORYSTATUS memoryStatus;
			GlobalMemoryStatus(&memoryStatus);
			//if (memoryStatus.dwTotalPhys < 72*1024*1024)
			//	TEXLOD = TEX_LOD(U32(TEXLOD)-1);

			switch (TEXLOD)
			{
			case TL_ULTRA_LOW:
				bias = -3.0f;
				break;
			case TL_LOW:
				bias = -2.0f;
				break;
			case TL_MEDIUM:
				bias = -1.0f;
				break;
			case TL_HIGH:
				bias = 0.0f;
				break;
			}

			if (bNoMips) //humoring the algorithm in TEXTURELIBRARY
			{
				if (CQRENDERFLAGS.bSoftwareRenderer)
					bias = 0.0f;
				else
					bias = 0.6f;
			}

			TEXLIB->set_library_state(ITL_STATE_TEXTURE_LOD_LOAD_BIAS,*((U32 *)&bias));

			if (CQRENDERFLAGS.bMultiTexture==false)
			{
				CQEFFECTS.bEmissiveTextures = GlobalEffectsOptions::OPTVAL(0);
			}
			if (CQEFFECTS.bExpensiveTerrain==0)
			{
				CQEFFECTS.bBackground = GlobalEffectsOptions::OPTVAL(0);
				CQEFFECTS.bHighBackground = GlobalEffectsOptions::OPTVAL(0);
			}

			//			if (hasFSAA)
			//				PIPE->set_render_state(D3DRS_ANTIALIAS,TRUE);

			U32 modes[4];

			PIPE->query_device_ability( RP_A_BLEND_MATRIX, &modes[0]);
			if( !rp_a_is_blend_supported( D3DBLEND_ONE, D3DBLEND_ONE, modes ) )
			{
				CQEFFECTS.bExpensiveTerrain = GlobalEffectsOptions::OPTVAL(0);
			}

			//	if (CQFLAGS.bFrameLockEnabled)
			//	{
			//		if (SURFACE->Lock())	// test to see if locking is supported
			//			SURFACE->Unlock();
			//		else
			//			CQBOMB0("Frame Lock attempt failed.");
			//	}

			BATCH->set_state(RPR_BATCH,0);
			BATCH->set_state(RPR_BATCH_TRANSLUCENT_POOL,262144);
			BATCH->set_state(RPR_BATCH_TRANSLUCENT_NONZ_POOL,1024*1024);
			PIPE->set_pipeline_state(RP_CLEAR_COLOR,0xff000000);  // a,r,g,b
			PIPE->set_render_state(D3DRS_ZENABLE,TRUE);

			static bool bFirstTime = true;
			if (bFirstTime && DEFAULTS->GetDefaults()->bWindowMode == false)
			{
				set_gamma(true);
				bFirstTime = false;
			}
			else
			{
				set_gamma(!DEFAULTS->GetDefaults()->bWindowMode);
			}


			BATCH->set_state(RPR_BATCH,1);

			init_debug_font();

			EVENTSYS->Send(CQE_ENABLE3DMODE, (void*)bEnable);

			CQFLAGS.bNoToolbar = CQFLAGS.bMovieMode = 0;		// turn this back on by default
			// might cuase chaos to comment this out
			//		EVENTSYS->Send(CQE_SET_FOCUS, 0);

			if (bEnable==0 && TMANAGER)
				TMANAGER->Flush();

			CQFLAGS.bUseBWCursors = 1;
			if (bEnable)
			{
				if (bForceMonochromeOff)
					CQFLAGS.bUseBWCursors = 0;
				else
					if (bForceMonochromeOn)
						CQFLAGS.bUseBWCursors = 1;
					else
					{
						CQFLAGS.bUseBWCursors = 0;
						//test called direct draw, commented out by tom
						//testHardwareCursorSupport();
					}
			}

			if (bEnable && CQFLAGS.bNoGDI)
				SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
			else
				SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		}
	}*/
}
	//---------------------------------------------------------------------
	//
	void __stdcall SetupDiffuseBlend( U32 irp_texture_id, bool bClamp )
	{
		BATCH->set_render_state(D3DRS_COLORVERTEX, TRUE);
		BATCH->set_render_state(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
		BATCH->set_render_state(D3DRS_LIGHTING, FALSE);
		BATCH->set_texture_stage_texture( 0, irp_texture_id );
		//BATCH->set_texture_stage_texture( 0,0);
		// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
		BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		//BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		/*
		U8 r = irp_texture_id % 255;
		U8 g =(irp_texture_id/255)% 255;
		U8 b=(irp_texture_id/(255*255)) % 255;
		BATCH->set_render_state(D3DRS_TEXTUREFACTOR, D3DCOLOR_XRGB(r,g,b));
		
		BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_ADD);
		BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_TFACTOR );
		BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );

		BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

		*/
		
		BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

		// filtering - bilinear with mips
		BATCH->set_sampler_state( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		BATCH->set_sampler_state( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );


		if (bClamp)
		{
			// addressing - clamped
			BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
			BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
		}
		else
		{
			BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
			BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
		}
 

		for (int i = 1; i < 4; i++)
		{
			BATCH->set_texture_stage_state(i, D3DTSS_COLOROP, D3DTOP_DISABLE );
			BATCH->set_texture_stage_state(i, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
			BATCH->set_texture_stage_texture(i,0);
		}
	}
	//---------------------------------------------------------------------------
	//
	void __stdcall DisableTextures()
	{
		BATCH->set_texture_stage_texture(0,0);
		BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );

		BATCH->set_texture_stage_texture(1,0);
		BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}

//----------------------------------------------------------------------------------------
//
S32 __stdcall FindChild (const char * pathname, INSTANCE_INDEX parent)
{
	S32 index = -1;
	char buffer[MAX_PATH];
	const char *ptr=pathname, *ptr2;
	INSTANCE_INDEX child=-1;

	if (ptr[0] == '\\')
		ptr++;

	if ((ptr2 = strchr(ptr, '\\')) == 0)
	{
		strcpy(buffer, ptr);
	}
	else
	{
		memcpy(buffer, ptr, ptr2-ptr);
		buffer[ptr2-ptr] = 0;		// null terminating
	}

	while ((child = ENGINE->get_instance_child_next(parent,EN_DONT_RECURSE, child)) != -1)
	{
		const char *name = ENGINE->get_instance_part_name(child);
		if (strcmp(name, buffer) == 0)
		{
			if (ptr2)
			{
				// found the child, go deeper if needed
				parent = child;
				child = -1;
				ptr = ptr2+1;
				if ((ptr2 = strchr(ptr, '\\')) == 0)
				{
					strcpy(buffer, ptr);
				}
				else
				{
					memcpy(buffer, ptr, ptr2-ptr);
					buffer[ptr2-ptr] = 0;		// null terminating
				}
			}
			else
			{
				index = child;
				break;
			}
		}
	}

	return index;
}
//-----------------------------------------------------------------------------------------------------
//
static BOOL32 __fastcall _local_PatternMatch (const char *string, const char *pattern)
{
	BOOL32 result;
	
	if (pattern[0] == '*')
	{
		if ((result = _local_PatternMatch(string, ++pattern)) == 0)
		{
			while (string[0])
				if ((result = _local_PatternMatch(++string, pattern)) != 0)
					break;
		}
	}
	else
	{
		if (pattern[0] == 0 && string[0] == 0)
			result=1;
		else
		{
			if (pattern[0] == '?' && string[0] == 0)
				result = 0;
			else
				if (pattern[0] != '?' && toupper(pattern[0]) != toupper(string[0]))
					result = 0;
				else
					return _local_PatternMatch(string+1, pattern+1);
		}
	}
	
	return result;
}
//-----------------------------------------------------------------------------------------------------
//
struct LocalEnumData
{
	const char * szPattern;
	INSTANCE_INDEX index;
	ARCHETYPE_INDEX arch;
	IHPEnumerator * callback;
	bool bDone;
};
//-----------------------------------------------------------------------------------------------------
//
static void __cdecl hpcallback (const char* script_name, void* misc)
{
	LocalEnumData * ldata = (LocalEnumData *) misc;

	if (ldata->bDone==false && _local_PatternMatch(script_name, ldata->szPattern))
	{
		HPENUMINFO info;

		if (HARDPOINT->retrieve_hardpoint_info(ldata->arch, script_name, info.hardpointinfo))
		{
			info.index = ldata->index;
			info.name = script_name;

			if (ldata->callback->EnumerateHardpoint(info) == false)
				ldata->bDone = true;
		}
	}
}
//-----------------------------------------------------------------------------------------------------
//
static bool __stdcall enumerateHardpoints (INSTANCE_INDEX parent, const char * pattern, IHPEnumerator * callback)
{
	HARCH arch = parent;
	LocalEnumData ldata;

	ldata.szPattern = pattern;
	ldata.index = parent;
	ldata.bDone = false;
	ldata.arch = arch;
	ldata.callback = callback;

	HARDPOINT->enumerate_hardpoints(hpcallback, arch, &ldata);

	if (ldata.bDone)
		return false;

	INSTANCE_INDEX child = -1;
	while ((child = ENGINE->get_instance_child_next(parent,EN_DONT_RECURSE, child)) != -1)
	{
		if (enumerateHardpoints(child, pattern, callback) == 0)
			return false;
	}

	return true;
}
//-----------------------------------------------------------------------------------------------------
//
void __stdcall EnumerateHardpoints (INSTANCE_INDEX parent, const char * pathName, IHPEnumerator * callback)
{
	char buffer[MAX_PATH];
	char *ptr;

	strcpy(buffer, pathName);
	if ((ptr = strrchr(buffer, '\\')) != 0)
	{
		*ptr++ = 0;
		if (buffer[0])
			parent = FindChild(buffer, parent);
		if (parent == -1)
			CQERROR1("Could not find child object \"%s\"", buffer);
	}
	else
		ptr = buffer;

	if (parent != -1)
		enumerateHardpoints(parent, ptr, callback);
}
#if 1
//----------------------------------------------------------------------------------------
//
//  index will be returned as the index of the child object that owns the hardpoint
BOOL32 __stdcall FindHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent)
{
	index = -1;
	BOOL32 result=0;
	char buffer[MAX_PATH];
	char *ptr=buffer;
	BOOL32 bHadPath=0;

	strcpy(buffer, pathname);
	if ((ptr = strrchr(ptr, '\\')) == 0)
		goto Done;

	bHadPath = 1;

	*ptr++ = 0;
	if (buffer[0])
	{
		index = FindChild(buffer, parent);
	}
	else
		index = parent;

Done:	
	if (index != -1)
	{
		HARCH arch = index;

		if ((result = HARDPOINT->retrieve_hardpoint_info(arch, ptr, hardpointinfo)) == false)
		{
			if (!bHadPath)
				CQERROR1("Hardpoint %s not found off of specified object, did you forget a '\\'?",pathname);
			if (bHadPath)
				CQERROR1("Hardpoint not found %s",pathname);
			index = -1;		// invalidate result
		}
	}

	return result;
}

BOOL32 __stdcall FindHardpointSilent (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent)
{
	index = -1;
	BOOL32 result=0;
	char buffer[MAX_PATH];
	char *ptr=buffer;
	BOOL32 bHadPath=0;

	strcpy(buffer, pathname);
	if ((ptr = strrchr(ptr, '\\')) == 0)
		goto Done;

	bHadPath = 1;

	*ptr++ = 0;
	if (buffer[0])
	{
		index = FindChild(buffer, parent);
	}
	else
		index = parent;

Done:	
	if (index != -1)
	{
		HARCH arch = index;

		if ((result = HARDPOINT->retrieve_hardpoint_info(arch, ptr, hardpointinfo)) == false)
		{
			index = -1;		// invalidate result
		}
	}

	return result;
}

#else
struct _hpenum : IHPEnumerator
{
	INSTANCE_INDEX & index;
	HardpointInfo & hardpointinfo;

	_hpenum (INSTANCE_INDEX & _index, HardpointInfo & info) : index(_index), hardpointinfo(info)
	{
	}

	virtual bool EnumerateHardpoint (const HPENUMINFO & info)
	{
		hardpointinfo = info.hardpointinfo;
		index = info.index;
		return false;		// discontinue enum
	}
};
//
//  index will be returned as the index of the child object that owns the hardpoint
BOOL32 __stdcall FindHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent)
{
	_hpenum hpenum(index, hardpointinfo);

	index = -1;
	EnumerateHardpoints(parent, pathname, &hpenum);
	
	return (hpenum.index != -1);
}
#endif
//---------------------------------------------------------------------
//
GENRESULT __stdcall CreateProfileParser (const char * filename, IProfileParser2 **ppParser, IFileSystem * parent)
{
	GENRESULT result = GR_OK;
	if (parent==0)
		parent = PROFILEDIR;

	*ppParser = 0;

	BEGIN_MAPPING(parent, filename);
	
	if (pImage==0)
	{
		result = GR_GENERIC;
	}
	else
	{
		PROFPARSEDESC2 pdesc;
		COMPTR<IProfileParser2> parser;

		if ((result = DACOM->CreateInstance(&pdesc, parser)) == GR_OK)
		{
			if ((result = parser->Initialize2((char *)pImage, parent->GetFileSize(hFile))) == GR_OK)
			{
				*ppParser = parser;
				parser->AddRef();
			}
		}
	}

	END_MAPPING(parent);

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 __stdcall RecursiveDelete (IFileSystem * file)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	int result=0;

	if ((handle = file->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		return 1;	// directory is empty ?

	do
	{
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
		{
			result++;

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// traverse subdirectory
				if (file->SetCurrentDirectory(data.cFileName))
				{
					result = RecursiveDelete(file);
					file->SetCurrentDirectory("..");	// restore current directory
					file->RemoveDirectory(data.cFileName);
				}
			}
			else 
			{	
				file->DeleteFile(data.cFileName);
			}
		}

	} while (file->FindNextFile(handle, &data));

	file->FindClose(handle);

	return result;
}
//---------------------------------------------------------------------
//
U32 __stdcall CreateZBufferAnalog (const char * filename)
{
	U32 result = 0;
	COMPTR<IFileSystem> file;
	U32 dataSize;
	DAFILEDESC fdesc = filename;
	HANDLE hMapping=0;
	U8 * pImage = 0;

	if (DACOM->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;
	if ((hMapping = file->CreateFileMapping()) == 0)
		goto Done;
	if ((pImage = (U8 *) file->MapViewOfFile(hMapping)) == 0)
		goto Done;

	dataSize = file->GetFileSize();

	while (dataSize-- > 0)
	{
		result += pImage[dataSize];
		if ((dataSize & 127) == 0)
			result *= 1006;
	}

Done:
	if (pImage)
		file->UnmapViewOfFile(pImage);
	if (hMapping)
		file->CloseHandle(hMapping);
	return result;
}
//---------------------------------------------------------------------
//----------------------------End System.cpp---------------------------
//---------------------------------------------------------------------
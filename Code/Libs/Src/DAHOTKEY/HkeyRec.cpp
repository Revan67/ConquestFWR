//--------------------------------------------------------------------------//
//                                                                          //
//                               HKeyLib.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Logfile: /Libs/dev/Src/DAHotkey/HkeyRec.cpp $

    $Revision: 4 $

    $Date: 3/21/00 4:30p $

    $Author: Pbleisch $
*/			    
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "HKeyRec.h"
#include "HotKey.h"
#include "HeapObj.h"
#include "TComponent.h"

#include <stdlib.h>

#include "Resource.h"
//--------------------------------------------------------------------------//
//--------------------------DEFINES and Static data-------------------------//
//--------------------------------------------------------------------------//
HINSTANCE hInstance;		// initialized by DllMain

#define MAX_HOTKEYS  1024
typedef long (__stdcall *_WNDPROC) (HANDLE, unsigned int,unsigned int,long);
static struct HotkeyRecorder *pRecorderList=0;
static char interface_name[] = "IHotkeyRecorder";
void RegisterHotkeyEventServer (ICOManager *DACOM);
void RegisterTheEventSystem (ICOManager *DACOM);

static KB_MODIFIERS	kb;

//--------------------------------------------------------------------------//
//-----------------------------HotkeyRecorder class-------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE HotkeyRecorder : public IHotkeyRecorder
{
	struct HotkeyRecorder *	pNext;
	HWND					hwnd;
	_WNDPROC				pOldWndProc;
	
	char 					szButtonNames[32][16]; 			// JOY, POV, and MOUSE buttons 
	DAHOTKEY				LastHandleFreed;				// last handle freed
	DAHOTKEY				LastHandleAlloc;				// last handle allocated
	DAHOTKEY_STRUCT			KeyList[MAX_HOTKEYS];
	BYTE					KeyboardState[256];
	DWORD					dwGlobalMouseButtons;
	BYTE					modifier_table[256];			// which virtual keys are modifiers
 	DWORD					LPARAM_LIST[256];

	BEGIN_DACOM_MAP_INBOUND(HotkeyRecorder)
	DACOM_INTERFACE_ENTRY(IHotkeyRecorder)
	DACOM_INTERFACE_ENTRY2(IID_IHotkeyRecorder,IHotkeyRecorder)
	END_DACOM_MAP()

	HotkeyRecorder (void);
	~HotkeyRecorder (void);
	
	GENRESULT init (HKRECDESC *info);
	
	/* IHotkeyRecorder members */

    DEFMETHOD_(DAHOTKEY,CreateHotkey) (const U8 *buffer=0);

	DEFMETHOD_(BOOL32,DestroyHotkey) (DAHOTKEY hotkey);

	DEFMETHOD_(BOOL32,ReadEvents) (DAHOTKEY hotkey, U32 flags);

	DEFMETHOD_(U32,GetData) (DAHOTKEY hotkey, U8 *buffer, U32 maxBytes);

	DEFMETHOD_(BOOL32,IsEqual) (DAHOTKEY hotkey1, DAHOTKEY hotkey2);

	DEFMETHOD_(BOOL32,IsEmpty) (DAHOTKEY hotkey);

	DEFMETHOD_(BOOL32,SetEmpty) (DAHOTKEY hotkey);

	DEFMETHOD_(U32,ConvertToString) (DAHOTKEY hotkey, C8 * buffer, U32 maxBytes);;

	DEFMETHOD_(U32,GetModifiers) (DAHOTKEY hotkey);

	DEFMETHOD_(BOOL32,SetModifiers) (DAHOTKEY hotkey, U32 flags);

	DEFMETHOD_(U32,GetVirtualKey) (DAHOTKEY hotkey);

	DEFMETHOD_(BOOL32,SetVirtualKey) (DAHOTKEY hotkey, U32 key);

	DEFMETHOD_(U32,GetNumKeys) (DAHOTKEY hotkey);

	DEFMETHOD_(U32,GetTriggerType) (DAHOTKEY hotkey);

	DEFMETHOD_(BOOL32,SetTriggerType) (DAHOTKEY hotkey, U32 trigger);

	DEFMETHOD_(U32,GetSymbolNumber) (DAHOTKEY hotkey);

	DEFMETHOD_(BOOL32,SetSymbolNumber) (DAHOTKEY hotkey, U32 num);

	/* HotkeyRecorder members */

	static LONG CALLBACK WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	LONG WndCallback (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	BOOL32 isValid (DAHOTKEY hotkey);

	BOOL32 isMod (DWORD dwKey);

	DWORD GetVirtKeyFromJoyButtons(DWORD dwOldJoy, DWORD & dwNewJoy);

	DWORD GetVirtKeyFromMouseButtons (DWORD dwOldMouse, DWORD & dwNewMouse);

	DWORD GetVirtKeyFromModifierButtons(DWORD dwOldKey, DWORD & dwNewKey);

	static BOOL32 __fastcall isJoystickVKey (DAHOTKEY handle);
		   
	static BOOL32 __fastcall isPOVVKey (DAHOTKEY handle);

	static BOOL32 __fastcall isDIRVKey (DAHOTKEY handle);

	static BOOL32 __fastcall isMouseVKey (DAHOTKEY handle);

	static BOOL32 __fastcall isKeyboardVKey (DAHOTKEY handle);

	static DWORD __fastcall ConvertVKeyToMKEY (DAHOTKEY handle);
	
	static DWORD __fastcall ConvertVKeyToMJOY (DAHOTKEY handle);

	static DWORD __fastcall ConvertVKeyToMMOUSE (DAHOTKEY handle);

	DWORD UsePreviousValue(DAHOTKEY handle, DWORD & dwJoyDIR, DWORD & dwJoyPOV, DWORD & dwJoyButtons, DWORD & dwMouseButtons, DWORD & dwKeyModifiers);

	int PrintMKey (DAHOTKEY handle, char *lpBuffer, int iMaxBytes);
	
	int PrintVKey (DAHOTKEY handle, char *lpBuffer, int iMaxBytes);

	static void __fastcall AddToParamList (DWORD dwIndex, DWORD dwValue);
};

//--------------------------------------------------------------------------//
//
void HotkeyRecorder::AddToParamList (DWORD dwIndex, DWORD dwValue)
{
	HotkeyRecorder *pList = pRecorderList;

	while (pList)
	{
		pList->LPARAM_LIST[dwIndex] = dwValue;
		pList = pList->pNext;
	}
}
//--------------------------------------------------------------------------//
//
/*
char *GetStringResource(DWORD dwResource)
{
	HRSRC hrsrc;
	HGLOBAL hglobal;

    hrsrc = FindResource(hInstance, MAKEINTRESOURCE(dwResource), MAKEINTRESOURCE(RT_STRING));
	hglobal = LoadResource(hInstance, hrsrc);
	return (char *) LockResource(hglobal);
}
*/
//--------------------------------------------------------------------------//
//
HotkeyRecorder::HotkeyRecorder (void)
{
	memset(((DWORD *)this)+1, 0, sizeof(*this)-sizeof(DWORD));
	modifier_table[VK_LSHIFT] = 
	modifier_table[VK_RSHIFT] = 
	modifier_table[VK_LCONTROL] = 
	modifier_table[VK_RCONTROL] = 
	modifier_table[VK_LMENU] = 
	modifier_table[VK_RMENU] = 
	modifier_table[VK_SHIFT] = 
	modifier_table[VK_CONTROL] = 
	modifier_table[VK_MENU] = 1;

	// initialize the button names
	DWORD i; 

	for (i = IDS_JOY0; i <= IDS_MOUSE_MIDDLE; i++)
		LoadString(hInstance, i, szButtonNames[i-IDS_JOY0], sizeof(szButtonNames[i-IDS_JOY0]));

	if ((pNext = pRecorderList) != 0)
		pNext->AddRef();
	pRecorderList = this;
}
//--------------------------------------------------------------------------//
//
HotkeyRecorder::~HotkeyRecorder (void)
{
	SetWindowLong(hwnd, GWL_WNDPROC, (LONG) pOldWndProc);
	if ((pRecorderList = pNext) != 0)
		pNext->Release();
}
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//----------------------Helper functions for HotKeys------------------------//
//--------------------------------------------------------------------------//
// If handle is valid, convert it to int
//
inline BOOL32 HotkeyRecorder::isValid (DAHOTKEY hotkey)
{
	BOOL32 result;

	if (hotkey >= KeyList+MAX_HOTKEYS || hotkey < KeyList)
		result = 0;
	else
		result = hotkey->bValid;

	return result;
}
//--------------------------------------------------------------------------//
//
LONG CALLBACK HotkeyRecorder::WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	_WNDPROC lastProc=0;
	HotkeyRecorder *pNode = pRecorderList;

	if (((message == WM_KEYUP || message == WM_KEYDOWN) && 
		 (wParam == VK_LWIN || wParam == VK_RWIN)) ||
		(message == WM_CONTEXTMENU))
	{
		return 0;
	}

	while (pNode)
	{
		if (pNode->hwnd == hwnd)
		{
			pNode->WndCallback(hwnd, message, wParam, lParam);
			lastProc = pNode->pOldWndProc;
		}
		pNode = pNode->pNext;
	}

	// prevent menu from popping up
	if (message == WM_SYSKEYDOWN)
		wParam = 0;
	
#ifdef STRICT
	return CallWindowProc((WNDPROC)lastProc, hwnd, message, wParam, lParam);
#else
	return CallWindowProc((FARPROC)lastProc, hwnd, message, wParam, lParam);
#endif
}
//--------------------------------------------------------------------------//
//
LONG HotkeyRecorder::WndCallback (HWND _hwnd, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
		case WM_ACTIVATE:
		case WM_ACTIVATEAPP:
			kb.reset();
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			AddToParamList(wParam, lParam & ~(1 << 25));

			switch ((BYTE)wParam)
			{
			 	case VK_SHIFT:
					AddToParamList(VK_RSHIFT, LPARAM_LIST[VK_SHIFT]);
					AddToParamList(VK_LSHIFT, LPARAM_LIST[VK_SHIFT]);
					if ((lParam>>24) & 1)	// if right button
						kb.bRightShift = ((((unsigned)lParam)>>31)^1);
					else
						kb.bLeftShift = ((((unsigned)lParam)>>31)^1);
					break;

			 	case VK_CONTROL:
					AddToParamList(VK_RCONTROL, LPARAM_LIST[VK_CONTROL]);
					AddToParamList(VK_LCONTROL, LPARAM_LIST[VK_CONTROL]);
					if ((lParam>>24) & 1)	// if right button
						kb.bRightControl = ((((unsigned)lParam)>>31)^1);
					else
						kb.bLeftControl = ((((unsigned)lParam)>>31)^1);
					break;
					
			 	case VK_MENU:
					AddToParamList(VK_RMENU, LPARAM_LIST[VK_MENU]);
					AddToParamList(VK_LMENU, LPARAM_LIST[VK_MENU]);
					if ((lParam>>24) & 1)	// if right button
						kb.bRightAlt = ((((unsigned)lParam)>>31)^1);
					else
						kb.bLeftAlt = ((((unsigned)lParam)>>31)^1);
					break;
			}

			break;
		
/*
		case WM_SYSCHAR:
		case WM_CHAR:
			if (bLeftControl+bRightControl+bLeftAlt+bRightAlt > 0)
				return 1;
			break;
*/
		
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MOUSEMOVE:
			dwGlobalMouseButtons = 0;
			if (wParam & MK_LBUTTON)
				dwGlobalMouseButtons |= 1;
			if (wParam & MK_RBUTTON)
				dwGlobalMouseButtons |= 2;
			if (wParam & MK_MBUTTON)
				dwGlobalMouseButtons |= 4;
			break;

	}

	return 0;
}
//--------------------------------------------------------------------------//
//
inline BOOL32 HotkeyRecorder::isMod (DWORD dwKey)
{
	return modifier_table[(BYTE)dwKey];
}
//--------------------------------------------------------------------------//
//
inline static int __fastcall BitsSet (DWORD dwNumber)		// return number of bits set in number
{
	int result=0;
	DWORD i=1;

	do
	{
		if (dwNumber & i)
			result++;
		i <<= 1;
	} while (i);


	return result;
}
//--------------------------------------------------------------------------//
// Finds the new button pressed
// Returns virtual key or 0
//
DWORD HotkeyRecorder::GetVirtKeyFromJoyButtons(DWORD dwOldJoy, DWORD & dwNewJoy)
{
	DWORD i, result=HKR_VKEY_JOY0;
	
	dwOldJoy = (dwOldJoy & dwNewJoy) ^ dwNewJoy;

	// find lowest bit that is set
	i = 1;
	do 
	{
		if (i & dwOldJoy)
			break;
		result++;
		i <<= 1;
	} while (i);

	if (i==0)
		return 0;

	dwNewJoy &= ~i;

	return result;
}
//--------------------------------------------------------------------------//
//
DWORD HotkeyRecorder::GetVirtKeyFromMouseButtons (DWORD dwOldMouse, DWORD & dwNewMouse)
{
	DWORD i, result=HKR_VKEY_MOUSELEFT;
	
	dwOldMouse = (dwOldMouse & dwNewMouse) ^ dwNewMouse;

	// find lowest bit that is set
	i = 1;
	do 
	{
		if (i & dwOldMouse)
			break;
		result++;
		i <<= 1;
	} while (i);

	if (i==0)
		return 0;

	dwNewMouse &= ~i;

	return result;
}
//--------------------------------------------------------------------------//
//
DWORD HotkeyRecorder::GetVirtKeyFromModifierButtons(DWORD dwOldKey, DWORD & dwNewKey)
{
	DWORD i, result;
	
	dwOldKey = (dwOldKey & dwNewKey) ^ dwNewKey;

	// find lowest bit that is set
	i = 1;
	do 
	{
		if (i & dwOldKey)
			break;
		i <<= 1;
	} while (i);

	if (i==0)
		return 0;

	dwNewKey &= ~i;

	switch (i)
	{
		case HKRF_SHIFT:
			result = (kb.bLeftShift) ? VK_LSHIFT : VK_RSHIFT;
			break;
		case HKRF_CTRL:
			result = (kb.bLeftControl) ? VK_LCONTROL : VK_RCONTROL;
			break;
		case HKRF_ALT:
		default:
			result = (kb.bLeftAlt) ? VK_LMENU : VK_RMENU;
			break;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
inline BOOL32 HotkeyRecorder::isJoystickVKey (DAHOTKEY handle)
{
	return (handle->vKEY >= HKR_VKEY_JOY0 && handle->vKEY <= HKR_VKEY_JOYLAST);
}
//--------------------------------------------------------------------------//
//
inline BOOL32 HotkeyRecorder::isPOVVKey (DAHOTKEY handle)
{
	return (handle->vKEY >= HKR_VKEY_POVUP && handle->vKEY <= HKR_VKEY_POVLEFT);
}
//--------------------------------------------------------------------------//
//
inline BOOL32 HotkeyRecorder::isDIRVKey (DAHOTKEY handle)
{
	return (handle->vKEY >= HKR_VKEY_DIR_U && handle->vKEY <= HKR_VKEY_DIR_UL);
}
//--------------------------------------------------------------------------//
//
inline BOOL32 HotkeyRecorder::isMouseVKey (DAHOTKEY handle)
{
	switch (handle->vKEY)
	{
		case HKR_VKEY_MOUSELEFT:
		case HKR_VKEY_MOUSERIGHT:
		case HKR_VKEY_MOUSEMIDDLE:
	 		return 1;
	}

	return 0;
}
//--------------------------------------------------------------------------//
//
inline BOOL32 HotkeyRecorder::isKeyboardVKey (DAHOTKEY handle)
{
 	return (handle->vKEY > 7 && handle->vKEY < 256);
}
//--------------------------------------------------------------------------//
//
DWORD HotkeyRecorder::ConvertVKeyToMKEY (DAHOTKEY handle)
{
	DWORD result=handle->vKEY;

	switch (result)
	{
		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
			result = HKRF_SHIFT;
			break;

		case VK_CONTROL:
		case VK_LCONTROL:
		case VK_RCONTROL:
			result = HKRF_CTRL;
			break;

		case VK_MENU:
		case VK_LMENU:
		case VK_RMENU:
			result = HKRF_ALT;
			break;

		default:
			result = 0;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
DWORD HotkeyRecorder::ConvertVKeyToMJOY (DAHOTKEY handle)
{
	DWORD result=handle->vKEY;

	if (isJoystickVKey(handle))
		result = 1 << (result - HKR_VKEY_JOY0);
	else
		result = 0;

	return result;
}
//--------------------------------------------------------------------------//
//
DWORD HotkeyRecorder::ConvertVKeyToMMOUSE (DAHOTKEY handle)
{
	DWORD result=handle->vKEY;

	if (isMouseVKey(handle))
		result = 1 << (result - HKR_VKEY_MOUSELEFT);
	else
		result = 0;

	return result;
}
//--------------------------------------------------------------------------//
//
DWORD HotkeyRecorder::UsePreviousValue(DAHOTKEY handle, DWORD & dwJoyDIR, DWORD & dwJoyPOV, DWORD & dwJoyButtons, DWORD & dwMouseButtons, DWORD & dwKeyModifiers)
{
	DWORD result=0;

	if (isDIRVKey(handle))
	{
		if (dwJoyDIR)
		{
			result = dwJoyDIR;
			dwJoyDIR = 0;
		}
	}
	else
	if (isPOVVKey(handle))
	{
		if (dwJoyPOV)
		{
			result = dwJoyPOV;
			dwJoyPOV = 0;
		}
	}
	else
	if (isJoystickVKey(handle))
	{
		DWORD tmp;
	 	tmp = 1 << (handle->vKEY - HKR_VKEY_JOY0);
		if (tmp & dwJoyButtons)
		{
			// same button is still being pressed
			result = handle->vKEY;	
			dwJoyButtons &= ~tmp;
		}
	}
	else
	if (isMouseVKey(handle))
	{
		DWORD tmp;
	 	tmp = 1 << (handle->vKEY - HKR_VKEY_MOUSELEFT);
		if (tmp & dwMouseButtons)
		{
			// same button is still being pressed
			result = handle->vKEY;	
			dwMouseButtons &= ~tmp;
		}
	}
	else
	if (isKeyboardVKey(handle))
	{
		DWORD tmp=0;
		switch (handle->vKEY)
		{
			case VK_LSHIFT:
				tmp = (kb.bLeftShift)?HKRF_SHIFT:0;
				break;
			case VK_RSHIFT:
				tmp = (kb.bRightShift)?HKRF_SHIFT:0;
				break;
			case VK_LCONTROL:
				tmp = (kb.bLeftControl)?HKRF_CTRL:0;
				break;
			case VK_RCONTROL:
				tmp = (kb.bRightControl)?HKRF_CTRL:0;
				break;
			case VK_LMENU:
				tmp = (kb.bLeftAlt)?HKRF_ALT:0;
				break;
			case VK_RMENU:
				tmp = (kb.bRightAlt)?HKRF_ALT:0;
				break;
		}
		if (tmp)
		{
			result = handle->vKEY;	
			dwKeyModifiers &= ~tmp;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
// Return length, not counting null terminator.
//
int HotkeyRecorder::PrintVKey (DAHOTKEY handle, char *lpBuffer, int iMaxBytes)
{
	DWORD dwVirtKey;

	if (isKeyboardVKey(handle))
	{
	 	int result;
	 	result = GetKeyNameText(LPARAM_LIST[handle->vKEY], lpBuffer, iMaxBytes);
		return result;
	}

	if ((dwVirtKey = handle->vKEY) >= 256)
		if (dwVirtKey <= HKR_VKEY_MOUSELAST)
		{
		 	dwVirtKey -= 256;
			if ((iMaxBytes = __min(iMaxBytes, (int)strlen(szButtonNames[dwVirtKey]))) > 0)
			{
				memcpy(lpBuffer, szButtonNames[dwVirtKey], iMaxBytes);
				return iMaxBytes;
			}
		}

	return 0;
}
//--------------------------------------------------------------------------//
// Return length, not counting null terminator.
//
int HotkeyRecorder::PrintMKey (DAHOTKEY handle, char *lpBuffer, int iMaxBytes)
{
	DWORD dwMod = handle->getModifiers();
	int i,result=0;
	DWORD j=HKRF_MOD_JOY0,k=0;

	// print modifier keys
	if (dwMod & HKRF_SHIFT)
	{
		i = GetKeyNameText(LPARAM_LIST[VK_SHIFT]|(1<<25), lpBuffer, iMaxBytes);
		lpBuffer+=i;
		iMaxBytes-=i;
		result+=i;
	}
	if (dwMod & HKRF_CTRL)
	{
		if (result && iMaxBytes>0)
		{
		 	*lpBuffer++ = '+';
			iMaxBytes--;
			result++;
		}
			
		i = GetKeyNameText(LPARAM_LIST[VK_CONTROL]|(1<<25), lpBuffer, iMaxBytes);
		lpBuffer+=i;
		iMaxBytes-=i;
		result+=i;
	}
	if (dwMod & HKRF_ALT)
	{
		if (result && iMaxBytes>0)
		{
		 	*lpBuffer++ = '+';
			iMaxBytes--;
			result++;
		}
			
		i = GetKeyNameText(LPARAM_LIST[VK_MENU]|(1<<25), lpBuffer, iMaxBytes);
		lpBuffer+=i;
		iMaxBytes-=i;
		result+=i;
	}

	do 
	{
		if (dwMod & j)
		{
			if (result && iMaxBytes>0)
			{
			 	*lpBuffer++ = '+';
				iMaxBytes--;
				result++;
			}
		
			i = strlen(szButtonNames[k]);
			i = __min(i, iMaxBytes);
			memcpy(lpBuffer, szButtonNames[k], i);
			lpBuffer+=i;
			iMaxBytes-=i;
			result+=i;
		}
		j <<= 1;
		k++;
	} while (j);

	return result;
}
//--------------------------------------------------------------------------//
//------------------Actual HotkeyRecorder methods---------------------------//
//--------------------------------------------------------------------------//
//
DAHOTKEY HotkeyRecorder::CreateHotkey (const U8 *lpBuffer)
{
	DAHOTKEY result;

	if ((result = LastHandleFreed) == 0)
	{
		DAHOTKEY first;
		if (LastHandleAlloc==0 || LastHandleAlloc>=KeyList+MAX_HOTKEYS-1)
			first = result = KeyList;
		else
	 		first = result = LastHandleAlloc + 1;
		do
		{
			if (result->bValid==0)
			 	break;
			if (++result >= KeyList+MAX_HOTKEYS)
				result = KeyList;
		} while (result != first);
	}
	if (result->bValid==0)
	{
		if (lpBuffer)
		{
		 	*result = *((DAHOTKEY) lpBuffer);
			if (result->bValid==0)
				result->init();
			LastHandleAlloc = result;
		}
		else
		{
			LastHandleAlloc = result;
			result->init();
		}
	}
	else
		result = 0;

	if (result == LastHandleFreed)
		LastHandleFreed = 0;

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 HotkeyRecorder::DestroyHotkey (DAHOTKEY hotkey)
{
	if (isValid(hotkey))
	{
		LastHandleFreed = hotkey;
		memset(hotkey, 0, sizeof(*hotkey));
		return 1;
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL32 HotkeyRecorder::ReadEvents (DAHOTKEY hotkey, DWORD dwFlags)
{
	JOYINFOEX joyinfo;
	DWORD dwJoyButtons=0;
	DWORD dwJoyPOV=JOY_POVCENTERED;
	DWORD dwJoyDIR=0;
	DWORD dwMouseButtons=dwGlobalMouseButtons;
	DWORD dwVirtKey=0, dwKeyModifiers=0;

	if (isValid(hotkey) == 0)
		return 0;

	if (dwFlags & HKRF_KEYBOARD)
	{
		memset(KeyboardState, 0, sizeof(KeyboardState));
		kb.reset();
	}
	else
		GetKeyboardState(KeyboardState);

	if ((dwFlags & (HKRF_JOYBUTTONS|HKRF_JOYPOV|HKRF_JOYDIR)) == (HKRF_JOYBUTTONS|HKRF_JOYPOV|HKRF_JOYDIR))
	{
	 	// dont read joystick at all
	}
	else
	{
		joyinfo.dwSize = sizeof(joyinfo);
		joyinfo.dwFlags =  (dwFlags & HKRF_JOYBUTTONS)?0:JOY_RETURNBUTTONS;
		joyinfo.dwFlags |= (dwFlags & HKRF_JOYPOV)?0:JOY_RETURNPOV;
		joyinfo.dwFlags |= (dwFlags & HKRF_JOYDIR)?0:(JOY_RETURNX | JOY_RETURNY);
		joyinfo.dwPOV = dwJoyPOV;
	 	if (joyGetPosEx(JOYSTICKID1, &joyinfo) == JOYERR_NOERROR)
		{
			if ((dwFlags & HKRF_JOYBUTTONS)==0)
			{
				dwJoyButtons  = (joyinfo.dwButtons & JOY_BUTTON1) ? 1:0;
				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON2) ? 2:0;
				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON3) ? 4:0;
				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON4) ? 8:0;
				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON5) ? 16:0;
				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON6) ? 32:0;
				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON7) ? 64:0;
				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON8) ? 128:0;

				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON9) ? (1<<8):0;
				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON10) ?(1<<9):0;
				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON11) ?(1<<10):0;
				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON12) ?(1<<11):0;
				dwJoyButtons |= (joyinfo.dwButtons & JOY_BUTTON13) ?(1<<12):0;
			}

			if ((dwFlags & HKRF_JOYDIR)==0)
			{
				if (joyinfo.dwXpos > 0xC000)
				{
					if (joyinfo.dwYpos < 0x4000)
						dwJoyDIR = HKR_VKEY_DIR_UR;
					else
					if (joyinfo.dwYpos > 0xC000)
						dwJoyDIR = HKR_VKEY_DIR_DR;
					else
						dwJoyDIR = HKR_VKEY_DIR_R;
				}
				else
				if (joyinfo.dwXpos < 0x4000)
				{
					if (joyinfo.dwYpos < 0x4000)
						dwJoyDIR = HKR_VKEY_DIR_UL;
					else
					if (joyinfo.dwYpos > 0xC000)
						dwJoyDIR = HKR_VKEY_DIR_DL;
					else
						dwJoyDIR = HKR_VKEY_DIR_L;
				}
				else
				{
					if (joyinfo.dwYpos < 0x4000)
						dwJoyDIR = HKR_VKEY_DIR_U;
					else
					if (joyinfo.dwYpos > 0xC000)
						dwJoyDIR = HKR_VKEY_DIR_D;
				}
			}
			
			if ((dwFlags & HKRF_JOYPOV)==0)
				dwJoyPOV = joyinfo.dwPOV;
		}
	}

	// add virtual keys for mouse
	dwMouseButtons |= ((KeyboardState[VK_LBUTTON] & 0x80)!=0);
	dwMouseButtons |= ((KeyboardState[VK_RBUTTON] & 0x80)!=0)<<1;
	dwMouseButtons |= ((KeyboardState[VK_MBUTTON] & 0x80)!=0)<<2;
	memset(KeyboardState, 0, 8);	// remove first 8 elements of keyboard
	memset(KeyboardState+VK_PROCESSKEY, 0, 256-VK_PROCESSKEY);	// remove last elements of keyboard
	
	if (dwFlags & HKRF_MOUSEBUTTONS)
		dwMouseButtons = 0;	

	// see if there is a virtual key on keyboard
	if ((dwFlags & HKRF_VKEYS)==0)
	{
		int i;

		if (hotkey->vKEY < 256 && isMod(hotkey->vKEY)==0 && (KeyboardState[hotkey->vKEY] & 0x80))
			dwVirtKey = hotkey->vKEY;
		else
	 	for (i = 0; i < 256; i++)
			if (isMod(i)==0 && KeyboardState[i] & 0x80)
			{
				dwVirtKey = i;
				break;
			}
	}

	dwKeyModifiers  = (KeyboardState[VK_MENU]    & 0x80) ? HKRF_ALT : 0;
	dwKeyModifiers |= (KeyboardState[VK_CONTROL] & 0x80) ? HKRF_CTRL : 0;
	dwKeyModifiers |= (KeyboardState[VK_SHIFT]   & 0x80) ? HKRF_SHIFT : 0;

	switch (dwJoyPOV)
	{
		case JOY_POVFORWARD:
			dwJoyPOV = HKR_VKEY_POVUP;
			break;
		case JOY_POVRIGHT:
			dwJoyPOV = HKR_VKEY_POVRIGHT;
			break;
		case JOY_POVBACKWARD:
			dwJoyPOV = HKR_VKEY_POVDOWN;
			break;
		case JOY_POVLEFT:
			dwJoyPOV = HKR_VKEY_POVLEFT;
			break;
		default:
			dwJoyPOV = 0;
	}

	if (dwFlags & HKRF_MODIFIERS)		// single key event
	{
	 	if (dwVirtKey)
		{
			dwKeyModifiers = 
			dwJoyButtons = 
			dwJoyPOV = 
			dwJoyDIR = 
		 	dwMouseButtons = 0;
		}
		else // else pick a modifier for VirtKey, and clear the others
			dwFlags |= HKRF_FORCE_VKEY;
	}

	if ((dwVirtKey==0) && 
		((dwJoyPOV!=0) + 
		 (dwJoyDIR!=0) +
		 BitsSet(dwKeyModifiers) + 
		 BitsSet(dwJoyButtons) + 
		 BitsSet(dwMouseButtons)) == 1)
	{
		dwFlags |= HKRF_FORCE_VKEY;			// one key pressed. it must be a virtual key
	}

	if ((dwFlags & HKRF_FORCE_VKEY) && dwVirtKey==0)
	{
		// move one of the modifiers over to the VirtKey position
		// see if one of the groups has increased in number.
		if ((dwFlags & HKRF_MODIFIERS)==0 && dwJoyDIR && !hotkey->mDIR && !isDIRVKey(hotkey))
		{
			dwVirtKey = dwJoyDIR;
			dwJoyDIR = 0;
		}
		else
		if ((dwFlags & HKRF_MODIFIERS)==0 && dwJoyPOV && !hotkey->mHAT && !isPOVVKey(hotkey))
		{
			dwVirtKey = dwJoyPOV;
			dwJoyPOV = 0;
		}
		else
		if ((dwFlags & HKRF_MODIFIERS)==0 && BitsSet(dwJoyButtons) > BitsSet(hotkey->mJOY) + isJoystickVKey(hotkey))
		{
			dwVirtKey = GetVirtKeyFromJoyButtons(ConvertVKeyToMJOY(hotkey)|hotkey->mJOY, dwJoyButtons);
			// appropriate bit has been removed from dwJoyButtons
		}
		else
		if ((dwFlags & HKRF_MODIFIERS)==0 && BitsSet(dwMouseButtons) > BitsSet(hotkey->mMOUSE) + isMouseVKey(hotkey))
		{
			dwVirtKey = GetVirtKeyFromMouseButtons(ConvertVKeyToMMOUSE(hotkey)|hotkey->mMOUSE, dwMouseButtons);
			// appropriate bit has been removed from dwMouseButtons
		}
		else
		if ((dwFlags & HKRF_MODIFIERS)==0 && BitsSet(dwKeyModifiers) > BitsSet(hotkey->mKEY) + isKeyboardVKey(hotkey))
		{
			dwVirtKey = GetVirtKeyFromModifierButtons(ConvertVKeyToMKEY(hotkey)|hotkey->mKEY, dwKeyModifiers);
			// appropriate bit has been removed from dwKeyButtons
		}
		else // else use the previous value...
		if ((dwVirtKey = UsePreviousValue(hotkey, dwJoyDIR, dwJoyPOV, dwJoyButtons, dwMouseButtons, dwKeyModifiers)) != 0)
		{
			// nothing more to do
		}
		else // else pick one
		if (dwJoyDIR)
		{
			dwVirtKey = dwJoyDIR;
			dwJoyDIR = 0;
		}
		else
		if (dwJoyPOV)
		{
			dwVirtKey = dwJoyPOV;
			dwJoyPOV = 0;
		}
		else
		if ((dwVirtKey = GetVirtKeyFromJoyButtons(0,dwJoyButtons)) != 0)
		{
		 	// nothing more to do
		}
		else
		if ((dwVirtKey = GetVirtKeyFromMouseButtons(0,dwMouseButtons)) != 0)
		{
		 	// nothing more to do
		}
		else
		if ((dwVirtKey = GetVirtKeyFromModifierButtons(0,dwKeyModifiers)) != 0)
		{
		 	// nothing more to do
		}
		// else no key must be pressed !!
	}

	// find all of the modifiers
	// convert to mod
	switch (dwJoyPOV)
	{
		case HKR_VKEY_POVUP:
			dwJoyPOV = 1;
			break;
		case HKR_VKEY_POVDOWN:
			dwJoyPOV = 2;
			break;
		case HKR_VKEY_POVRIGHT:
			dwJoyPOV = 4;
			break;
		case HKR_VKEY_POVLEFT:
			dwJoyPOV = 8;
			break;
		default:
			dwJoyPOV = 0;
	}

	// convert to mod
	switch (dwJoyDIR)
	{
		case HKR_VKEY_DIR_U:
			dwJoyDIR = 1;
			break;
		case HKR_VKEY_DIR_UR:
			dwJoyDIR = 2;
			break;
		case HKR_VKEY_DIR_R:
			dwJoyDIR = 4;
			break;
		case HKR_VKEY_DIR_DR:
			dwJoyDIR = 8;
			break;
		case HKR_VKEY_DIR_D:
			dwJoyDIR = 16;
			break;
		case HKR_VKEY_DIR_DL:
			dwJoyDIR = 32;
			break;
		case HKR_VKEY_DIR_L:
			dwJoyDIR = 64;
			break;
		case HKR_VKEY_DIR_UL:
			dwJoyDIR = 128;
			break;
		default:
			dwJoyDIR = 0;
	}

	if (dwFlags & HKRF_MODIFIERS)		// single key event
	{
		dwKeyModifiers = 
		dwJoyButtons = 
		dwJoyPOV = 
		dwJoyDIR = 
	 	dwMouseButtons = 0;
	}

	hotkey->vKEY	= dwVirtKey;
	hotkey->mKEY   	= dwKeyModifiers;
	hotkey->mJOY	= dwJoyButtons;
	hotkey->mHAT	= dwJoyPOV;
	hotkey->mDIR	= dwJoyDIR;
	hotkey->mMOUSE	= dwMouseButtons;

	return 1;
}
//--------------------------------------------------------------------------//
//
U32 HotkeyRecorder::GetData (DAHOTKEY hotkey, U8 *buffer, U32 maxBytes)
{
	U32 result;

	if (buffer == 0)
		return sizeof(*hotkey);
	if (isValid(hotkey) == 0)
		return 0;

	result = __min(maxBytes, sizeof(*hotkey));

	memcpy(buffer, hotkey, result);

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 HotkeyRecorder::IsEqual (DAHOTKEY hotkey1, DAHOTKEY hotkey2)
{
	if (isValid(hotkey1) == 0)
		return 0;
	if (isValid(hotkey2) == 0)
		return 0;
	
	return (*hotkey1 == *hotkey2);
}
//--------------------------------------------------------------------------//
//
BOOL32 HotkeyRecorder::IsEmpty (DAHOTKEY hotkey)
{
	DAHOTKEY_STRUCT key;

	if (isValid(hotkey) == 0)
		return 1;

	key.init();

	return (*hotkey == key);
}
//--------------------------------------------------------------------------//
//
BOOL32 HotkeyRecorder::SetEmpty (DAHOTKEY hotkey)
{
	if (isValid(hotkey) == 0)
		return 0;

	return (hotkey->init());
}
//--------------------------------------------------------------------------//
//
U32 HotkeyRecorder::ConvertToString (DAHOTKEY hotkey, C8 * lpBuffer, U32 maxBytes)
{
	U32 result, result2;
	char buffer[260];
	
	if (isValid(hotkey) == 0)
		return 0;

	if ((result = PrintMKey(hotkey, buffer, 256)) > 0)
	{
		buffer[result++] = '+';
		buffer[result] = 0;
	}

	result2 = PrintVKey (hotkey, buffer+result, 256-result);
	if (result>0 && result2 == 0)
	 	result--;	// remove the '+'
	result += result2;

	if (result)
	{
		buffer[result++] = 0;
		if (lpBuffer)
		{
		 	if ((result = __min(result, maxBytes)) > 0)
			{
				memcpy(lpBuffer, buffer, result);
				lpBuffer[result-1] = 0;		// force null termination
			}
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
U32 HotkeyRecorder::GetModifiers (DAHOTKEY hotkey)
{
	if (isValid(hotkey) == 0)
		return 0;

	return hotkey->getModifiers();
}
//-------------------------------------------------------------------------------------
//
BOOL32 HotkeyRecorder::SetModifiers (DAHOTKEY hotkey, U32 dwFlags)
{
	if (isValid(hotkey) == 0)
		return 0;

	hotkey->setModifiers(dwFlags);	

	return 1;
}
//-------------------------------------------------------------------------------------
//
U32 HotkeyRecorder::GetVirtualKey (DAHOTKEY hotkey)
{
	if (isValid(hotkey) == 0)
		return 0;

	return hotkey->vKEY;
}
//-------------------------------------------------------------------------------------
//
BOOL32 HotkeyRecorder::SetVirtualKey (DAHOTKEY hotkey, DWORD dwKey)
{
	if (isValid(hotkey) == 0)
		return 0;
	
	hotkey->vKEY = dwKey;
	return 1;
}
//-------------------------------------------------------------------------------------
//
U32 HotkeyRecorder::GetNumKeys (DAHOTKEY hotkey)
{
	if (isValid(hotkey) == 0)
		return 0;

	return BitsSet(hotkey->getModifiers()) + (hotkey->vKEY != 0);
}
//-------------------------------------------------------------------------------------
//
U32 HotkeyRecorder::GetTriggerType (DAHOTKEY hotkey)
{
	if (isValid(hotkey) == 0)
		return 0;

	return hotkey->bType;
}
//-------------------------------------------------------------------------------------
//
BOOL32 HotkeyRecorder::SetTriggerType (DAHOTKEY hotkey, U32 trigger)
{
	if (isValid(hotkey) == 0)
		return 0;

	hotkey->bType = trigger;
	return 1;
}
//-------------------------------------------------------------------------------------
//
U32 HotkeyRecorder::GetSymbolNumber (DAHOTKEY hotkey)
{
	if (isValid(hotkey) == 0)
		return 0;

	return hotkey->symbol;
}
//-------------------------------------------------------------------------------------
//
BOOL32 HotkeyRecorder::SetSymbolNumber (DAHOTKEY hotkey, U32 dwNum)
{
	if (isValid(hotkey) == 0)
		return 0;

	hotkey->symbol = (BYTE) dwNum;
	return 1;
}

//--------------------------------------------------------------------------//
//
GENRESULT HotkeyRecorder::init (HKRECDESC * lpDesc)
{
	GENRESULT result = GR_OK;

	if (lpDesc->hWindow == 0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	if ((pOldWndProc = (_WNDPROC)GetWindowLong((HWND) lpDesc->hWindow, GWL_WNDPROC)) == 0)
	{
		if ((pRecorderList = pNext) != 0)
			pRecorderList->Release();
		result = GR_GENERIC;
		goto Done;
	}

	hwnd = (HWND) lpDesc->hWindow;
	SetWindowLong((HWND) lpDesc->hWindow, GWL_WNDPROC, (LONG) HotkeyRecorder::WndProc);

Done:
	return result;
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
				IComponentFactory * rec = new DAComponentFactory<DAComponent<HotkeyRecorder>,HKRECDESC> (interface_name);

				if (rec)
					DACOM->RegisterComponent(rec, interface_name);
				rec->Release();

				RegisterHotkeyEventServer(DACOM);
				RegisterTheEventSystem(DACOM);
			}
		}
	}

	return 1;
}
//--------------------------------------------------------------------------//
//----------------------------END HKeyRec.cpp-------------------------------//
//--------------------------------------------------------------------------//

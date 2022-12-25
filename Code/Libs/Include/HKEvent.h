#ifndef HKEVENT_H
#define HKEVENT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               HKEvent.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Logfile: /Conquest/Libs/Include/HKEvent.h $

    $Revision: 5 $

    $Date: 4/28/00 11:57p $

    $Author: Rmarr $
*/			    
//--------------------------------------------------------------------------//
/*

							Hotkey Event Library

	This code handles all of the processing of button press events, translating 
	them into game defined events.
	A HotKey specifies at most one virtual key, optionally combined with one or more 
	keyboard modifiers, joystick buttons, mouse buttons, or joystick POV. When the
	user simultaneously presses the buttons specified in a HotKey, its state changes to
	"PRESSED".
	Writing an application to "listen" for HotKey events offers a number of advantages
	over hardcoding actual key values.
		1. Documentation. All keys used in the game can be found by loading hotkey table in 
			the handy Hotkey editor.
		2. Coherent keyboard strategy - putting keys in convenient places, 
				avoids multiple use of the same key combinations
		3. Localization, allows for multiple keyboard layouts.
		4. User configurable.
		5. Simplier code. eg. No more "key_pressed()=='C' && alt_pressed() && !ctrl_pressed() &&
			!shift_pressed()" code.
	

	Example usage: The HotKey editor writes out the hotkey data in the following format:
		Hotkey Data
		{
			Version
			{
				long	0x101		// version 1.01
			}

			Binary data
			{
				long numberOfKeys
				long sizeOfKey
				keys[]
			}

			Event messages
			{
				long numberOfStrings
				long sizeOfString
				strings[]				// ASCIIZ strings
			}

			Keyboard text
			{
				long numberOfString
				long sizeOfString
				strings[]
			}
			
			Descriptions
			{
				long numberOfStrings
				long sizeOfString
				strings[]
			}
		}

	Only the "Version" and "Binary data" chunks are needed at runtime. The "Keyboard text",
	"Event messages", and "Descriptions" chunks are used by the hotkey editor program.

//-------------------------------------------------------------------------------------
//
void Enable (void);
	OUTPUT:
		Enables message processing. When enabled, the object processes keyboard, joystick, 
		and mouse events, and turns them into hotkey events. The object is enabled by default.

//-------------------------------------------------------------------------------------
//
void Disable (void);
	OUTPUT:
		Disables message processing. The object still keeps track of the state of
		all of the hotkeys, but does not send any hotkey event messages to the event
		manager.

//-------------------------------------------------------------------------------------
//
BOOL32 GetVkeyState (U32 key);
	INPUT:
		key: Specifies the Virtual key to inspect. (VK_* in WinUser.H)
	RETURNS:
		HKEF_RELEASED:  The user is not currently pressing the virtual key.
		HKEF_PRESSED:   The user is holding the virtual key down.
		
//-------------------------------------------------------------------------------------
//
BOOL32 GetHotkeyState (U32 hotkey);
	INPUT:
		hotkey: specifies the Hotkey to inspect
	RETURNS:
		HKEF_RELEASED:  The user is not currently pressing the key sequence.
		HKEF_PRESSED:   The user is holding the key sequence down.
		

//-------------------------------------------------------------------------------------
//
void __cdecl SystemMessage (S32 hwnd, S32 message, S32 wParam, S32 lParam, S32 unused1,	S32 unused2);
	INPUT:
		hwnd: Handle to the window that generated the message
		message: The windows message or user defined joystick message number.
		wParam, lParam: defined according the message.
			when it receives a "joyMessage" (See HKEVENTDESC, below), 'lParam' should point to a JOYINFOEX structure.
	RESULTS:
		Turns windows messages into hotkey events.

//-------------------------------------------------------------------------------------
//
U32 GetHotkeyText (U32 hotkey, C8 *outBuffer, U32 bufferSize);
	INPUT:
		hotkey: specifies the Hotkey to inspect
		outBuffer: User supplied memory that will be filled with the text version of the hotkey
		bufferSize: length of the buffer.
	RETURNS:
		Number of bytes copied into the user's buffer, not counting the terminating NULL character.
	OUTPUT:
		Fills the user's buffer with the ASCIIZ string representation of the key sequence. If multiple sequences
		are defined for the same hotkey, the method picks the simplest sequence.
		If 'outBuffer' is NULL, only the length of the string is returned.

  
//---------------------------------------------------------------------------------------
//	The  hotkey system also supports IEventSink and IEventCallback as incoming interfaces
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
//
void SendEvent (const C8 *message, S32 parm0, S32 parm1, S32 parm2, S32 parm3)
	INPUT:
		message: Should be ASCIIZ string "Windows message"
		parm0: handle (HWND) of the window originating the message
		parm1: window message (See WinUser.h)
		parm2: wParam value defined for windows message
		parm3: lParam value defined for windows message
	RESULTS:
		Turns windows messages into hotkey events.

//---------------------------------------------------------------------------------------
//
GENRESULT Notify (U32 message, void *param)
	INPUT:
		message: Can either be the joyMessage (See HKEVENTDESC, below), or a Windows message.
		param: If (message == joyMessage), 'param' should point to a JOYINFOEX structure.
			   If (message < WM_USER) 'param' points to a MSG structure.
	RESULTS:
		Turns messages into hotkey events.

*/
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------

#ifndef DACOM_H
#include <DACOM.h>
#endif


#define IID_IEventMessageFilter MAKE_IID("IEventMessageFilter",1)

struct DACOM_NO_VTABLE IEventMessageFilter : public IDAComponent
{
	DEFMETHOD(PreMessageFilter) (S32 * hwnd,    
                                 S32 * message,    
                                 S32 * wParam,
                                 S32 * lParam) = 0;

	DEFMETHOD(PostMessageFilter)(S32   hwnd,    
                                 S32   message,    
                                 S32   wParam,
                                 S32   lParam) = 0;
};



//----------------------------------------------------------------------------
// key states
//----------------------------------------------------------------------------
#define HKEF_RELEASED		0x00000000
#define HKEF_PRESSED  		0x00000001


//-------------------
//-------------------

//
// Outgoing interfaces supported:
//     IEventSink:	 (See EventMgr.h)
//		  calls SendEvent("HOTKEY", HOTKEY_IDENTIFIER);   // HOTKEY_IDENTIDIER is a #define, 
//															  generated by the hotkey editor
//
//     IEventMessageFilter:
//        calls PreMessageFilter() before handling a system message. If PreMessageFilter returns
//        an error, the system message is not processed.
//        calls PostMessageFilter() for all system messages that were NOT used. (A system message
//        is used only if the message causes the status of a hotkey to change.)
//     
//     IEventCallback:  (See EventSys.h)
//         calls the Notify() routine with the 'hotkeyMessage' passed in the HKEVENTDESC.
//			  client->Notify(hotkeyMessage, (void *)HOTKEY_IDENTIFIER);		    
//						  HOTKEY_IDENTIDIER is a #define, 
//						  generated by the hotkey editor

     


//----------------------------------------------------------------------------
// structure used in CreateInstance call
//----------------------------------------------------------------------------

struct HKEVENTDESC : public AGGDESC
{
	struct IFileSystem *	file;
	U32						joyMessage;
	U32                     hotkeyMessage;

    HKEVENTDESC (const C8 *_interfaceName = "IHotkeyEvent") : AGGDESC(_interfaceName)
	{
		memset((char *)this+sizeof(AGGDESC), 0, sizeof(*this) - sizeof(AGGDESC));
		size = sizeof(*this);
	};
};

#define IID_IHotkeyEvent MAKE_IID("IHotkeyEvent",1)

struct DACOM_NO_VTABLE IHotkeyEvent : public IAggregateComponent
{
	DEFMETHOD(Enable) (void) = 0;

	DEFMETHOD(Disable) (void) = 0;

	DEFMETHOD_(BOOL32,GetVkeyState) (U32 vkey) = 0;

	DEFMETHOD_(BOOL32,GetHotkeyState) (U32 hotkey) = 0;

    virtual void __cdecl SystemMessage ( S32   hwnd,    
                                         S32   message,    
                                         S32   wParam,
                                         S32   lParam,
                                         S32   unused1=0,
                                         S32   unused2=0) = 0;


	DEFMETHOD_(U32,GetHotkeyText) (U32 hotkey, C8 *outBuffer, U32 bufferSize) = 0;

	DEFMETHOD_(BOOL32,IsAnyHotkeyPressed) (void) = 0;
};



//----------------------------------------------------------------------------
//--------------------------END HKEvent.h-------------------------------------
//----------------------------------------------------------------------------
#endif

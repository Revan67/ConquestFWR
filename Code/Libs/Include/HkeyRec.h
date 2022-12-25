#ifndef HKEYREC_H
#define HKEYREC_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               HKeyRec.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Logfile: /Conquest/Libs/Include/HkeyRec.h $

    $Revision: 5 $

    $Date: 4/28/00 11:57p $

    $Author: Rmarr $
*/			    
//--------------------------------------------------------------------------//
/*

						Hotkey Manipulation (Recording) Library

	This library includes functions to create and modify "hotkeys". "Hotkeys" translate
	push-button events from the keyboard, mouse, or joystick (or a combination of them) into
	a game defined event. This library only handles button press/release events. Analog events 
	such as mouse movement or joystick stick positions should be handled by a seperate system.

									Rules and Definitions

	Shift, Alt, Ctrl keys, Joystick buttons, joystick POV, and mouse buttons
	can act as modifier keys or virtual keys. All of the other keys on the keyboard can only be
	used as virtual keys (i.e. non-modifiers).
	
	On the game pad (PSx), any button can be either a virtual key or a modifier key, depending on
	how the Hotkey is defined.

	In any Hotkey, there can be at most one virtual key, with any number of modifier keys. If a 
	Hotkey contains a virtual key, all modifier keys (if any) must be pressed and held before 
	the	virtual key is pressed to trigger a "press" event. To trigger a "release" event, the keys
	must be pressed in the same order as a "press" event, and the virtual key (if there is one)
	must be released first.

	If a Hotkey contains only modifier keys (no virtual keys), the order that keys are pressed does 
	not matter.


//-------------------------------------------------------------------------------------
//
DAHOTKEY CreateHotkey (U8 *lpBuffer=0);
	INPUT:
		lpBuffer: Address of instance data used to create Hotkey. If 'lpBuffer' is NULL,
		the Hotkey is created with no active events.
	RETURNS:
		An opaque handle to a Hotkey instance. A return value of NULL means some 
		unknown error occurred. (failure)
//-------------------------------------------------------------------------------------
//
BOOL32 DestroyHotkey (DAHOTKEY hotkey);
	INPUT:
		hotkey: Handle to a Hotkey
	OUTPUT:
		Releases any resources used by the Hotkey instance.
	RETURNS:
		TRUE if the Hotkey instance was freed successfully.
//-------------------------------------------------------------------------------------
//
BOOL32 ReadEvents (DAHOTKEY hotkey, U32 flags);
	INPUT:
		hotkey: Handle to a Hotkey
		flags: (bitfield) describes which types of input to ignore
			HKRF_MODIFIERS:		Ignore modifiers (single key event)
			HKRF_VKEYS:			Ignore all keyboard keys that can not be modifiers.
			HKRF_KEYBOARD:		Ignore all keyboard events
			HKRF_JOYBUTTONS: 	Ignore joystick buttons
			HKRF_MOUSEBUTTONS:	Ignore mouse buttons
			HKRF_JOYPOV:		Ignore the POV hat on the joystick
			HKRF_FORCE_VKEY:	Force last key pressed to be a vkey, if possible
	OUTPUT:
		Reads keyboard, mouse, joystick buttons and POV hat. Any button currently 
		being pressed becomes part of the Hotkey instance.
	RETURNS:
		TRUE if successful.

//-------------------------------------------------------------------------------------
//
U32 GetData (DAHOTKEY hotkey, U8 *buffer, U32 maxBytes);
	INPUT:
		hotkey: Handle to a Hotkey
		buffer: Address of buffer to receive Hotkey data.
		MaxBytes: Max size in bytes available in user buffer.
	OUTPUT:
		Copies the internal data structure of the Hotkey to the user buffer.
		If 'buffer' is NULL, no copy operation take place; function will return 
		the size (in bytes) of the structure.
	RETURNS:
		The number of bytes copied into the user's buffer. If 'maxBytes' is less than
		the amount of space needed to perform the copy, no copy is performed and 0 is 
		returned.
//-------------------------------------------------------------------------------------
//
BOOL32 IsEmpty (DAHOTKEY hotkey);
	INPUT:
		hotkey: Handle to a Hotkey
	RETURNS:	
		TRUE if the handle is invalid or the Hotkey has no events.
//-------------------------------------------------------------------------------------
//
BOOL32 SetEmpty (DAHOTKEY hotkey);
	INPUT:	
		hotkey: Handle to a Hotkey
	OUTPUT:
		Resets Hotkey instance to its initial state. (ie. with no events.)
	RETURNS:
		TRUE if handle is valid.
//-------------------------------------------------------------------------------------
//
BOOL32 IsEqual (DAHOTKEY hHotkey1, DAHOTKEY hHotkey2);
	INPUT:
		hHotkey1: Handle to a Hotkey
		hHotkey2: Handle to a Hotkey
	OUTPUT:
		Compares the Hotkeys referenced by the two handles.
	RETURNS:
		FALSE is one or both of the handles are invalid. 
		TRUE iff both Hotkeys are triggered on the same events.
//-------------------------------------------------------------------------------------
//
U32 ConvertToString (DAHOTKEY hotkey, C8 * buffer, U32 maxBytes);
	INPUT:
		hotkey: Handle to a Hotkey
		lpBuffer: Address of buffer to receive output string.
		iMaxBytes: Max size in bytes available in user buffer.
	OUTPUT:
		Converts the Hotkey referenced by 'hotkey' to an ASCIIZ string.
	RETURNS:
		The number of bytes written into the user's buffer, including the 0 terminator. 
		If 'maxBytes' is less than the amount of space needed to perform the convertion, 
		the outputed string is truncated to fit the buffer; and the last byte of the buffer
		will be 0.
		If 'buffer' is NULL, no conversion take place; function will return 
		the size (in bytes) needed to hold the ASCIIZ string.
	NOTES:
		The Hotkey instance is not changed by this method.
//-------------------------------------------------------------------------------------
//
DWORD GetModifiers (DAHOTKEY hotkey);
	INPUT:
		hotkey: Handle to a Hotkey
	RETURNS:
		Bitfield representing the keyboard modifiers present in the Hotkey.		
		HKRF_SHIFT:				either shift
		HKRF_CTRL:				either control
		HKRF_ALT:				either alt
		HKRF_MOD_JOY0			joystick button 0
		HKRF_MOD_JOY1			joystick button 1
		HKRF_MOD_JOY2			joystick button 2
		HKRF_MOD_JOY3			joystick button 3
		HKRF_MOD_POVCENTER		joystick pov center
		HKRF_MOD_POVUP			joystick pov up
		HKRF_MOD_POVDOWN		joystick pov down
		HKRF_MOD_POVRIGHT		joystick pov right
		HKRF_MOD_POVLEFT		joystick pov left
		HKRF_MOD_MOUSELEFT		mouse left button
		HKRF_MOD_MOUSERIGHT		mouse right button
		HKRF_MOD_MOUSEMIDDLE	mouse middle button

//-------------------------------------------------------------------------------------
//
BOOL32 SetModifiers (DAHOTKEY hotkey, U32 flags);
	INPUT:
		hotkey: Handle to a Hotkey
		flags: Bitfield representing the keyboard modifiers to set in Hotkey.
			HKRF_SHIFT:				either shift
			HKRF_CTRL:				either control
			HKRF_ALT:				either alt
			HKRF_MOD_JOY0			joystick button 0
			HKRF_MOD_JOY1			joystick button 1
			HKRF_MOD_JOY2			joystick button 2
			HKRF_MOD_JOY3			joystick button 3
			HKRF_MOD_POVCENTER		joystick pov center
			HKRF_MOD_POVUP			joystick pov up
			HKRF_MOD_POVDOWN		joystick pov down
			HKRF_MOD_POVRIGHT		joystick pov right
			HKRF_MOD_POVLEFT		joystick pov left
			HKRF_MOD_MOUSELEFT		mouse left button
			HKRF_MOD_MOUSERIGHT		mouse right button
			HKRF_MOD_MOUSEMIDDLE	mouse middle button
	OUTPUT:
		Sets the keyboard modifiers to keys specified in 'flags'.
	RETURNS:
		TRUE if successful.
//-------------------------------------------------------------------------------------
//
U32 GetVirtualKey (DAHOTKEY hotkey);
	INPUT:
		hotkey: Handle to a Hotkey
	RETURNS:
		Virtual key associated with the Hotkey; 0 if no virtual key.
//-------------------------------------------------------------------------------------
//
BOOL32 SetVirtualKey (DAHOTKEY hotkey, U32 key);
	INPUT:
		hotkey: Handle to a Hotkey
		dwKey: new value to store as virtual key
	OUTPUT:
		Adds the virtual key 'dwKey' to the Hotkey, overwriting the previous virtual key value.
	RETURNS:
		TRUE if successful.
//----------------------------------------------------------------------------
//
U32 GetNumKeys (DAHOTKEY hotkey);
	INPUT:
		hotkey: Handle to a Hotkey
	RETURNS:
		Total number of keys to press in order to trigger event.
		(ie. Number of modifier keys + 1 (if virtual key exists))
//----------------------------------------------------------------------------
//
U32 GetTriggerType (DAHOTKEY hotkey);
	INPUT:
		hotkey: Handle to a Hotkey
	RETURNS:
		Trigger type for the Hotkey.
			HKR_PRESSED: Triggered when all keys in Hotkey are pressed. The virtual key (if 
				the Hotkey has one) must have been pressed after all modifier keys. 
			HKR_RELEASE: Triggered when all keys in Hotkey are pressed in the correct order 
			(modifiers before virtual keys), then at least one of the keys is released. If the 
			Hotkey contains a virtual key, it must be the first key released.

//----------------------------------------------------------------------------
//
BOOL32 SetTriggerType (DAHOTKEY hotkey, U32 trigger);
	INPUT:
		hotkey: Handle to a Hotkey
		trigger: New trigger type for the Hotkey
			HKR_PRESSED: Triggered when all keys in Hotkey are pressed. The virtual key (if 
				the Hotkey has one) must have been pressed after all modifier keys. 
			HKR_RELEASE: Triggered when all keys in Hotkey are pressed in the correct order 
			(modifiers before virtual keys), then at least one of the keys is released. If the 
			Hotkey contains a virtual key, it must be the first key released.
	OUTPUT:
		Sets the trigger type for the Hotkey to 'trigger'.
	RETURNS:
		TRUE if successful

//----------------------------------------------------------------------------
//
U32 GetSymbolNumber (DAHOTKEY hotkey);
	INPUT:
		hotkey: Handle to a Hotkey
	RETURNS:
		The Symbol value for this event. (Application defined)

//----------------------------------------------------------------------------
//
BOOL32 SetSymbolNumber (DAHOTKEY hotkey, U32 num);
	INPUT:
		hotkey: Handle to a Hotkey
		num: New symbol value for the event.
	RETURNS:
		True if successful.
	OUTPUT:
		Sets the symbol value for the event to 'num'. The meaning of 
		this number is application defined.


*/
//----------------------------------------------------------------------------
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

#define HKEVENT_VERSION 0x101

//----------------------------------------------------------------------------
// flags for ReadEvents()
//----------------------------------------------------------------------------
#define	HKRF_MODIFIERS			0x00000001
#define	HKRF_VKEYS				0x00000002
#define	HKRF_KEYBOARD			0x00000004
#define	HKRF_JOYBUTTONS			0x00000008
#define	HKRF_MOUSEBUTTONS		0x00000010
#define	HKRF_JOYPOV				0x00000020
#define HKRF_JOYDIR				0x00000040
#define HKRF_FORCE_VKEY			0x00000080

//----------------------------------------------------------------------------
// modifiers bitfield
//----------------------------------------------------------------------------

#define	HKRF_SHIFT				0x00000001
#define	HKRF_CTRL				0x00000002
#define	HKRF_ALT				0x00000004
#define HKRF_MOD_JOY0			0x00000008
#define HKRF_MOD_JOY1			0x00000010
#define HKRF_MOD_JOY2			0x00000020
#define HKRF_MOD_JOY3			0x00000040

#define HKRF_MOD_DIR_U			0x00020000
#define HKRF_MOD_DIR_UR			0x00040000
#define HKRF_MOD_DIR_R			0x00080000
#define HKRF_MOD_DIR_DR			0x00100000
#define HKRF_MOD_DIR_D			0x00200000
#define HKRF_MOD_DIR_DL			0x00400000
#define HKRF_MOD_DIR_L			0x00800000
#define HKRF_MOD_DIR_UL			0x01000000

#define HKRF_MOD_POVCENTER		0x00000000
#define HKRF_MOD_POVUP			0x02000000
#define HKRF_MOD_POVDOWN		0x04000000
#define HKRF_MOD_POVRIGHT		0x08000000
#define HKRF_MOD_POVLEFT		0x10000000

#define HKRF_MOD_MOUSELEFT		0x20000000
#define HKRF_MOD_MOUSERIGHT		0x40000000
#define HKRF_MOD_MOUSEMIDDLE	0x80000000

//----------------------------------------------------------------------------
// Virtual keys values
// 0 - 255 are keyboard virtual keys
//----------------------------------------------------------------------------

#define HKR_VKEY_JOY0			0x00000100
#define HKR_VKEY_JOY1			0x00000101
#define HKR_VKEY_JOY2			0x00000102
#define HKR_VKEY_JOY3			0x00000103
#define HKR_VKEY_JOY4			0x00000104
#define HKR_VKEY_JOY5			0x00000105
#define HKR_VKEY_JOY6			0x00000106
#define HKR_VKEY_JOY7			0x00000107
#define HKR_VKEY_JOY8			0x00000108
#define HKR_VKEY_JOY9			0x00000109
#define HKR_VKEY_JOY10			0x0000010A
#define HKR_VKEY_JOY11			0x0000010B
#define HKR_VKEY_JOY12			0x0000010C
#define HKR_VKEY_JOY13			0x0000010D
#define HKR_VKEY_JOYLAST		HKR_VKEY_JOY13

#define HKR_VKEY_DIR_U			0x0000010E
#define HKR_VKEY_DIR_UR			0x0000010F
#define HKR_VKEY_DIR_R			0x00000110
#define HKR_VKEY_DIR_DR			0x00000111
#define HKR_VKEY_DIR_D			0x00000112
#define HKR_VKEY_DIR_DL			0x00000113
#define HKR_VKEY_DIR_L			0x00000114
#define HKR_VKEY_DIR_UL			0x00000115

#define HKR_VKEY_POVUP			0x00000116
#define HKR_VKEY_POVDOWN		0x00000117
#define HKR_VKEY_POVRIGHT		0x00000118
#define HKR_VKEY_POVLEFT		0x00000119

#define HKR_VKEY_MOUSELEFT		0x0000011A
#define HKR_VKEY_MOUSERIGHT		0x0000011B
#define HKR_VKEY_MOUSEMIDDLE	0x0000011C
#define HKR_VKEY_MOUSELAST		HKR_VKEY_MOUSEMIDDLE

#define HKR_VKEY_ALPHA			0x0000011D
#define HKR_VKEY_NUMBER			0x0000011E
#define HKR_VKEY_OTHER			0x0000011F
#define HKR_VKEY_SINGLE			0x00000120

#define HKR_VKEY_LAST			HKR_VKEY_SINGLE

//----------------------------------------------------------------------------
// trigger event types
//----------------------------------------------------------------------------

#define HKR_PRESSED				0x00000000
#define HKR_RELEASED			0x00000001

//----------------------------------------------------------------------------
// Partially defined hotkey data type 
//----------------------------------------------------------------------------

// struct DAHOTKEY_STRUCT;
typedef struct DAHOTKEY_STRUCT * DAHOTKEY;


//----------------------------------------------------------------------------
// structure used in CreateInstance call
//----------------------------------------------------------------------------

struct HKRECDESC : public DACOMDESC
{
	void *hWindow;

    HKRECDESC (const C8 *_interfaceName = "IHotkeyRecorder") : DACOMDESC(_interfaceName)
	{
		hWindow=0;
		size = sizeof(*this);
	};
};


#define IID_IHotkeyRecorder MAKE_IID("IHotkeyRecorder",1)

struct IHotkeyRecorder : public IDAComponent
{
	DEFMETHOD_(DAHOTKEY,CreateHotkey) (const U8 *buffer=0) = 0;

	DEFMETHOD_(BOOL32,DestroyHotkey) (DAHOTKEY hotkey) = 0;

	DEFMETHOD_(BOOL32,ReadEvents) (DAHOTKEY hotkey, U32 flags) = 0;

	DEFMETHOD_(U32,GetData) (DAHOTKEY hotkey, U8 *buffer, U32 maxBytes) = 0;

	DEFMETHOD_(BOOL32,IsEqual) (DAHOTKEY hotkey1, DAHOTKEY hotkey2) = 0;

	DEFMETHOD_(BOOL32,IsEmpty) (DAHOTKEY hotkey) = 0;

	DEFMETHOD_(BOOL32,SetEmpty) (DAHOTKEY hotkey) = 0;

	DEFMETHOD_(U32,ConvertToString) (DAHOTKEY hotkey, C8 * buffer, U32 maxBytes) = 0;;

	DEFMETHOD_(U32,GetModifiers) (DAHOTKEY hotkey) = 0;

	DEFMETHOD_(BOOL32,SetModifiers) (DAHOTKEY hotkey, U32 flags) = 0;

	DEFMETHOD_(U32,GetVirtualKey) (DAHOTKEY hotkey) = 0;

	DEFMETHOD_(BOOL32,SetVirtualKey) (DAHOTKEY hotkey, U32 key) = 0;

	DEFMETHOD_(U32,GetNumKeys) (DAHOTKEY hotkey) = 0;

	DEFMETHOD_(U32,GetTriggerType) (DAHOTKEY hotkey) = 0;

	DEFMETHOD_(BOOL32,SetTriggerType) (DAHOTKEY hotkey, U32 trigger) = 0;

	DEFMETHOD_(U32,GetSymbolNumber) (DAHOTKEY hotkey) = 0;

	DEFMETHOD_(BOOL32,SetSymbolNumber) (DAHOTKEY hotkey, U32 num) = 0;
};

//----------------------------------------------------------------------------
//--------------------------END HKeyRec.h-------------------------------------
//----------------------------------------------------------------------------
#endif

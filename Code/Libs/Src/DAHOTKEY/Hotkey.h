#ifndef HOTKEY_H
#define HOTKEY_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                HotKey.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Logfile: /Libs/Src/DAHotkey/Hotkey.h $

    $Revision: 2 $

    $Date: 8/02/97 3:34p $

    $Author: Jasony $
*/			    
//--------------------------------------------------------------------------//

#pragma pack ( push, 4 )

struct DAHOTKEY_STRUCT
{
	unsigned mKEY:3;
	unsigned mJOY:14;
	unsigned mDIR:8;		// 8 way directional joystick buttons
	unsigned mHAT:4;
	unsigned mMOUSE:3;
	unsigned vKEY:9;
	unsigned bType:1;
	unsigned bValid:1;
	unsigned symbol:12;
	unsigned bDirty:1;
	unsigned bPressed:1;

	int init (void)
	{
	 	memset(this, 0, sizeof(*this));
		return (bValid = 1);
	}

	int operator == (DAHOTKEY_STRUCT & key)
	{
		return (memcmp(this, &key, sizeof(*this))==0);
	}

	DWORD getModifiers (void)
	{
	 	DWORD result;

		result = ((DWORD *)this)[0];		// mods take up first 32 bits of field
		return result;
	}

	void setModifiers (DWORD dwMods)
	{
		DWORD * ptr = (DWORD *)this;

		*ptr = dwMods;						// mods take up first 32 bits of field
	}

	S32 __fastcall compare (DAHOTKEY_STRUCT & rec);

	BOOL32 operator > (DAHOTKEY_STRUCT & rec)
	{
	 	return (compare(rec) > 0);
	}

	BOOL32 operator < (DAHOTKEY_STRUCT & rec)
	{
	 	return (compare(rec) < 0);
	}
};			

struct KB_MODIFIERS
{
	BYTE bLeftShift:1;
	BYTE bRightShift:1;
	BYTE bLeftControl:1;
	BYTE bRightControl:1;
	BYTE bLeftAlt:1;
	BYTE bRightAlt:1;

	void reset (void)
	{
	 	memset(this, 0, sizeof(*this));
	}

	int isEmpty (void)
	{
	 	return (((BYTE *)this)[0] == 0);
	}
};


#pragma pack ( pop )

#endif

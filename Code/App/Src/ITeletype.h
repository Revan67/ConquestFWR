#ifndef ITELETYPE_H
#define ITELETYPE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               ITeletype.h                                //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Sbarton $

   $Header: /Conquest/App/Src/ITeletype.h 8     9/25/00 10:39p Sbarton $
*/
//--------------------------------------------------------------------------//
 
#ifndef DACOM_H
#include <DACOM.h>
#endif


struct ITeletype : IDAComponent
{
	virtual U32  CreateTeletypeDisplay (const wchar_t *string, RECT rc, U32 fontID, COLORREF color, U32 lifetime = 10000, U32 displayTime = 5000, bool bMute = false, bool bCenter = false) = 0;

	virtual U32  CreateTeletypeDisplayEx (const wchar_t *string, RECT rc, U32 fontID, COLORREF color, U32 lifetime = 10000, U32 displayTime = 5000, bool bMute = false, bool bIgnorePause = false) = 0;

	virtual BOOL32 IsPlaying (U32 teletypeID) = 0;

	virtual void  Flush (void) = 0;	// kill all teletyping that's going on

	virtual U32 GetLastTeletypeID (void) = 0;
	
	virtual void SetLastTeletypeID (U32 id) = 0;
};

#endif
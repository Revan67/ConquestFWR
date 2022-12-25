#ifndef ILINEMANAGER_H
#define ILINEMANAGER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               ILineManager.h                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Sbarton $
*/
//--------------------------------------------------------------------------//
 
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct ILineManager : IDAComponent
{
	virtual U32 __stdcall CreateLineDisplay (S32 x1, S32 y1, S32 x2, S32 y2, COLORREF color, U32 lifetime = 10000, U32 displayTime = 5000) = 0;
	
	virtual U32 __stdcall CreateLineDisplay (S32 x1, S32 y1, IBaseObject* obj, COLORREF color, U32 lifetime = 10000, U32 displayTime = 5000) = 0;
	
	virtual U32 __stdcall CreateLineDisplay (IBaseObject* obj1, IBaseObject* obj2, COLORREF color, U32 lifetime = 10000, U32 displayTime = 5000) = 0;
	
	virtual void __stdcall DeleteLine (U32 lineID) = 0;	
	
	virtual void __stdcall Flush (void) = 0;	// kill all line drawing that's going on
};

#endif
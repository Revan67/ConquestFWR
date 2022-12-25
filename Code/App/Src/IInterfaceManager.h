#ifndef IINTERFACEMANAGER_H
#define IINTERFACEMANAGER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               IInterfaceManager.h                        //
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

struct IInterfaceManager : IDAComponent
{
	virtual void StartHotkeyFlash (U32 hotkeyID) = 0;

	virtual void StopHotkeyFlash (U32 hotkeyID) = 0;
	
	virtual void Flush () = 0;

	virtual bool RenderHotkeyFlashing (U32 hotkeyID) = 0;
};

#endif
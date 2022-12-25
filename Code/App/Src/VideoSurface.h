#ifndef VIDEOSURFACE_H
#define VIDEOSURFACE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             VideoSurface.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/VideoSurface.h 4     11/08/00 12:10p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef VFX_H
#include <VFX.h>
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IVideoSurface : IDAComponent
{
	virtual const VFX_WINDOW & GetWindow (void) = 0;

	virtual const PANE & GetPane (void) = 0;

	virtual bool Lock (void) = 0;			// true if successful (can fail if app is in background)
	
	virtual void Unlock (void) = 0;

	virtual bool IsLocked (void) = 0;

	virtual bool LockFrontBuffer (void) = 0;

	virtual void UnlockFrontBuffer (void) = 0;
};

//---------------------------------------------------------------------------
//-------------------------------END VideoSurface.h--------------------------
//---------------------------------------------------------------------------
#endif

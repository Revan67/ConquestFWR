#ifndef IANIMATE_H
#define IANIMATE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IAnimate.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IAnimate.h 8     9/01/00 4:42p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct BaseHotRect;
struct ANIMATE_DATA;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IAnimate : IDAComponent
{
	virtual void InitAnimate (const ANIMATE_DATA & data, BaseHotRect * parent, const U32 * indexArray, U32 numElements) = 0; 

	virtual void SetVisible (bool bVisible) = 0;

	virtual void PauseAnim (bool bPause) = 0;

	// deletes the instance at a later, more convenient time
	virtual void DeferredDestruction (void) = 0;

	virtual void SetAnimationData (const U32 * indexArray, U32 numElements) = 0;

	virtual void SetLoopingAnimation (bool bLooping) = 0;

	virtual bool HasAnimationCompleted (void) = 0;

	virtual void SetAnimationFrameRate (U32 frameRate);
};

#endif
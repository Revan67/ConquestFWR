#ifndef IPROGRESSSTATIC_H
#define IPROGRESSSTATIC_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IStatic.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IProgressStatic.h 2     6/27/00 12:08p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct BaseHotRect;
struct PROGRESS_STATIC_DATA;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IProgressStatic : IDAComponent
{
	virtual void InitProgressStatic (const PROGRESS_STATIC_DATA & data, BaseHotRect * parent, bool bHighPriority = false) = 0; 

	virtual void SetText (const wchar_t *szText) = 0; 

	virtual void SetTextID (const U32 textID) = 0;

	virtual int  GetText (wchar_t * szBuffer, int bufferSize) = 0;

	virtual void SetVisible (bool bVisible) = 0;

	virtual void EnableRollupBehavior (int value) = 0;	// the control will display text rolling up to the value given

	virtual void SetProgress(U32 current, U32 max) = 0;
};

#endif
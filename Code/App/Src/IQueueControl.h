#ifndef IQUEUECONTROL_H
#define IQUEUECONTROL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IQueueControl.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IQueueControl.h 4     4/04/00 10:43a Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct BaseHotRect;
struct QUEUECONTROL_DATA;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IQueueControl : IDAComponent
{
	virtual void InitQueueControl (const QUEUECONTROL_DATA & data, BaseHotRect * parent) = 0; 

	virtual void SetVisible (bool bVisible) = 0;

	virtual void AddToQueue(IDrawAgent ** shape,U32 slotID) = 0;

	virtual void ResetQueue() = 0;

	virtual void SetPercentage(SINGLE percent, U32 _stallType) = 0;

	virtual void SetControlID(U32 newID) = 0;
};

#endif
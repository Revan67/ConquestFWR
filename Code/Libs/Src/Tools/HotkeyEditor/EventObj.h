//****************************************************************************
//*                                                                          *
//*  EVENTOBJ.H: Event notification object                                   *
//*                                                                          *
//*  Source compatible with 32-bit 80386 C/C++                               *
//*                                                                          *
//*  V1.00 of 2-Mar-97: Initial                                              *
//*                                                                          *
//*  32-bit protected-mode source compatible with Watcom 10.5/MSC 9.0        *
//*                                                                          *
//*  Author: Jason Yenawine                                                  *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  The EventObject is an abstract base class which can be used to receive	 *
//*  messages from the event system. (See EventSys.h)                        *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1997 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#ifndef EVENTOBJ_H
#define EVENTOBJ_H

#ifndef US_TYPEDEFS
#include "typedefs.h"
#endif



class BaseEventObject
{
protected:
	struct IEventSystem *eventSys;
	void				*proc;
public:

	BaseEventObject (void)
	{
		eventSys=0;
	}

	virtual ~BaseEventObject (void);

	virtual void __cdecl NotifyCallback (U32 message, void *param) = 0;

	void WINAPI Notify (struct IEventSystem *_eventSys);
};





#endif
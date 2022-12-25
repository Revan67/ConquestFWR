//****************************************************************************
//*                                                                          *
//*  EVENTMGR.H: DA COM event/message/notification manager                   *
//*                                                                          *
//*  Source compatible with 32-bit 80386 C/C++                               *
//*                                                                          *
//*  V1.00 of 2-Mar-97: Initial                                              *
//*                                                                          *
//*  32-bit protected-mode source compatible with Watcom 10.5/MSC 9.0        *
//*                                                                          *
//*  Author: John Miles                                                      *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  The event manager provides a system-independent abstraction layer for   *
//*  dispatching events and messages to clients.                             *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1997 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#ifndef EVENTMGR_H
#define EVENTMGR_H

//
// General type definitions for portability
// 
#ifndef DACOM_H
#include "dacom.h"   // DA component object manager
#endif

#ifndef YES
#define YES 1
#endif

#ifndef NO
#define NO  0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE  0
#endif

//
// Preference names and default values
//

#define N_EVTMGR_PREFS      0     // # of preference types

//****************************************************************************
//*                                                                          *
//* EVNTMGR class definition                                                 *
//*                                                                          *
//****************************************************************************

typedef S32 HNOTIFY;

typedef void (__cdecl * NOTIFY_CALLBACK)(const void *instance, 
                                         S32   parm0,    
                                         S32   parm1,    
                                         S32   parm2,
                                         S32   parm3,
                                         S32   context,
                                         S32   message_identifier);

typedef void (__cdecl IDAComponent::*DACOM_NOTIFY_CALLBACK) (
                                         S32   parm0,    
                                         S32   parm1,    
                                         S32   parm2,
                                         S32   parm3,
                                         S32   context,
                                         S32   message_identifier);


//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------

struct DACOM_NO_VTABLE IEventSink : public IDAComponent 
{
	// 
	// A component implements IEventSink to receive notification when events occur.
	// 

   virtual void    COMAPI SendEvent         (const C8    *message,
                                              S32          parm0    = -1,
                                              S32          parm1    = -1,
                                              S32          parm2    = -1,
                                              S32          parm3    = -1) = 0;

};

// 
// NOTE: The event manager implements IEventSink::SendEvent() by routing the call to IEventManager::post_event()
//


//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------


struct DACOM_NO_VTABLE IEventManager : public IDAComponent
{
   //
   // Provide service to the event manager once per frame
   //
   // This function does NOT need to be called unless the post_event()
   // function is used to postpone message transmission
   //

   virtual void    COMAPI serve              (void) = 0;

   //
   // Arrange for the event manager to call a specified function when a
   // specified event is dispatched
   //

   virtual HNOTIFY COMAPI notify             (const void  *instance,
											  NOTIFY_CALLBACK         callback,
                                              const C8    *message,
                                              S32          parm0        = -1,
                                              S32          parm1        = -1,
                                              S32          parm2        = -1,
                                              S32          parm3        = -1,
                                              S32          context      = -1) = 0; 

   //
   // Cancel further event callbacks
   //
   // A NULL message name or callback function matches any entry, as does
   // a -1 parameter or context value
   //

   virtual void    COMAPI cancel             (HNOTIFY      handle)  = 0;

   virtual void    COMAPI cancel             (const void  *instance =  NULL,
                                   NOTIFY_CALLBACK         callback =  NULL,
                                              const C8    *message  =  NULL,
                                              S32          parm0    = -1,
                                              S32          parm1    = -1,
                                              S32          parm2    = -1,
                                              S32          parm3    = -1,
                                              S32          context  = -1) = 0; 

   //
   // Return TRUE if the specified event was posted to the queue during
   // the last service interval
   // 

   virtual BOOL32  COMAPI test_event         (const C8    *message,
                                              S32          parm0   = -1,
                                              S32          parm1   = -1,
                                              S32          parm2   = -1,
                                              S32          parm3   = -1) = 0;

   //
   // Post an event to the event manager's internal queue, to be dispatched
   // during the next serve() call
   //
   // Typically used for input events which are processed at the end of each
   // frame
   //

   virtual void    COMAPI post_event         (const C8    *message,
                                              S32          parm0        = -1,
                                              S32          parm1        = -1,
                                              S32          parm2        = -1,
                                              S32          parm3        = -1) = 0;

   //
   // Send an event to any/all registered receiver functions, without
   // queueing it
   //

   virtual void    COMAPI send_event         (const C8    *message,
                                              S32          parm0    = -1,
                                              S32          parm1    = -1,
                                              S32          parm2    = -1,
                                              S32          parm3    = -1) = 0;

   //
   // Register a constant message identifier meaningful to clients 
   // of a particular event manager instance
   //
   // The specified message identifier will be received by all message
   // handlers, and can be used to allow more than one message type
   // to be processed by each handler function (e.g., with a switch {} 
   // statement)
   //
   // Returns the previous constant identifier associated with this message,
   // if any
   //

   virtual S32     COMAPI register_message_identifier
                                             (const C8    *message,
                                              S32          message_identifier) = 0;

   //
   // Allow the application to inspect the event queue on an entry-by-entry
   // basis (e.g., to check for input events posted during the last service
   // interval)
   //

   virtual BOOL32  COMAPI fetch_event        (S32          entry_number,  
                                              C8         **message_name       = NULL,
                                              S32         *message_identifier = NULL,
                                              S32         *parm0              = NULL,
                                              S32         *parm1              = NULL,
                                              S32         *parm2              = NULL,
                                              S32         *parm3              = NULL) = 0;
   //
   // Arrange for the event manager to call a specified component method when a
   // specified event is dispatched
   //

   HNOTIFY notify                            (IDAComponent *instance,
											  DACOM_NOTIFY_CALLBACK  callback,
                                              const C8    *message,
                                              S32          parm0        = -1,
                                              S32          parm1        = -1,
                                              S32          parm2        = -1,
                                              S32          parm3        = -1,
                                              S32          context      = -1)
   {
		NOTIFY_CALLBACK proc;
	   
	    _asm 
		{
		   mov eax, DWORD PTR [callback]
		   mov DWORD PTR [proc], eax
		};
		
		return notify(instance, proc, message, parm0, parm1, parm2, parm3, context);
   }


   void cancel                              (IDAComponent *instance,
                                              DACOM_NOTIFY_CALLBACK callback,
                                              const C8    *message  =  NULL,
                                              S32          parm0    = -1,
                                              S32          parm1    = -1,
                                              S32          parm2    = -1,
                                              S32          parm3    = -1,
                                              S32          context  = -1)
   {
		NOTIFY_CALLBACK proc;
	   
	    _asm 
		{
		   mov eax, DWORD PTR [callback]
		   mov DWORD PTR [proc], eax
		};
	   
		cancel(instance, proc, message, parm0, parm1, parm2, parm3, context);
   }


};



#endif

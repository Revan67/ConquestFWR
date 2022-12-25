#ifndef EVENTSYS_H
#define EVENTSYS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               EventSys.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/Libs/Include/EventSys.h 5     4/28/00 11:57p Rmarr $
*/			    
//--------------------------------------------------------------------------//


#ifndef DACOM_H
#include "DACOM.h"
#endif

/*
   The EventSystem component supports the following incoming interfaces:
		ISystemComponent
		IEventSystem
		IEventCallback

   And the following outgoing interfaces:
		IEventCallback

  
   To create an EventSystem, use the AGGDESC structure, setting the 'interface_name'
   member to "IEventSystem".


//---------------------
//
GENRESULT ISystemComponent::Update (void);
	RETURNS:
		GR_OK on success.
	OUTPUT:
		Calls each of the registered notification routines for each message that
		has been posted since the last call to Update().
		If a notification routine posts more messages, these new messages are
		also delived. The delivery loop continues until there are no messages
		waiting to be delivered.

//---------------------
//
GENRESULT IEventSystem::Initialize (U32 flags);
	INPUT:
		flags:  should be zero. (no flags have been implemented)
	RETURNS:
		GR_OK If succssfully initialized.

  
//---------------------
//
GENRESULT IEventSystem::Post (U32 message, void *param = 0);
	INPUT:
		message: User defined message
		param: User defined parameter
	RETURNS:
		GR_OK on success.
		GR_OUT_OF_MEMORY if a memory allocation failed.
	OUTPUT:
		Adds a user defined message to the event queue. The message will be
		delivered the next time Update() is called.


//---------------------
//
GENRESULT IEventSystem::Send (U32 message, void *param = 0);
	INPUT:
		message: User defined message
		param: User defined parameter
	RETURNS:
		GR_OK on success.
	OUTPUT:
		Sends the user defined message to all registered callback routines.


//---------------------
//
GENRESULT IEventSystem::Peek (U32 index, U32 *message = 0, void **param = 0);
	INPUT:
		index: specifies which message in the queue to inspect
		message: address of U32 that receives the message value
		param: address of variable that receives the param for the message
	RETURNS:
		GR_OK on success.
		GR_GENERIC if the end of the list has been reached.
	OUTPUT:
		The method allows the application to inspect the messages in the queue.
		To inspect every message in the queue, call Peek() with 'index' == 0, then
		repeatedly call Peek() with an incrementally larger 'index' value
		until it returns GR_GENERIC.
		'message' can be NULL if the application does not desire the message value.
		Likewise, 'param' can be NULL if the application does not desire 
		the param value for the message.

	Example Usage:

		extern IEventSystem * EVENT;
		...
		U32 index=0;
		U32 message;
		void *param;

		while (EVENT->Peek(index++, &message, &param) == GR_OK)
			switch (message)
			{
				case ...
					...
				case ...
			};


*/

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

#define IID_IEventCallback MAKE_IID("IEventCallback",1)

struct DACOM_NO_VTABLE IEventCallback : public IDAComponent 
{
	DEFMETHOD(Notify) (U32 message, void *param = 0) = 0;
};


//---------------------
//-- flags for use in Initialize
//---------------------

#define DAEVENTFLAG_MULTITHREADED	0x00000001

//---------------------

#define IID_IEventSystem MAKE_IID("IEventSystem",1)

struct DACOM_NO_VTABLE IEventSystem : public IDAComponent
{
	DEFMETHOD(Initialize) (U32 flags) = 0;

	DEFMETHOD(Post) (U32 message, void *param = 0) = 0;

	DEFMETHOD(Send) (U32 message, void *param = 0) = 0;

	DEFMETHOD(Peek) (U32 index, U32 *message = 0, void **param = 0) = 0;
};



#endif
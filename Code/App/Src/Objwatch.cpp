//--------------------------------------------------------------------------//
//                                                                          //
//                              OBJWATCH.CPP                                //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jasony $

    $Header: /Conquest/App/Src/Objwatch.cpp 8     4/18/00 11:13a Jasony $  
*/			    
//-------------------------------------------------------------------

#include "pch.h"
#include <globals.h>

#include "objwatch.h"

// 0 = NONSYSVOLATILEPTR
// > SYSVOLATILEPTR == do not add to list
//-------------------------------------------------------------------
//

//-------------------------------------------------------------------
//
/*
static void checkCycle (OBJPTR<IBaseObject> * list)
{
	OBJPTR<IBaseObject> * node = list, *node2=list;

	while (node && node2)
	{
		node=node->next;
		if ((node2=node2->next)!=0)
			node2=node2->next;
		CQASSERT(node==0 || node!=node2);	// should never be equal
	}
}
*/
//-------------------------------------------------------------------
//
void __fastcall InitObjectPointer (OBJPTR<IBaseObject> & instance, U32 playerID, IBaseObject * targetObject, U32 offset)
{
	if (instance.ptr != targetObject)
	{
		UninitObjectPointer(instance);			// remove watcher from list, playerID must be set correctly

		instance.playerID = playerID;

		if (playerID <= SYSVOLATILEPTR)		// if greater than SYSVOLATILEPTR, don't add to list
		{
			if(targetObject)
			{
				instance.next = targetObject->objwatch;
				instance.prev = NULL;
				if(targetObject->objwatch)
					targetObject->objwatch->prev = &instance;

				targetObject->objwatch = &instance;
			}
		}
	}
	instance.playerID = playerID;
	instance.ptr = targetObject;
	instance.offset = offset;
	instance.verify();
}
//-------------------------------------------------------------------
//
void __fastcall UninitObjectPointer (OBJPTR<IBaseObject> & instance)
{
	if (instance.playerID <= SYSVOLATILEPTR && instance.ptr)
	{
		if (instance.prev)
			instance.prev->next = instance.next;
		else
			instance.ptr->objwatch = instance.next;
		if (instance.next)
			instance.next->prev = instance.prev;
	}
	instance.next = instance.prev = 0;
	instance.ptr = 0;
	instance.offset = 0;
	instance.playerID = 0xFFFFFFFF;
}
//-------------------------------------------------------------------
//
void __fastcall UnregisterWatchersForObjectForPlayer (IBaseObject *lpObject, U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	OBJPTR<IBaseObject> * node = lpObject->objwatch;

	while (node)
	{
		node->verify();
		if (node->playerID == playerID)
		{
			if (node->prev)
				node->prev->next = node->next;
			else  // beginning of the list
				lpObject->objwatch = node->next;
			if (node->next)
				node->next->prev = node->prev;
			OBJPTR<IBaseObject> * tmp = node->next;
			node->next = node->prev = 0;
			node->ptr = 0;
			node->offset = 0;
			node->playerID = 0xFFFFFFFF;
			node = tmp;
		}
		else
			node = node->next;
	}
}
//-------------------------------------------------------------------
//
void __fastcall UnregisterNonSystemVolatileWatchersForObject (IBaseObject *lpObject)
{
	OBJPTR<IBaseObject> * node = lpObject->objwatch;

	while (node)
	{
		node->verify();
		if (node->playerID == NONSYSVOLATILEPTR)
		{
			if (node->prev)
				node->prev->next = node->next;
			else  // beginning of the list
				lpObject->objwatch = node->next;
			if (node->next)
				node->next->prev = node->prev;
			OBJPTR<IBaseObject> * tmp = node->next;
			node->next = node->prev = 0;
			node->ptr = 0;
			node->offset = 0;
			node->playerID = 0xFFFFFFFF;
			node = tmp;
		}
		else
			node = node->next;
	}
}
//-------------------------------------------------------------------
//
void __fastcall UnregisterWatchersForObject (IBaseObject *lpObject)
{
	OBJPTR<IBaseObject> * node = lpObject->objwatch;

	while (node)
	{
		node->verify();
		if (node->prev)
			node->prev->next = node->next;
		else  // beginning of the list
			lpObject->objwatch = node->next;
		if (node->next)
			node->next->prev = node->prev;
		OBJPTR<IBaseObject> * tmp = node->next;
		node->next = node->prev = 0;
		node->ptr = 0;
		node->offset = 0;
		node->playerID = 0xFFFFFFFF;
		node = tmp;
	}
}
//-------------------------------------------------------------------
//
void __fastcall UnregisterSystemVolatileWatchersForObject (IBaseObject *lpObject)
{
	OBJPTR<IBaseObject> * node = lpObject->objwatch;

	while (node)
	{
		node->verify();
		if (node->playerID == SYSVOLATILEPTR)
		{
			if (node->prev)
				node->prev->next = node->next;
			else  // beginning of the list
				lpObject->objwatch = node->next;
			if (node->next)
				node->next->prev = node->prev;
			OBJPTR<IBaseObject> * tmp = node->next;
			node->next = node->prev = 0;
			node->ptr = 0;
			node->offset = 0;
			node->playerID = 0xFFFFFFFF;
			node = tmp;
		}
		else
			node = node->next;
	}
}
//-------------------------------------------------------------------
//
void __fastcall UnregisterWatchersForObjectForPlayerMask (IBaseObject *lpObject, U32 playerMask)
{
	int i;

	if (playerMask & (1 << NONSYSVOLATILEPTR))
		UnregisterNonSystemVolatileWatchersForObject(lpObject);
	if (playerMask & (1 << SYSVOLATILEPTR))
		UnregisterSystemVolatileWatchersForObject(lpObject);

	for (i = 1; i <= MAX_PLAYERS; i++)
	{
		if (playerMask & (1 << i))
			UnregisterWatchersForObjectForPlayer(lpObject, i);
	}
}
//------------------------------------------------------------------------------
//------------------------------END OBJWATCH.CPP--------------------------------
//------------------------------------------------------------------------------

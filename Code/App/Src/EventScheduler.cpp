//--------------------------------------------------------------------------//
//                                                                          //
//                               EventScheduler.cpp                         //
//                                                                          //
//                  COPYRIGHT (C) 2004 Warthog, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "EventScheduler.h"
#include "IWeapon.h"
#include "Objwatch.h"
#include "Startup.h"
#include "Objlist.h"
#include "ILauncher.h"

struct DamageInfo
{
	U32 shooterID;
	U32 targetID;
	U32 amount;
};

struct EventNode
{
	EventNode * nextBucket;
	EventNode * nextSibling;

	U32 updateTime;
	
	enum EventType
	{
		ET_DAMAGE
	}eventType;

	union Info
	{
		DamageInfo damageInfo; //ET_DAMAGE
	}info;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct DACOM_NO_VTABLE EventScheduler : IEventScheduler
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(EventScheduler)
	DACOM_INTERFACE_ENTRY(IEventScheduler)
	END_DACOM_MAP()

	EventNode * eventList;

	EventNode * freeList;

	// methods

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	EventScheduler (void)
	{
		eventList = NULL;
	}

	~EventScheduler (void);
	
    /* IEventScheduler methods */

	virtual void QueueDamageEvent(SINGLE timeTilEvent,U32 _ownerID, U32 targetID, U32 _amount);

	virtual void CleanQueue();

	virtual void UpdateQueue();

	//EventScheduler

	void executeEvent(EventNode * event);
};
//------------------------------------------------------------------------
//
EventScheduler::~EventScheduler (void)
{
	CleanQueue();
}
//----------------------------------------------------------------------------------------------
//
void EventScheduler::QueueDamageEvent(SINGLE timeTillEvent,U32 _ownerID, U32 targetID, U32 _amount)
{
	EventNode * node = NULL;
	if(freeList)
	{
		node = freeList;
		freeList = freeList->nextBucket;
	}
	else
	{
		node = new EventNode;
	}
	node->eventType = EventNode::ET_DAMAGE;
	node->info.damageInfo.shooterID = _ownerID;
	node->info.damageInfo.targetID = targetID;
	node->info.damageInfo.amount = _amount;

	node->updateTime = MGlobals::GetUpdateCount() + __max(timeTillEvent/(ELAPSED_TIME/8),1);

	EventNode * search = eventList;
	EventNode * prev = NULL;
	while(search)
	{
		if(search->updateTime == node->updateTime)
		{
			node->nextSibling = search;
			node->nextBucket = search->nextBucket;
			if(prev)
				prev->nextBucket = node;
			else
				eventList = node;
			break;
		}
		else if(search->updateTime > node->updateTime)
		{
			node->nextSibling = NULL;
			node->nextBucket = search;
			if(prev)
				prev->nextBucket = node;
			else
				eventList = node;
			break;
		}
		prev = search;
		search = search->nextBucket;
	}
	if(!search)
	{
		node->nextBucket = NULL;
		node->nextSibling = NULL;
		if(prev)
			prev->nextBucket = node;
		else
			eventList = node;
	}
}
//----------------------------------------------------------------------------------------------
//
void EventScheduler::CleanQueue()
{
	while(eventList)
	{
		EventNode * bucket = eventList;
		eventList = eventList->nextBucket;
		while(bucket)
		{
			EventNode * tmp = bucket;
			bucket = bucket->nextSibling;
			delete tmp;
		}
	}
	while(freeList)
	{
		EventNode * tmp = freeList;
		freeList = freeList->nextBucket;
		delete tmp;
	}
}
//----------------------------------------------------------------------------------------------
//
void EventScheduler::UpdateQueue()
{
	U32 updateCount = MGlobals::GetUpdateCount();
	while(eventList && eventList->updateTime <= updateCount)
	{
		EventNode * search = eventList;
		eventList = eventList->nextBucket;
		while(search)
		{
			EventNode * tmp = search;
			search = search->nextSibling;
			executeEvent(tmp);
			tmp->nextBucket = freeList;
			freeList = tmp;
		}
	}
}
//----------------------------------------------------------------------------------------------
//
void EventScheduler::executeEvent(EventNode * event)
{
	switch(event->eventType)
	{
	case EventNode::ET_DAMAGE:
		{
			VOLPTR(IWeaponTarget) target = OBJLIST->FindObject(event->info.damageInfo.targetID);
			if(target)
			{
				target->ApplyAOEDamage(event->info.damageInfo.shooterID, event->info.damageInfo.amount);
			}
			else//this may be a fighter
			{
				//find the carier
				IBaseObject * obj = OBJLIST->FindObject(event->info.damageInfo.targetID&(~SUBORDID_MASK));
				if(obj)
				{
					VOLPTR(ILaunchOwner) launcher = obj;
					if(launcher)
					{
						target = launcher->FindChildTarget(event->info.damageInfo.targetID);
						if(target)
						{
							target->ApplyAOEDamage(event->info.damageInfo.shooterID, event->info.damageInfo.amount);
						}
					}
				}
			}
			break;
		}
	}
}
//----------------------------------------------------------------------------------------------
//
struct EventSchedulerComponent : GlobalComponent
{
	EventScheduler * ev;

	virtual void Startup (void)
	{
		SCHEDULER = ev = new DAComponent<EventScheduler>;
		AddToGlobalCleanupList((IDAComponent **) &SCHEDULER);
	}
	
	virtual void Initialize (void)
	{
	}
};

static EventSchedulerComponent rventSchedulerComponent;

//--------------------------------------------------------------------------//
//----------------------------END EventScheduler.cpp------------------------//
//--------------------------------------------------------------------------//

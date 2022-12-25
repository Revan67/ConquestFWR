// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "IdleNotification.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
IdleNotification* IdleNotification::mFirstNode = 0;
// --------------------------------------------------------------------------
IdleNotification::IdleNotification(const NotificationHandler handler)
:mHandler(handler)
{
	mNextNode = mFirstNode;
	mFirstNode = this;
}
// --------------------------------------------------------------------------
IdleNotification::~IdleNotification()
{
	if(mFirstNode == this)
	{	mFirstNode = mNextNode;
		mNextNode = 0;
	}
	else
	{	// Find the previous node in the list
		IdleNotification*	currNode = mFirstNode;

		while(currNode->mNextNode != 0)
		{	if(currNode->mNextNode == this)
			{	currNode->mNextNode = mNextNode;
				mNextNode = 0;
				return;
			}
		}

		ASSERT(0);	// This node was not in the list!
	}
}
// --------------------------------------------------------------------------
void IdleNotification::Notify()
{
	if(mFirstNode)
	{	mFirstNode->NotifyObjects();
	}
}
// --------------------------------------------------------------------------
void IdleNotification::NotifyObjects() const
{
	if(mHandler)
	{	mHandler();
	}

	if(mNextNode)
	{	mNextNode->NotifyObjects();
	}
}
// --------------------------------------------------------------------------

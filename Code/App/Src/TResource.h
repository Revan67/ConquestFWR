#ifndef TRESOURCE_H
#define TRESOURCE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                TResource.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Header: /Conquest/App/Src/TResource.h 3     9/21/99 5:26p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef IRESOURCE_H
#include "IResource.h"
#endif

#ifndef TCONNPOINT_H
#include <TConnPoint.h>
#endif


//--------------------------------------------------------------------------
//  

template <class Type, class Base=IResource> 
struct DACOM_NO_VTABLE Resource : public Base
{
	IResourceClient * owner;		// current owner of the resource
	U32 priority;
	bool bNotifyNeeded;
	ConnectionPoint<Type,IResourceClient> point;
	
	BEGIN_DACOM_MAP_OUTBOUND(Type)
	DACOM_INTERFACE_ENTRY_AGGREGATE("IResourceClient", point)
	END_DACOM_MAP()

	//-------------------------------------

	Resource (void) : point (0)
	{
		owner = 0;
		priority = 0;
		bNotifyNeeded = false;
	}

	~Resource (void);
	
	DEFMETHOD_(BOOL32,GetOwnership) (struct IResourceClient *res, U32 priority=RES_PRIORITY_MEDIUM);

	DEFMETHOD_(BOOL32,ReleaseOwnership) (struct IResourceClient *res);

	DEFMETHOD(SetDefaultState) (void);		// set to state when there are no owners

	BOOL32 moveToFront (struct IResourceClient * res);

	Type * getBase (void)
	{
		Type * base = (Type *) ( ((U32)this) - ((U32)
					(static_cast<Resource<Type,Base>*>((Type*)8) )
					-8) );

		return base;
	}

	void updateResource (void);
};

//--------------------------------------------------------------------------//
//
template <class Type, class Base> 
BOOL32 Resource<Type, Base>::GetOwnership (struct IResourceClient *res, U32 _priority)
{
	//trying it without the equals for a while if you don't mind....
	//EXTREMELY DANGEROUS - BEING USED WHILE LIST IS BEING TRAVERSED
	if (res==0 || (_priority > priority && moveToFront(res)))
	{
		if (owner && owner != res)
			if (owner->Notify(this, RESMSG_RES_LOST) == 0)
				return 0;		// failed, client won't give up control
		priority = _priority;
		owner = res;
		if (owner == 0)
		{
			priority = 0;
			getBase()->OnNoOwner();
		}

		return 1;
	}
	else
	{
		if (res == owner && res != 0)
			return 1;
	}

	return 0;
}
//--------------------------------------------------------------------------//
//
template <class Type, class Base> 
BOOL32 Resource<Type, Base>::ReleaseOwnership (struct IResourceClient *res)
{
	if (res == owner)
	{
		owner = 0;
		priority = 0;
		bNotifyNeeded = true;
		return 1;
	}

	return 0;
}
//--------------------------------------------------------------------------//
//
template <class Type, class Base> 
GENRESULT Resource<Type, Base>::SetDefaultState (void)
{
	getBase()->OnNoOwner();
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
template <class Type, class Base> 
BOOL32 Resource<Type, Base>::moveToFront (struct IResourceClient * res)
{
	CONNECTION_NODE<IResourceClient> *node = point.pClientList, *prev=0;

	if (node)
	{
		if (node->client == res)
			return 1;
		prev = node;
		while ((node = node->pNext) != 0)
		{
			if (node->client == res)
			{
				prev->pNext = node->pNext;
				node->pNext = point.pClientList;
				point.pClientList = node;
				return 1;
			}
			prev = node;
		}
	}

	return 0;
}
//--------------------------------------------------------------------------//
//
template <class Type, class Base> 
void Resource<Type, Base>::updateResource (void)
{
	if (bNotifyNeeded)
	{
		CONNECTION_NODE<IResourceClient> *node = point.pClientList;
		bNotifyNeeded = false;

		CONNECTION_NODE<IResourceClient> *trueNext = point.pClientList;

		while (node && owner==0)
		{
			//ABSOLUTELY NECESSARY TO AVOID INFINITE LIST WALKING
			trueNext = node->pNext;
			node->client->Notify(this, RESMSG_RES_UNOWNED);
			node = trueNext;
		}

		if (owner == 0)
			getBase()->OnNoOwner();
	}
}
//--------------------------------------------------------------------------//
//
template <class Type, class Base> 
Resource<Type, Base>::~Resource (void)
{
	CONNECTION_NODE<IResourceClient> *node = point.pClientList, *next;

	while (node)
	{
		next = node->pNext;
		node->client->Notify(this, RESMSG_RES_CLOSING);
		node = next;
	}
}
//--------------------------------------------------------------------------//
//-----------------------------End TResource.h------------------------------//
//--------------------------------------------------------------------------//
#endif

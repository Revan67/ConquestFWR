#ifndef TCONNCONTAINER_H
#define TCONNCONTAINER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               TConnContainer.h                           //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $
*/			    
//--------------------------------------------------------------------------//


#ifndef ICONNECTION_H
#include "IConnection.h"
#endif

#ifndef TCOMPONENT_H
#include "TComponent.h"
#endif


//-----------------------------------------

template <class Type> struct DACOM_NO_VTABLE ConnectionPointContainer : public IDAConnectionPointContainer
{
	DEFMETHOD(FindConnectionPoint) (const C8 *connectionName, struct IDAConnectionPoint **connPoint);

	DEFMETHOD_(BOOL32,EnumerateConnectionPoints) (CONNCONTAINER_ENUM_PROC proc, void *context=0);
};

//-----------------------------------------
//
template <class Type>
GENRESULT ConnectionPointContainer< Type >::FindConnectionPoint (const C8 *connectionName, struct IDAConnectionPoint **connPoint)
{
	int i;
	const _DACOM_INTMAP_ENTRY *array = Type::_GetEntriesOut();

	for (i = 0; array[i].interface_name; i++)
	{
		IDAConnectionPoint *point;

		point = (IDAConnectionPoint *) ( ((U32)this) + array[i].offset - ((U32)
					(static_cast<ConnectionPointContainer<Type>*>((Type*)8) )
					-8) );

		if (strcmp(array[i].interface_name, connectionName) == 0)
		{
			*connPoint = point;
			point->AddRef();
			return GR_OK;
		}
	}

	*connPoint = 0;
	return GR_INTERFACE_UNSUPPORTED;
}
//-----------------------------------------
//
template <class Type>
BOOL32 ConnectionPointContainer< Type >::EnumerateConnectionPoints (CONNCONTAINER_ENUM_PROC proc, void *context)
{
	BOOL32 result=1;
	const _DACOM_INTMAP_ENTRY *array = Type::_GetEntriesOut();
	int i;
	
	for (i = 0; array[i].interface_name && result; i++)
	{
		IDAConnectionPoint *point;

		point = (IDAConnectionPoint *) ( ((U32)this) + array[i].offset - ((U32)
					(static_cast<ConnectionPointContainer<Type>*>((Type*)8) )
					-8) );
		
		result = proc(this, point, context);
	}


	return result;
}


//--------------------------------------------------------------------------//
//------------------------------End TConnContainer.h------------------------//
//--------------------------------------------------------------------------//
#endif
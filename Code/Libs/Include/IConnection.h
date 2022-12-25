#ifndef ICONNECTION_H
#define ICONNECTION_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               IConnection.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $
*/			    
//--------------------------------------------------------------------------//


#ifndef DACOM_H
#include "DACOM.h"
#endif


//-----------------------------------------
// return TRUE to continue the enumeration
//-----------------------------------------

typedef BOOL32  (__stdcall * CONNECTION_ENUM_PROC) (struct IDAConnectionPoint * connPoint, struct IDAComponent *client, void *context);

typedef BOOL32  (__stdcall * CONNCONTAINER_ENUM_PROC) (struct IDAConnectionPointContainer * container, struct IDAConnectionPoint *connPoint, void *context);

//-----------------------------------------

#define IID_IDAConnectionPoint MAKE_IID("IDAConnectionPoint",1)

struct DACOM_NO_VTABLE IDAConnectionPoint : public IDAComponent
{
	DEFMETHOD_(U32,GetOutgoingInterface) (C8 *interfaceName, U32 bufferLength) = 0;

	DEFMETHOD(GetContainer) (IDAConnectionPointContainer **container) = 0;

	DEFMETHOD(Advise) (IDAComponent *component, U32 *handle) = 0;

	DEFMETHOD(Unadvise) (U32 handle) = 0;

	DEFMETHOD_(BOOL32,EnumerateConnections) (CONNECTION_ENUM_PROC proc, void *context=0) = 0;
};

//-----------------------------------------

#define IID_IDAConnectionPointContainer MAKE_IID("IDAConnectionPointContainer",1)

struct DACOM_NO_VTABLE IDAConnectionPointContainer : public IDAComponent
{
	DEFMETHOD(FindConnectionPoint) (const C8 *connectionName, struct IDAConnectionPoint **connPoint) = 0;

	DEFMETHOD_(BOOL32,EnumerateConnectionPoints) (CONNCONTAINER_ENUM_PROC proc, void *context=0) = 0;
};

//-----------------------------------------

inline GENRESULT IDAComponent::QueryOutgoingInterface (const C8 *connectionName, struct IDAConnectionPoint **connection)
{
	struct IDAConnectionPointContainer *container;
	GENRESULT result;

	if ((result = QueryInterface( IID_IDAConnectionPointContainer, (void **)&container)) == GR_OK)
	{
		result = container->FindConnectionPoint(connectionName, connection);
		container->Release();
	}

	return result;
}


//-----------------------------------------------------------------------------//
//----------------------------End IConnection.h--------------------------------//
//-----------------------------------------------------------------------------//

#endif

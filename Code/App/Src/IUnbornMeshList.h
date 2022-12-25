#ifndef IUNBORNMESHLIST_H
#define IUNBORNMESHLIST_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IUnbornMeshList.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IUnbornMeshList.h 7     10/09/00 11:43a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef DACOM_H
#include <DACOM.h>
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IUnbornMeshList : IDAComponent
{
	virtual void RenderMeshAt(TRANSFORM trans,U32 archID,U32 playerID,U8 red = 255,U8 green = 255, U8 blue = 255, U8 alpha = 255) = 0;

	virtual void Render() = 0;

	virtual void Unload() = 0;

	virtual bool IsFabBuilding(U32 dwMissionfID,U32 archID) = 0;

	virtual void GetBoundingSphere(TRANSFORM trans,U32 archID,float & obj_rad,Vector &wcenter) = 0;

	virtual MeshChain * GetMeshChain(const TRANSFORM &trans,U32 archID) = 0;

	virtual bool IsFullGrid (U32 archID) = 0;
};

//---------------------------------------------------------------------------
//--------------------------END IUnbornMeshList.h----------------------------
//---------------------------------------------------------------------------
#endif

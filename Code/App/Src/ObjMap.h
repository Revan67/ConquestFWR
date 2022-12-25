//--------------------------------------------------------------------------//
//                                                                          //
//                               ObjMap.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ObjMap.h 12    10/03/00 4:14p Rmarr $
*/			    
//---------------------------------------------------------------------------
#ifndef OBJMAP_H
#define OBJMAP_H

#define GRIDSIZE 4096				// (also defined in GridVector.h)
#define MAX_SYS_SIZE GRIDSIZE*64//0x1FFFF   ( also defined in DSector.h )

#define SQUARE_SIZE 8192
#define EEP (MAX_SYS_SIZE/SQUARE_SIZE+1)
#define MAX_MAP_REF_ARRAY_SIZE (EEP)*(EEP)

#define OM_AIR			0x00000001
#define OM_UNTOUCHABLE  0x00000002
#define OM_TARGETABLE	0x00000004
#define OM_RESERVED_SHADOW 0xFF000000
#define OM_SHADOW		0x00000008
#define OM_SYSMAP_FIRSTPASS 0x00000010
#define OM_MIMIC		0x00000020
#define OM_EXPLOSION	0x00000040

struct ObjMapNode
{
	IBaseObject *obj;
	U32 dwMissionID;
	U32 flags;
	ObjMapNode *next;
};

struct DACOM_NO_VTABLE IObjMap : IDAComponent
{
	virtual ObjMapNode * AddObjectToMap(IBaseObject *obj,U32 systemID,int square_id,U32 _flags=0) = 0;

	virtual void RemoveObjectFromMap(IBaseObject *obj,U32 systemID,int square_id) = 0;

	virtual int GetMapSquare(U32 systemID, const Vector &pos) = 0;

	virtual int GetSquaresNearPoint(U32 systemID,const Vector &pos,int radius,int *ref_array)=0;

	virtual ObjMapNode *GetNodeList(U32 systemID,int square_id)=0;

	virtual U32 GetApparentPlayerID(ObjMapNode *node,U32 allyMask)=0;

	virtual bool IsObjectInMap(IBaseObject *obj) = 0;

	virtual void Init (void) = 0;

	virtual void Uninit (void) = 0;
};

#endif
//----------------------------------------------------------------------------
//---------------------------END ObjMap.h-------------------------------------
//----------------------------------------------------------------------------
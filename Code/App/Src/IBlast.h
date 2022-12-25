#ifndef IBLAST_H
#define IBLAST_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IBlast.h                                    //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IBlast.h 12    4/20/00 1:34p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//typedef S32 INSTANCE_INDEX;

struct _NO_VTABLE ICloakEffect : IObject
{
	virtual void Init(IBaseObject *_owner);
};

struct _NO_VTABLE IFireball : IObject
{
//	virtual BOOL32 InitFireball ( IBaseObject *_owner,const class TRANSFORM & orientation,SINGLE _animScale,SINGLE lifeTime) = 0;
	
	virtual void SetSpread(Vector _grow1,Vector _grow2,U8 _spreadFactor) = 0;
};

struct _NO_VTABLE IEffect : IObject
{
	virtual BOOL32 InitEffect ( IBaseObject *_owner,const class TRANSFORM & orientation,SINGLE _animScale,SINGLE lifeTime) = 0;

	virtual BOOL32 EditorInitEffect ( IBaseObject *_owner,SINGLE lifeTime) = 0;

	virtual SINGLE GetRadius () = 0;
};
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IBlast : IObject 
{
	virtual BOOL32 InitBlast (const class TRANSFORM & orientation, U32 systemID, IBaseObject *owner,SINGLE _animScale=1.0,SINGLE activationTime=0) = 0;

	virtual void SetRelativeTransform (const class TRANSFORM &_trans) = 0;

	virtual void GetFireball(OBJPTR<IBaseObject> & pInterface) = 0;

	virtual void SetVisible (bool _bVisible) = 0;

};
//---------------------------------------------------------------------------
//-------------------------END IBlast.h--------------------------------------
//---------------------------------------------------------------------------
#endif

#ifndef IBUILDSHIP_H
#define IBUILDSHIP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IBuild.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IBuildShip.h 16    8/18/00 3:55p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct IBuildShipFactory
{
	virtual void GetBuilderShips (struct IBuildShip **ships,int numShips,PARCHETYPE _pArchetype,IBaseObject * _owner) = 0;

	virtual void ReleaseShips (const IBuildShip * firstShip) = 0;
};

struct BUILDSHIP_INIT
{
	IBuildShipFactory *factory;

	virtual ~BUILDSHIP_INIT()
	{}
};

struct _NO_VTABLE IBuildShip : IObject
{
	virtual void InitBuildShip (IBaseObject * owner) = 0;

	virtual void Return() = 0;

	virtual void SetTransform (const TRANSFORM & trans)=0;

	virtual void SetPosition (const Vector & pos)=0;

	// updates position, rotation of root object. "dt" is in seconds
	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual void SetStartHardpoint (HardpointInfo  _hardpointinfo, INSTANCE_INDEX _dockIndex) = 0;
};

struct _NO_VTABLE IShuttle : IBuildShip
{
	virtual void FadeBuildShip(bool bExplode = true) = 0;

	virtual BOOL IsFadeComplete() = 0;

	virtual void GoAway()=0;
	
	virtual void SeekShip(IBaseObject * targ) = 0;

	virtual void BuildAtPos(Vector pos, Vector dir) = 0;

//	virtual void WorkAtShipPos(IBaseObject * targ, Vector pos, Vector dir,SINGLE offSet = 0) = 0;

	virtual bool AtTarget() = 0;

	//this is in here just for repair, and I don't like it
	virtual void WorkAtShipPos(IBaseObject * targ, Vector pos, Vector dir,SINGLE offSet = 0) = 0;

//	virtual void SetTransform (const TRANSFORM &trans)=0;

//	virtual void SetPosition (const Vector &pos)=0;

	virtual void GotoPosition (const Vector & pos) =0;

	virtual void SetSystemID (U32 _systemID) =0;
};

struct _NO_VTABLE ISpiderDrone : IBuildShip
{
	virtual void ScuttleTo(const Vector & pos,bool bOverride=0) = 0;

	virtual void SpinTo(const Vector & pos) = 0;

	virtual void IdleAt(const Vector & pos) = 0;
};

struct _NO_VTABLE ITerranDrone : IBuildShip
{
	virtual void BuildAtPos(const Vector & pos, const Vector &dir) = 0;

	virtual void IdleAtPos (const Vector & pos) = 0;
};
//---------------------------------------------------------------------------
//-----------------------END IBuildShip.h------------------------------------
//---------------------------------------------------------------------------
#endif

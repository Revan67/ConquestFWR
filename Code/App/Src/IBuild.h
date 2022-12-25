#ifndef IBUILD_H
#define IBUILD_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IBuild.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IBuild.h 28    9/07/00 6:35p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef IFABRICATOR_H
#include "IFabricator.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IBuild : IObject 
{
	virtual void Build (IFabricator *fab) = 0;

	virtual void AddFabToBuild (IFabricator * fab) = 0;

	virtual void RemoveFabFromBuild (IFabricator * fab) = 0;

	virtual U32 NumFabOnBuild () = 0;

//	virtual U32 GetPrimaryBuilder () = 0;

	virtual void EndBuild (bool bAborting) = 0;

	virtual void CancelBuild () = 0;

	virtual void HaltBuild () = 0;

//	virtual void SetBuildPause (bool _pause,SINGLE _percent) = 0;

	virtual void EnableCompletion () = 0;

	virtual SINGLE GetBuildProgress (U32 & stallType) = 0;

	virtual SINGLE GetBuildDisplayProgress (U32 & stallType) = 0;

	//for banker stalling
//	virtual ResourceCost GetAmountNeeded() = 0;
	
//	virtual void UnstallSpender() = 0;

	virtual bool IsReversing () = 0;

	virtual bool IsComplete () = 0;

	virtual bool IsPaused () = 0;

	virtual void SetProcessID (U32 processID) = 0;

	virtual U32 GetProcessID () = 0;

	virtual void SetBuilderType (U32 builderType) = 0;

	virtual S32 GetBuilderType () = 0;

	virtual void BeginDismantle(IBaseObject * _fab) = 0;
};

struct IHarvestBuilder : IObject
{
	virtual enum HARVEST_STANCE GetHarvestStance() = 0;

	virtual void SetHarvestStance(enum HARVEST_STANCE hStance) = 0;
};

struct IBuildEffect : IBaseObject
{
	//upgrades only
	virtual void SetupMesh (IBaseObject *fab,struct IMeshInfoTree *mesh_info,SINGLE _totalTime) = 0;

	//ships
	virtual void SetupMesh (IBaseObject *fab,IBaseObject *obj,struct IMeshInfoTree *mesh_info,INSTANCE_INDEX _instanceIndex,SINGLE _totalTime) = 0;

//	virtual void SetupMesh (IBaseObject *fab,IBaseObject *obj,SINGLE _totalTime) = 0;

	virtual void SetBuildPercent (SINGLE newPercent) = 0;

	//hint for visuals
	virtual void SetBuildRate (SINGLE percentPerSecond) = 0;

	virtual void AddFabricator (IFabricator *_fab) = 0;

	virtual void RemoveFabricator (IFabricator *_fab) = 0;

	virtual void SynchBuilderShips () = 0;

	virtual void Done () = 0; //effect data will be disposed of

	virtual void PrepareForExplode () = 0;

	virtual void PauseBuildEffect (bool bPause) = 0;
};

//---------------------------------------------------------------------------
//-----------------------END IBuild.h---------------------------------------
//---------------------------------------------------------------------------
#endif

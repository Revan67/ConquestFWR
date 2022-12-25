#ifndef MINEFIELD_H
#define MINEFIELD_H

//--------------------------------------------------------------------------//
//                                                                          //
//                             Minefield.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MineField.h 22    9/19/00 9:02p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef FIELD_H
#include "Field.h"
#endif

#ifndef DMINEFIELD_H
#include "DMineField.h"
#endif

#ifndef TOBJMISSION_H
#include "TObjMission.h"
#endif

#ifndef TOBJFRAME_H
#include "TObjFrame.h"
#endif

#ifndef TOBJTRANS_H
#include "TObjTrans.h"
#endif

#ifndef ANIM2D_H
#include "anim2d.h"
#endif

#ifndef TERRAINMAP_H
#include "TerrainMap.h"
#endif

#ifndef IMINEFIELD_H
#include "IMinefield.h"
#endif

#ifndef IWEAPON_H
#include "IWeapon.h"
#endif

//this determins the maximum number of ships that can be hurt durring each get sync data
#define MINEFIELD_MAXUPDATE_SIZE 10 


//--------------------------------------------------------------------------//

struct Mine
{
	AnimInstance regAnim;

	Vector velocity;
	Vector mineCenter;

	U8 fade;
	bool bActive:1;
	bool bFadeIn:1;
	bool bFadeOut:1;

	void * operator new (size_t size)
	{
		return calloc(size,1);
	}
	void operator delete (void * ptr)
	{
		::free(ptr);
	}
	void * operator new [] (size_t size)
	{
		return calloc(size,1);
	}
	void operator delete [] (void * ptr)
	{
		::free(ptr);
	}

	void SetFade(U32 _fade = 256)
	{
		if(_fade != 256)
			fade = _fade;
		regAnim.SetColor(fade,fade,fade,fade);
	}
};

struct Minefield : ObjectMission<
									ObjectFrame<IField,MINEFIELD_SAVELOAD,struct MinefieldArchetype>
												   >,
								IWeaponTarget, ISaveLoad, BASE_MINEFIELD_SAVELOAD,
								IMinefield,ITerrainSegCallback
{	
	BEGIN_MAP_INBOUND(Minefield)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IMinefield)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IField)
	END_MAP()

	GeneralSyncNode  genSyncNode;
	ExplodeNode		 explodeNode;

	MinefieldArchetype * archetype;

	Mine * mines;

	void * operator new (size_t size)
	{
		return calloc(size,1);
	}
	void operator delete (void * ptr)
	{
		::free(ptr);
	}

	U8 fade;
	U16 netHullPoints;
	S32 map_square;

	Minefield();
	~Minefield();

	// IBaseObject

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void RenderBackground (void)
	{
	};

	virtual void Render (void);

	virtual void MapRender (bool bPing);

	virtual void View (void);

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);	// set bVisible if possible for any part of object to appear

	virtual void CastVisibleArea();

	virtual GRIDVECTOR GetGridPosition() const
	{
		return gridPos;
	}

	// ISaveLoad 

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file); 

	virtual void ResolveAssociations (){}

	//ITerrainSegCallback

	virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos);		// return false to stop callback


	//IMissionActor

	// IWeaponTarget

	// apply damage from the area
	virtual void ApplyAOEDamage (U32 ownerID, U32 damageAmount);
	
	// ownerID is the partID of the unit responsible for the damage. e.g. The carrier is the owner, even though
	// the actual shooter may be the fighter.
	// returns true if shield was up
	virtual BOOL32 ApplyDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 shieldHit=1);

	virtual BOOL32 ApplyVisualDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 shieldHit=1);

	virtual BOOL32 GetCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction);

	virtual BOOL32 GetModelCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction);

	virtual void AttachBlast(PARCHETYPE pBlast,const Vector &pos,const Vector &dir);

	virtual void AttachBlast(PARCHETYPE pBlast,const Transform & baseTrans);

	//IMinefield

	virtual void InitMineField(GRIDVECTOR _gridPos, U32 _systemID);

	virtual void AddLayerRef();

	virtual U32 GetLayerRef();

	virtual void SubLayerRef();

	virtual void AddMine(Vector & pos,Vector & velocity);

	virtual void SetMineNumber(U32 mineNumber);

	virtual U32 GetMineNumber()
	{
		return hullPoints;
	}

	virtual U32 MaxMineNumber ();

	virtual BOOL32 AtPosition(GRIDVECTOR _gridPos);

	//Minefield

	virtual BOOL32 Setup ();//XYCoord *_squares,U32 _numSquares);

	virtual BOOL32 Init (HANDLE hArchetype);

	virtual BOOL32 ObjInField (IBaseObject *obj);

	virtual Vector GetCenterPos (void);

	void initMission ();
	
	bool hitObject(IBaseObject * obj);

	static HANDLE CreateArchetype (PARCHETYPE pArchetype, OBJCLASS objClass, void *data);

	static Minefield * CreateInstance (HANDLE hArchetype);

	void setTerrainFootprint (struct ITerrainMap * terrainMap);
	void unsetTerrainFootprint (struct ITerrainMap * terrainMap);

	U32 getSyncData (void * buffer);
	void putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery);

	void explodeMines (bool bExplode);
};

//------------------------------------------------------------------------------//
//--------------------------------End Minefield.h-------------------------------//
//------------------------------------------------------------------------------//

#endif
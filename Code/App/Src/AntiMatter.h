#ifndef ANTIMATTER_H
#define ANTIMATTER_H

//--------------------------------------------------------------------------//
//                                                                          //
//                              AntiMatter.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/AntiMatter.h 21    9/09/00 1:07a Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef FIELD_H
#include "Field.h"
#endif

#ifndef DFIELD_H
#include "DField.h"
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

#ifndef GRIDVECTOR_H
#include "GridVector.h"
#endif

#ifndef TMANAGER_H
#include "TManager.h"
#endif

struct SegmentPt
{
	GRIDVECTOR gv;
	//GRIDVECTOR gv1;
};

struct AMPts
{
	S32 x,y;
	Vector n;
};

struct SoftwareAntiField
{
	Vector centerPos;
	U8 dirBits;
	U8 fogRender;
};

struct AntiMatterArchetype : FieldArchetype<BT_ANTIMATTER_DATA>
{
	U32 textureID,modTexID;
	U32 mapTex;
	U32 softwareTexClearID,softwareTexFogID;
	S32 mouseX,mouseY;
	GRIDVECTOR lastPt;
	SegmentPt seg[MAX_SEGS+1];
	S32 numSegPts;
	bool bLaying;
	int gridX,gridY;

	AntiMatterArchetype ()
	{
		textureID = 0;
	}

	~AntiMatterArchetype (void)
	{
		TMANAGER->ReleaseTextureRef(textureID);
		textureID=0;
		TMANAGER->ReleaseTextureRef(modTexID);
		modTexID=0;

		if(softwareTexClearID)
		{
			TMANAGER->ReleaseTextureRef(softwareTexClearID);
			softwareTexClearID = 0;
		}
		if(softwareTexFogID)
		{
			TMANAGER->ReleaseTextureRef(softwareTexFogID);
			softwareTexFogID = 0;
		}
	}

	virtual void Notify(U32 message, void *param);

	virtual void EndEdit();

	virtual void Edit();

	virtual void RenderEdit();

	virtual void AntiMatterArchetype::SetUpPosition(S32 * arrX, S32 * arrY, U32 numPoints);

};

struct AntiMatter : ObjectMission<
									ObjectTransform<
										ObjectFrame<IField,ANTIMATTER_SAVELOAD,AntiMatterArchetype>
												   >
								>, 
								ISaveLoad, IQuickSaveLoad
{	
	BEGIN_MAP_INBOUND(AntiMatter)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IField)
	END_MAP()

	AntiMatterArchetype * arch;
	SINGLE * seg_lengths;
	SegmentPt *seg;
	S32 numSegPts;
	Vector *norms;
	RECT bounds;
	//SINGLE rad;
	//Vector roundPt0,roundPt1;
	AMPts *points;
	S32 numPts;
	SINGLE roll;
	U32 multiStages;

	//terrain map
	GRIDVECTOR gvec[MAX_SEGS+1];

	SoftwareAntiField * softwareNeb;

	AntiMatter();
	~AntiMatter();

	// IBaseObject

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void RenderBackground (void);

	virtual void Render (void);

	virtual void MapTerrainRender ();

	virtual void View (void);

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);	// set bVisible if possible for any part of object to appear

	virtual SINGLE TestHighlight (const RECT & rect);
	// ISaveLoad 

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file); 

	virtual void ResolveAssociations (){}

	/* IQuickSaveLoad methods */
	
	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void)
	{
	}

	//Terrain?? 

	virtual bool GetObjectBox (OBJBOX & box) const;

	// INavHazard
	
//	virtual bool IntersectFootprint(const Vector &pos, const Vector &dest, Vector &result);

//	virtual SINGLE GetRadius();

	// AntiMatter

	virtual BOOL32 Setup ();//struct XYCoord *_squares=0,U32 _numSquares=0);

	virtual BOOL32 Init (HANDLE hArchetype);

	virtual Vector GetCenterPos (void);

	virtual void SetFieldFlags(FIELDFLAGS &flags)
	{
		flags.bAntiMatter = TRUE;
	}

	void makePts();

	void unsetTerrainFootprint (struct ITerrainMap * terrainMap);
	void setTerrainFootprint (struct ITerrainMap * terrainMap);

	void setupSoftwareQuads(U32 i,U8 fogBits);

	void drawCournerQuad(U8 bits,Vector * vec);

	bool supportsSubtract (void);

};

struct AntiMatterArchetype *CreateAntiMatterArchetype(PARCHETYPE pArchetype,BT_ANTIMATTER_DATA *data);
struct AntiMatter *CreateAntiMatter(FieldArchetype<BASIC_DATA> *arch);

//------------------------------------------------------------------------------//
//-------------------------------End AntiMatter.h-------------------------------//
//------------------------------------------------------------------------------//

#endif
#ifndef FIELD_H
#define FIELD_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 Field.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Field.h 36    6/04/00 1:23p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef TOBJECT_H
#include "TObject.h"
#endif

#ifndef _INC_MALLOC
#include <malloc.h>
#endif

#ifndef DFIELD_H
#include "DField.h"
#endif

#define FIELD_TILE_SIZE	GRIDSIZE

/*enum FieldType
{
	FT_NEBULAE,
	FT_ASTEROID,
	FT_MINEFIELD
};*/

template <class Type> 
struct FieldArchetype
{
	Type *pData;
//	const char *name;
	PARCHETYPE pArchetype;
	S32 archIndex;
	IMeshArchetype * meshArch;
	
	void * operator new (size_t size)
	{
		return calloc(size,1);
	}
	void operator delete (void * ptr)
	{
		::free(ptr);
	}

	FieldArchetype (void)
	{
		meshArch = NULL;
		archIndex = -1;
	}

	virtual ~FieldArchetype() {}

	virtual void Notify(U32 message, void *param) = 0;

	virtual void EndEdit() = 0;

	virtual void Edit() = 0;

	virtual void RenderEdit() = 0;

	virtual void SetUpPosition(S32 * arrX, S32 * arrY, U32 numPoints) = 0;

};

#define ANCHOR_OFF -999

template <class Type>
struct DefaultArchetype : FieldArchetype<Type>
{
	//edit stuff
	PARCHETYPE nuggetType[4];

	XYCoord squares[MAX_SQUARES];
	U32 numSquares;
	BOOL32 laidSquare:1;
	BOOL32 snapping:1;
	S32 mouseX,mouseY;
	S32 snapX, snapY, anchorX, anchorY;
	U32 textureID;

	DefaultArchetype (void);

	~DefaultArchetype (void);

	virtual void Notify(U32 message, void *param);

	virtual void OnLButtonUp();

	virtual void OnLButtonDown();

	virtual void EndEdit();

	virtual void Edit();

	virtual void RenderEdit();

	virtual void SetUpPosition(S32 * arrX, S32 * arrY, U32 numPoints);
};

struct IField : IBaseObject
{
	char name[32];
	struct IFieldManager *factory;
	FIELDCLASS fieldType;
	U32 infoHelpID;
	IField * nextField;
	BOOL32 flag:1;
	struct FIELD_ATTRIBUTES attributes;

	//--- IBaseObject ---//
	
	virtual ~IField (void);	// find this in cloud.cpp

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)	// set bVisible if possible for any part of object to appear
	{
		bVisible = (GetSystemID() == currentSystem);
	}


	virtual SINGLE TestHighlight (const RECT & rect)	// set bHighlight if possible for any part of object to appear within rect
	{
		bHighlight = FALSE;
		return 0.0f;
	}

	//--- Field ---//

	virtual BOOL32 Setup () = 0;//struct XYCoord *_squares=0,U32 _numSquares=0) = 0;

	virtual BOOL32 Init (HANDLE hArchetype) = 0;

	virtual BOOL32 ObjInField (U32 systemID,const Vector &pos)
	{
		return FALSE;
	}

	virtual void SetSystemID (U32 _systemID) = 0;

	virtual void RenderBackground (void) = 0;

	virtual Vector GetCenterPos (void) = 0;

	virtual void SetFieldFlags (FIELDFLAGS &flags)
	{}

	virtual void PlaceBaseNuggets ()
	{};

	virtual void SetAmbientLight (const Vector &pos,const U8_RGB & old_amb)
	{}

//	virtual BOOL32 Init (struct BASIC_DATA *data,struct FieldArchetype *fieldArch) = 0;//AnimArchetype *animArch) = 0;
	//virtual void GetRegions(RECT **zone,U32 *numZones) {};
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct DACOM_NO_VTABLE IFieldManager: IDAComponent
{
	virtual void DeleteField (struct IField *field) = 0;

	virtual IField *GetFields () = 0;

	virtual IField *GetFieldContaining (IBaseObject *obj);

	virtual void RenderBackground () = 0;

//	virtual void SetShipBits () = 0;

	virtual void FieldHighlighted (struct IField * field);

	virtual void CreateField(char * fieldArchtype,S32 * xPos,S32 * yPos, U32 numberInList, U32 systemID) = 0;

	//obj is the pointer to the ship that is in the field
	virtual void SetAttributes(IBaseObject *obj,U32 dwFieldMissionID) = 0;

	virtual void AddFieldToList(IField * field) = 0;

	virtual void RemoveFieldFromList(IField * field) = 0;

	virtual void MapRenderFields(U32 systemID) = 0;

	virtual void CreateFieldBlast(IBaseObject * target,Vector pos,U32 systemID) = 0;
};
//---------------------------------------------------------------------------
//-------------------------END Field.h---------------------------------------
//---------------------------------------------------------------------------
#endif
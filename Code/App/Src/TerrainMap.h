#ifndef TERRAINMAP_H
#define TERRAINMAP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             TerrainMap.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TerrainMap.h 31    9/28/00 10:21a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

//--------------------------------------------------------------------------//
// flags for SetFootprint()
#define TERRAIN_IMPASSIBLE	 0x00000001
#define TERRAIN_BLOCKLOS     0x00000002
#define TERRAIN_FULLSQUARE   0x00000040
#define TERRAIN_HALFSQUARE   0x00000080
#define TERRAIN_PARKED		 0x00000100
#define TERRAIN_MOVING		 0x00000200
#define TERRAIN_DESTINATION  0x00000400
#define TERRAIN_FIELD		 0x00000800		// nebula, asteroid field, minefield
#define TERRAIN_UNITROTATING 0x00001000		// unit is rotating, so ignore out-of-system placements
#define TERRAIN_OUTOFSYSTEM  0x00004000
#define TERRAIN_WILLBEPLAT	 0x00020000      //footprint for where a platform will soon be
#define TERRAIN_REGION	     0x00040000      //footprint also represents a trigger region
//--------------------------------------------------------------------------//
// for internal use only
#define TERRAIN_TOPLEFT     0x00000004
#define TERRAIN_TOPRIGHT    0x00000008
#define TERRAIN_BOTTOMLEFT  0x00000010
#define TERRAIN_BOTTOMRIGHT 0x00000020
#define TERRAIN_FULLYTAKEN  0x00002000		// if a large ship is parked there
//--------------------------------------------------------------------------//
// flags for FindPath()
#define TERRAIN_FP_HALFSQUARE TERRAIN_HALFSQUARE
#define TERRAIN_FP_FULLSQUARE TERRAIN_FULLSQUARE
#define TERRAIN_FP_FINALPATH  0x00010000			// use all information for parking


struct GRIDVECTOR;
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ITerrainSegCallback		// test line seg
{
	virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos) = 0;		// return false to stop callback
};
//--------------------------------------------------------------------------//
//
struct IFindPathCallback
{
	virtual void SetPath (ITerrainMap * map, const GRIDVECTOR * const squares, int numSquares) = 0;
};
//--------------------------------------------------------------------------//
//
struct FootprintInfo
{
	U32 flags;
	SINGLE height;
	U32 missionID;

	FootprintInfo (void)
	{
		memset(this, 0, sizeof(*this));
	}
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ITerrainMap : IDAComponent
{
	virtual void SetWorldRect (const RECT & worldRect) = 0;

	virtual void SetFootprint (const GRIDVECTOR * const squares, int numSquares, const FootprintInfo & info) = 0;

	virtual void UndoFootprint (const GRIDVECTOR * const squares, int numSquares, const FootprintInfo & info) = 0;

	// enumerates all elements in a square, call with from == to to enumerate all elements of one square
	// returns false if the user's callback function stopped the enum early, else returns true.
	virtual bool TestSegment (const GRIDVECTOR & from, const GRIDVECTOR & to, ITerrainSegCallback * callback) = 0;

	virtual int FindPath (const GRIDVECTOR & from, const GRIDVECTOR & to, U32 dwMissionID, U32 flags, IFindPathCallback * callback) = 0;

	virtual void RenderEdit (void) = 0;	// want the terrain map to draw stuff in editor mode

	virtual bool IsGridEmpty (const GRIDVECTOR & grid, U32 dwIgnoreMissionID,  bool bFullSquare = true) = 0;

	virtual bool IsParkedAtGrid (const GRIDVECTOR & grid, U32 dwMissionID,  bool bFullSquare) = 0;

	virtual bool IsGridValid (const GRIDVECTOR & grid) = 0;

	virtual bool IsGridInSystem(const GRIDVECTOR & grid) = 0;

	virtual U32 GetFieldID (const GRIDVECTOR & grid) = 0;

	virtual bool IsOkForBuilding (const GRIDVECTOR & grid, bool checkParkedUnits, bool bFullSquare) = 0;
};

//---------------------------------------------------------------------------
//-----------------------END TerrainMap.h------------------------------------
//---------------------------------------------------------------------------
#endif

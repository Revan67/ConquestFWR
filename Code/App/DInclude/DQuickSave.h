#ifndef DQUICKSAVE_H
#define DQUICKSAVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DQuickSave.h                               //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DQuickSave.h 14    6/05/00 11:06a Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#define MAX_SYSTEMS 16
//----------------------------------------------------------------------------
//
struct MT_QSHIPLOAD
{
	SINGLE yaw;
	__hexview U32 dwMissionID;
	NETGRIDVECTOR pos;
};

struct MT_QJGATELOAD
{
	NETGRIDVECTOR pos;
	U32 gate_id,exit_gate_id;
	bool bJumpAllowed:1;
};

#define MAX_SQUARES 40

struct MT_QFIELDLOAD
{
	U32 systemID;
	U8 numSquares;
	GRIDVECTOR pos[MAX_SQUARES];
};

#define MAX_SEGS 12
#define MAX_SEGS_PLUS_ONE 13

struct MT_QANTIMATTERLOAD
{
	U32 systemID;
	U8 numSegments;
	GRIDVECTOR pts[MAX_SEGS_PLUS_ONE];
};

struct MT_QCAMERALOAD
{
	U32 systemID;
	Vector look;
	Vector at;
	SINGLE fovX;
	SINGLE fovY;
};

struct MT_QOBJGENERATOR_LOAD
{
	NETGRIDVECTOR position;
	SINGLE mean;
	SINGLE minDiff;
	SINGLE timer;
	SINGLE nextTime;
	char archTypeName[GT_PATH];
	__hexview U32 dwMissionID;
	bool bGenEnabled:1;
};

struct MT_PLANET_QLOAD
{
	NETGRIDVECTOR pos;
};

struct MT_TRIGGER_QLOAD
{
	NETGRIDVECTOR position;

	U32 triggerShipID;
	U32 triggerObjClassID;
	U32 triggerMObjClassID;
	U32 triggerPlayerID;
	SINGLE triggerRange;
	U32 triggerFlags;
	char progName[GT_PATH];
	bool bEnabled:1;
	bool bDetectOnlyReady:1;
};

struct MT_PLAYERBOMB_QLOAD
{
	__hexview U32 dwMissionID;
	NETGRIDVECTOR position;
	bool          bNoExplode;
};

struct MT_LIGHT_QLOAD
{
	U8 red,green,blue;
	S32 range;
	NETGRIDVECTOR position;
	Vector direction;
	SINGLE cutoff;
	bool bInfinite;
	char name[GT_PATH];
	bool bAmbient;
};

struct MT_SCRIPTOBJECT_QLOAD
{
	__hexview U32 dwMissionID;
	NETGRIDVECTOR position;
};

struct MT_WAYPOINT_QLOAD
{
	__hexview U32 dwMissionID;
	NETGRIDVECTOR position;
};

struct MT_NUGGET_QLOAD
{
	NETGRIDVECTOR position;
};

struct MT_PLATFORM_QLOAD
{
	__hexview U32 dwMissionID;
	NETGRIDVECTOR position;
};

struct MT_BLACKHOLE_QLOAD
{
	NETGRIDVECTOR pos;

	U8 targetSys[MAX_SYSTEMS];
	U8 numTargetSys;
};

struct MT_OBJECTFAMILY_QLOAD
{
	U32  numObjects;
	char name[GT_PATH];
};

struct MT_OBJECTFAMILYENTRY_QLOAD
{
	char scriptHandle[GT_PATH];
};

struct MT_FIELDLINK
{
	char scriptHandle[GT_PATH];
	char field[GT_PATH];
	char groupname[GT_PATH];
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
#endif

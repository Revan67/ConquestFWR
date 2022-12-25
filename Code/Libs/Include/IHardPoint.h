#ifndef _IHARDPOINT_H_
#define _IHARDPOINT_H_

//$Header: /Conquest/Libs/Include/IHardPoint.h 5     4/28/00 11:57p Rmarr $
//Copyright (c) 1997 Digital Anvil, Inc.

#include "Engine.h"

#ifndef DACOM_H
#include "DACOM.h"
#endif

#ifndef _3DMATH_H
#include "3DMath.h"
#endif

typedef S32 ARCHETYPE_INDEX;	// also defined in Engine.h

struct HardpointInfo
{
	JointType type;

	Vector point;
	Matrix orientation;

	//information related to different joint types

	Vector axis;	// Unit axis of rotation or translation in 'orientation.'
	float min0;
	float max0;		//minimum and maximum values, in radians if appropriate

	float spring_constant;
	float damping_constant;
	float rest_length;
};

typedef void (__cdecl * HARDPOINT_ENUM_CALLBACK)(const char* hardpoint_name, void* misc);

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define IHARDPOINT_VERSION 2
#define IID_IHardpoint MAKE_IID("IHardpoint", 2)

struct DACOM_NO_VTABLE IHardpoint : public IDAComponent
{
	virtual bool COMAPI retrieve_hardpoint_info (ARCHETYPE_INDEX arch, 
													const char* name, 
													HardpointInfo& result) const = 0;

	// if the hard point already exists it's values will get updated
	// if not a new hard point will be created
	virtual bool COMAPI set_hardpoint_info (ARCHETYPE_INDEX arch, 
													const char* name, 
													const HardpointInfo& hp) = 0;

	virtual void COMAPI enumerate_hardpoints (HARDPOINT_ENUM_CALLBACK, 
												ARCHETYPE_INDEX arch, 
												void* misc=0) const = 0;

	//child's hardpoint connection type determines connection type.
	//returns 0 on success otherwise:
	//-2: no implementation of connection API found.
	//-1: no IEngine provider found.
	//1: parent hardpoint name not found for instance archetype
	//2: child hardpoint name not found for instance archetype
	//3: call to IEngine::connect failed.
	virtual int COMAPI connect (INSTANCE_INDEX parent, 
								const char* parent_hardpoint,
								INSTANCE_INDEX child,
								const char* child_hardpoint) = 0;
};

#endif
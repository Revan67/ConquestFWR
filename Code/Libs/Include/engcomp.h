//****************************************************************************
//*                                                                          *
//*  ENGCOMP.H: DA COM game engine component interface                       *
//*                                                                          *
//*  $Header: /Conquest/Libs/Include/engcomp.h 5     4/28/00 11:57p Rmarr $
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1997 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#ifndef ENGCOMP_H
#define ENGCOMP_H

#ifndef DACOM_H
#include "DACOM.h"
#endif

#ifndef _INC_MEMORY
#include <memory.h>
#endif

#ifndef DAVARIANT_H
#include "davariant.h"
#endif

typedef S32 ARCHETYPE_INDEX;
typedef S32 INSTANCE_INDEX;

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define IENGINECOMPONENT_VERSION 1
#define IID_IEngineComponent MAKE_IID("IEngineComponent", 1)

// ..........................................................................
//
// IEngineComponent
//
// Components that create and manage data associated with engine archetypes
// and instances must implement this interface to get notifications of when
// to modify their local instance and archetype data.
//
struct IEngineComponent : public IAggregateComponent
{
	// create_archetype
	//
	// Allocate an component-specific archetype data and associate the data with the
	// archetype index arch_index, then load the archetype from the given filesys.
	//
	// This function may be called to ensure that subsequent create_instance() calls 
	// do not involve disk accesses or any other time-consuming operations.  
	//
	// arch_index can not be INVALID_ARCHETYPE_INDEX ( implementing components should ASSERT this )
	// filesys can not be NULL ( implementing components should ASSERT this )
	//
	// If the implementing component creates data specific to given arch_index 
	// (usually determined through IFileSystem chunks), this function returns TRUE.
	//
	// If the implementing component DOES NOT create data specific to given arch_index 
	// (usually determined through IFileSystem chunks), this function returns FALSE.
	//
	virtual BOOL32 COMAPI create_archetype( ARCHETYPE_INDEX arch_index, struct IFileSystem *filesys ) = 0;

	// duplicate_archetype
	//
	// Allocate new component-specific data and associate it with new_arch_index, then
	// copy the component-specific data associated with old_arch_index into new_arch_index.
	//
	virtual void COMAPI	duplicate_archetype( ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index ) = 0;

	// destroy_archetype
	//
	// Destroy component-specific data associated with the given arch_index.
	//
	// arch_index can not be INVALID_ARCHETYPE_INDEX ( implementing components should ASSERT this )
	//
	virtual void COMAPI destroy_archetype( ARCHETYPE_INDEX arch_index ) = 0;

	// query_archetype_interface
	//
	// Retrieves a particular interface from the given archetype.
	//
	// Note that there is not necessarily any correlation between whether
	// an archetype and an instance of that archetype respond to 
	// the same interface requests in the same manner.  i.e. just because
	// an instance implements a particular interface does not mean that
	// the archetype of that instance does.
	//
	virtual GENRESULT COMAPI query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif ) = 0;

	// create_instance
	//
	// Create component-specific instance data and associate it with the given inst_index
	//
	// inst_index can not be INVALID_INSTANCE_INDEX ( implementing components should ASSERT this )
	//
	// arch_index
	//
	// If the implementing component creates data specific to given inst_index 
	// this function returns TRUE.
	//
	// If the implementing component DOES NOT create data specific to given inst_index 
	// this function returns FALSE.
	//
	virtual BOOL32 COMAPI create_instance( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index ) = 0;

	// destroy_instance
	//
	// Destroy component-specific instance data associated with inst_index
	//
	// inst_index can not be INVALID_INSTANCE_INDEX ( implementing components should ASSERT this )
	//
	virtual void COMAPI destroy_instance( INSTANCE_INDEX inst_index ) = 0;

	// update_instance
	//
	// Update the simulation data for the given instance by the delta time specified.
	//
	virtual void COMAPI update_instance( INSTANCE_INDEX inst_index, SINGLE dt ) = 0;

	// render_instance
	//
	// Render the given instance using the given camera.
	// 
	// camera can not be NULL
	//
	// lod_fraction is in the range [0,1] 
	//
	// rf_flags is a combination of the RF_* flags defined in Engine.h.
	//
	// modifier_transform is applied to the instance before rendering the object
	// according to the rf_flags.
	//
	// Returns whether the object was visible in the given camera according to the
	// implementing component.
	//
	virtual enum vis_state COMAPI render_instance( struct ICamera *camera, INSTANCE_INDEX inst_index, float lod_fraction, U32 flags, const Transform *modifier_transform ) = 0;

	// query_instance_interface
	//
	// Retrieve a specific interface from the given instance.
	//
	// Note that there is not necessarily any correlation between whether
	// an archetype and an instance of that archetype respond to 
	// the same interface requests in the same manner.  i.e. just because
	// an instance implements a particular interface does not mean that
	// the archetype of that instance does.
	//
	virtual GENRESULT COMAPI query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif ) = 0;

	// update
	//
	// Update the simulation data for all instances by the delta time specified.
	//
	virtual void COMAPI update(SINGLE dt) = 0;
};

#endif

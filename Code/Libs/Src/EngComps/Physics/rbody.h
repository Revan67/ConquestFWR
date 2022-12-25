//
//
//

#ifndef RBODY_H
#define RBODY_H

//

#include "3dmath.h"
#include "stddat.h"
#include "geom.h"
#include "cmesh.h"
#include "DebugPrint.h"

#include "IRigidBody.h"
#include "IExtentContainer.h"

//

#include "extent.h"

//

struct RigidBody : public IRigidBody,
				   public IExtentContainer
{
	U32				DEBUG_tag;			// known value, for debugging
	S32				DEBUG_index;		// archeindex, for debugging
	SINGLE			mass;
	Matrix			Ibody;

	Vector			arm;	// Location of origin relative to center of mass.

	DynamicState	initial_dynamic_state;

// Collision and extent stuff.

	BaseExtent *	extents;

	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}
	void operator delete( void *ptr )
	{
		::free( ptr );
	}


	RigidBody(void)
	{
	// these are for debugging...
		DEBUG_tag = 0x12345678;
		DEBUG_index = -1;
	// These are all arbitrary.
		mass = 1.0;
		Ibody.set_identity();
		arm.zero();
		extents = NULL;

		initial_dynamic_state = DS_NONDYNAMIC;
	}

	void free (void)
	{
		mass = -1.0f;
		
		set_extents_tree( NULL );
	}

	~RigidBody(void)
	{
		DBGTRACE11("PHYSICS::~RigidBody #%d\n", DEBUG_index);
		free ();
	}

	void load(IFileSystem * file);
	BaseExtent * load_extent_tree(IFileSystem * file);
	void delete_extent_tree(BaseExtent * root);

	// IExtentContainer

	bool COMAPI get_extents_tree( BaseExtent **out_Extent ) 
	{
		if( (*out_Extent = extents) != NULL ) {
			return true;
		}

		return false;
	}

	//

	bool COMAPI set_extents_tree( BaseExtent *Extent ) 
	{
		if( extents ) {
			delete_extent_tree( extents );
			extents = NULL;
		}

		if( Extent ) {
			extents = Extent ;
		}

		return true;
	}	

	//

	// IRigidBody
	DynamicState COMAPI get_dynamic( void ) const 
	{
		return initial_dynamic_state;
	}

	//

	bool COMAPI set_dynamic( DynamicState dstate ) 
	{
		initial_dynamic_state = dstate;
		return true;
	}

	//

	SINGLE COMAPI get_mass( void ) 
	{
		return mass;
	}

	//

	bool COMAPI set_mass( SINGLE _mass ) 
	{
		mass = _mass;
		return true;
	}

	//

	Vector COMAPI get_center_of_mass_in_object( void ) 
	{
		return -arm;
	}

	//

	bool COMAPI set_center_of_mass_in_object( const Vector &center_o_mass ) 
	{
		arm = -center_o_mass;
		return true;
	}

	//

	const Matrix *COMAPI get_inertial_tensor( void ) 
	{
		return &Ibody;
	}

	//

	bool COMAPI set_inertial_tensor( const Matrix &_Ibody ) 
	{
		Ibody = _Ibody;
		return true;
	}

	// IDAComponent
	GENRESULT COMAPI QueryInterface( const char *iid, void **out_iif )
	{
		if( strcmp( iid, IID_IRigidBody ) == 0 ) {
			*out_iif = static_cast<IRigidBody*>(this);
			return GR_OK;
		}
		if( strcmp( iid, IID_IExtentContainer ) == 0 ) {
			*out_iif = static_cast<IExtentContainer*>(this);
			return GR_OK;
		}
		else if( strcmp( iid, IID_IDAComponent ) == 0 ) {
			*out_iif = static_cast<IRigidBody*>(this);
			return GR_OK;
		}

		return GR_GENERIC;
	}

	//

	U32 COMAPI AddRef( void )
	{
		// because these are managed components, we do not actually
		// refcount them, RigidBodyPhysics does
		return 1;
	}

	//

	U32 COMAPI Release( void )
	{
		// because these are managed components, we do not actually
		// refcount them, RigidBodyPhysics does
		return 1;
	}


};

//

typedef RigidBody Archetype;

//

#endif
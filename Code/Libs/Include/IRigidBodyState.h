// IRigidBodyState.h
//
//
//
//

#ifndef __IRigidBodyState_h__
#define __IRigidBodyState_h__

//

#include "DACOM.h"
#include "Vector.h"
#include "Matrix.h"

#include "IRigidBody.h"

//

#define IID_IRigidBodyState  MAKE_IID( "IRigidBodyState", 1 )

//

// ...........................................................................
//
// IRigidBodyState
//
// Accesses the dynamic state of a rigid body.
//
//
struct IRigidBodyState : public IRigidBody
{
	virtual Vector			COMAPI get_center_of_mass_in_world( void ) = 0;
	virtual bool			COMAPI set_center_of_mass_in_world( const Vector &center_o_mass ) = 0;

	virtual Vector			COMAPI get_velocity( void ) const = 0;
	virtual bool			COMAPI set_velocity( const Vector &velocity ) = 0;

	virtual Vector			COMAPI get_angular_velocity( void ) const = 0;
	virtual bool			COMAPI set_angular_velocity( const Vector &angular_velocity ) = 0;

	virtual Vector			COMAPI get_momentum( void ) const = 0;
	virtual Vector			COMAPI get_angular_momentum( void ) const = 0;
	
	//
	// Force/torque/impulse interface. Note that add_x_at_point() functions require
	// the point of force application expressed in world coordinates.
	//
	virtual bool			COMAPI add_impulse( const Vector &impulse ) = 0;
	virtual bool			COMAPI add_impulse_at_point( const Vector &impulse, const Vector &world_space_point ) = 0;
	virtual bool			COMAPI add_force( const Vector &force ) = 0;
	virtual bool			COMAPI add_force_at_point( const Vector &force, const Vector &world_space_point ) = 0;
	virtual bool			COMAPI add_torque( const Vector &torque ) = 0;
};



#endif // EOF


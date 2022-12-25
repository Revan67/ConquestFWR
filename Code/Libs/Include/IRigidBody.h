// IRigidBody.h
//
//
//
//

#ifndef __IRigidBody_h__
#define __IRigidBody_h__

//

#include "DACOM.h"
#include "Vector.h"
#include "Matrix.h"

#include "physics.h"

//

#define IID_IRigidBody  MAKE_IID( "IRigidBody", 1 )

//

// ...........................................................................
//
// IRigidBody
//
// Accesses the static (or near-static) parameters of a rigid body.
//
//
struct IRigidBody : public IDAComponent
{
	virtual DynamicState	COMAPI get_dynamic( void ) const = 0;
	virtual bool			COMAPI set_dynamic( DynamicState dstate ) = 0;

	virtual SINGLE			COMAPI get_mass( void ) = 0;
	virtual bool			COMAPI set_mass( SINGLE mass ) = 0;
							
	virtual Vector			COMAPI get_center_of_mass_in_object( void ) = 0;
	virtual bool			COMAPI set_center_of_mass_in_object( const Vector &center_o_mass ) = 0;

	virtual const Matrix *	COMAPI get_inertial_tensor( void ) = 0;
	virtual bool			COMAPI set_inertial_tensor( const Matrix &Ibody ) = 0;
};



#endif // EOF


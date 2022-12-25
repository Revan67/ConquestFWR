// IPhysicalCharacteristics
//
//
//

#ifndef IPhysicalCharacteristics_H
#define IPhysicalCharacteristics_H

#include "DACOM.h"
#include "DACOM_Utility.h"

static const char *IID_IPhysicalCharacteristics = "IPhysicalCharacteristics";

dacom_interface( IPhysicalCharacteristics )
{
	DACOM_INTERFACE_METHOD( SetMass,			( float mass));
	DACOM_INTERFACE_METHOD( GetMass,			( float *mass));
	
	DACOM_INTERFACE_METHOD( SetDynamic,			( DynamicState dstate));
	DACOM_INTERFACE_METHOD( GetDynamic,			( DynamicState dstate));
	
	DACOM_INTERFACE_METHOD( SetVelocity,		( Vector *lin_vel));
	DACOM_INTERFACE_METHOD( GetVelocity,		( Vector *lin_vel));
	
	DACOM_INTERFACE_METHOD( SetAngularVelocity,	( Vector *ang_vel));
	DACOM_INTERFACE_METHOD( GetAngularVelocity,	( Vector *ang_vel));
	
	DACOM_INTERFACE_METHOD( SetRegion,			( U32 region ));
	DACOM_INTERFACE_METHOD( GetRegion,			( U32 *region ));

//	DACOM_INTERFACE_METHOD( SetCenterOfMass,	( Vector *com_in_world));
	DACOM_INTERFACE_METHOD( GetCenterOfMass,	( Vector *com_in_world));
	
	DACOM_INTERFACE_METHOD( AddForce,			( Vector *force ));
	DACOM_INTERFACE_METHOD( AddForceAtPoint,	( Vector *force, Vector *point_in_world ));

	DACOM_INTERFACE_METHOD( AddTorque,			( Vector *torque ));

	DACOM_INTERFACE_METHOD( AddImpulse,			( Vector *impulse ));
	DACOM_INTERFACE_METHOD( AddImpulseAtPoint,	( Vector *impulse, Vector *point_in_world ));

};

#endif

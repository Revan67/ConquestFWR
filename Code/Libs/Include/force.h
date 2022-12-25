#ifndef FORCE_H
#define FORCE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               Force.h                                    //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Conquest/Libs/Include/force.h 5     4/28/00 11:57p Rmarr $
*/			    
//--------------------------------------------------------------------------//

//
// ForceElement class.
//
// Any forces that depend on an object's position, orientation, velocity, or angular
// velocity should be applied using a ForceElement. Physics will tend to evaluate these
// forces multiple times per render frame based on updated intermediate object states.
// 
// If you don't use a force element, but add the forces in your regular update loop,
// you will get large inaccuracies and possibly even instabilities since the force
// will be treated as constant over the timestep rather than varying as the object's
// position varies during the timestep. See B. Baldwin for further discussion if
// necesssary.
//
// The functions your ForceElement must provide are:
//
//  - bool affects_object(INSTANCE_INDEX object) - return true if the force element
//    affects the object in question, false if not.
//
//  - Vector compute_force(INSTANCE_INDEX object) - return the force element's 
//    force on the object in question. DO NOT CALL IPhysics->add_force() IN 
//    THIS FUNCTION. RETURN THE FORCE VECTOR INSTEAD.
//
// WARNING: DO NOT ASSUME IEngine->get_position() will return an object's correct
// INTERMEDIATE position unless and until you've returned true from affects_object()
// for the object in question. I do not inform the Engine of intermediate positions
// over the timestep unless absolutely necessary.
//
// NEW BEHAVIOR: ForceElement now contains a new member, "INSTANCE_INDEX object" which
// you can set if you want the ForceElement to act on a particular instance rather than
// globally. In this case, affects_object() is NOT CALLED and the ForceElement ALWAYS
// affects the instance in question.
//
// GLOBAL FORCE ELEMENTS MUST HAVE object == INVALID_INSTANCE_INDEX.
//



#ifndef US_TYPEDEFS
#include "typedefs.h"
#endif

typedef S32 INSTANCE_INDEX;			// also defined in Engine.h, ObjNode.h
class Vector;


class ForceElement
{
public:

	INSTANCE_INDEX	object;

	ForceElement(void) 
	{
		object = INVALID_INSTANCE_INDEX;
	}

	virtual ~ForceElement(void) {}

	virtual bool affects_object(INSTANCE_INDEX object) = 0;

// Callee fills in force and torque vectors for the object in question.
	virtual void compute_force_and_torque(INSTANCE_INDEX object, Vector & force, Vector & torque) = 0;
};

//

#endif
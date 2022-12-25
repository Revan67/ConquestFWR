#ifndef PHYSICS_H
#define PHYSICS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               Physics.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Conquest/Libs/Include/physics.h 5     4/28/00 11:57p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include "dacom.h"
#endif

#include "collision.h"

typedef S32 JOINT_INDEX;			// also defined in Engine.h
typedef S32 INSTANCE_INDEX;			// also defined in Engine.h, ObjNode.h
typedef S32 ARCHETYPE_INDEX;
class Vector;
typedef void * HANDLE;
class ForceElement;

//

typedef enum
{
//
// PS_USE_FORCES says whether or not to update linear and angular momentum 
// based on applied forces and torques. If DISABLED, all velocity changes
// must be controlled directly by the app; any forces and torques are ignored.
//
	PS_USE_FORCES,			// default: Enabled.

	PS_COLLISION_FRICTION,	// default: Disabled.
	PS_JOINT_DYNAMICS		// default: Disabled.
} PhyState;

//
// If the callback function returns true, normal collision response will be 
// computed. If it returns false, no physical collision response will occur.
// See collision.h for definition of CollisionData struct.
//
typedef bool (*CollisionCallback)(INSTANCE_INDEX, INSTANCE_INDEX, const CollisionData *);

struct IJointDriver
{
	virtual void drive( INSTANCE_INDEX parent_instance, INSTANCE_INDEX child_instance, const struct JointInfo * joint, float & force, float & torque ) = 0;
};


//
// State parameters for set_dynamic().
//
// DS_DYNAMIC -    physics moves object as appropriate, app can move 
//                 object any time.
//
// DS_NONDYNAMIC - physics never moves the object (forces ignored), but app
//                 may move it at any time.
//
// DS_FIXED -      nobody ever moves the object. Use with care, only when the
//                 object will really NEVER move. 
//
typedef enum
{
	DS_NONDYNAMIC,
	DS_DYNAMIC,
	DS_FIXED,
	DS_END = 0x7FFFFFF // make it 32-bit
} DynamicState;

struct PhyInstanceStats
{
	int	num_dynamic;
	int num_nondynamic;
	int num_fixed;
};

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define IPHYSICS_VERSION 2
#define IID_IPhysics MAKE_IID("IPhysics",2)

struct IPhysics : public IDAComponent
{
//
// Object state access functions.
//
	
//
// See comments for DynamicState, above.
//
	virtual DynamicState COMAPI get_dynamic (INSTANCE_INDEX object) const = 0;
	virtual void COMAPI set_dynamic (INSTANCE_INDEX object, DynamicState dynamic) = 0;

// 
// Object state information functions.
//
	
	virtual SINGLE COMAPI get_mass (INSTANCE_INDEX object) const = 0;
	virtual Vector COMAPI get_velocity (INSTANCE_INDEX object) const = 0;
	virtual Vector COMAPI get_momentum (INSTANCE_INDEX object) const = 0;
	virtual Vector COMAPI get_angular_velocity (INSTANCE_INDEX object) const = 0;
	virtual Vector COMAPI get_angular_momentum (INSTANCE_INDEX object) const = 0;

// This returns the center of mass in WORLD coordinates. This is not the same as the position of the object.
	virtual Vector COMAPI get_center_of_mass (INSTANCE_INDEX object) const = 0;

// This returns the LOCAL center of mass, in object space.
	virtual Vector COMAPI get_local_center_of_mass(INSTANCE_INDEX idx) const = 0;

	virtual const Matrix * COMAPI get_inertia_tensor(INSTANCE_INDEX idx) const = 0;
	
//
// Object state modification functions.
//

	virtual void COMAPI set_velocity (INSTANCE_INDEX object, const Vector & vel) = 0;
	virtual void COMAPI set_angular_velocity (INSTANCE_INDEX object, const Vector & ang) = 0;

	virtual void COMAPI set_mass (INSTANCE_INDEX object, SINGLE mass) = 0;
	virtual void COMAPI set_inertia_tensor (INSTANCE_INDEX object, const Matrix & tensor) = 0;
	virtual void COMAPI set_local_center_of_mass (INSTANCE_INDEX object, const Vector & center_of_mass) = 0;


//
// Force/torque/impulse interface. Note that add_x_at_point() functions require
// the point of force application expressed in world coordinates.
//
	virtual void COMAPI add_force (INSTANCE_INDEX object, const Vector & force) = 0;
	virtual void COMAPI add_force_at_point (INSTANCE_INDEX object, const Vector & force, const Vector & absolute) = 0;
	virtual void COMAPI add_torque (INSTANCE_INDEX object, const Vector & torque) = 0;
	
	virtual void COMAPI add_impulse (INSTANCE_INDEX object, const Vector & impulse) = 0;
	virtual void COMAPI add_impulse_at_point (INSTANCE_INDEX object, const Vector & impulse, const Vector & absolute) = 0;
	
//
// Give the IPhysics provider a pointer to a function to call when a collision
// occurs. NOTE that only one such function is supported.
//
	virtual void	COMAPI set_collision_callback (const CollisionCallback func) = 0;

//
// Any forces that depend on an object's state (pos, vel, orient, etc.), for 
// example Springs, Planetary Gravity, etc., must be applied using a 
// ForceElement or instabilities will result. See FORCE.H for further 
// explanation.
//
	virtual HANDLE	COMAPI add_force_element(ForceElement * fe) = 0;
	virtual void  	COMAPI remove_force_element(HANDLE fe_handle) = 0;

	virtual void	COMAPI compute_collision_response(const CollisionData & data, INSTANCE_INDEX obj1, INSTANCE_INDEX obj2) = 0;

	virtual void	COMAPI enable(PhyState state) = 0;
	virtual void	COMAPI disable(PhyState state) = 0;
	virtual bool	COMAPI is_enabled(PhyState state) = 0;

//
// NOTE: EXTENTS RETURNED ARE RELATIVE TO INSTANCE'S CENTER OF MASS. So if you use
// these extents in the ICollision functions, pass in IPhysics::get_center_of_mass() result
// instead of instance position.
//
	virtual bool	COMAPI get_extent(const BaseExtent ** extent, INSTANCE_INDEX object) = 0;

	virtual void	COMAPI set_joint_driver(JOINT_INDEX jidx, IJointDriver * jd) = 0;

//
// min_dt defaults to 1.0/10 seconds. This is a relatively large timestep, depending
// on what you're doing. For velocity-only updating, e.g. Conquest, this should be more
// than adequate. If using ForceElements to do springs, gravity wells, etc., you may need
// to crank it down to 1.0/30 or even smaller to remain stable.
//
// Also, if using PS_AUTO_COLLISION, you may need a pretty small timestep (1/30 or less)
// to avoid missed collisions.
//
	virtual void	COMAPI set_min_dt(float min_dt) = 0;
	virtual float	COMAPI get_min_dt(void) const = 0;

	virtual void COMAPI update_instance (INSTANCE_INDEX instance, SINGLE dt) = 0;

	virtual bool COMAPI get_archetype_extent(const BaseExtent ** extent, ARCHETYPE_INDEX arch) = 0;

//
// This version takes into account all objects in the hierarchy, but doesn't affect joint variables,
// i.e. it uses the inertia of the whole hierarchy, but treats all objects as fixed w.r.t. each other.
//
	virtual void	COMAPI compute_hierarchy_collision_response(const CollisionData & data, INSTANCE_INDEX obj1, INSTANCE_INDEX obj2) = 0;

	virtual void	COMAPI get_stats(PhyInstanceStats & stats) const = 0;

	virtual GENRESULT COMAPI get_mass (ARCHETYPE_INDEX, SINGLE&) const = 0;

	virtual bool    COMAPI is_valid (INSTANCE_INDEX instance) const = 0;
};

//----------------------------------------------------------------------------------
//------------------------END Physics.h---------------------------------------------
//----------------------------------------------------------------------------------

#endif
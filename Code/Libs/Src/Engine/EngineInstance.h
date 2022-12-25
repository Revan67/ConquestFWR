#ifndef __EngineInstance_h__
#define __EngineInstance_h__

//
#include <float.h>

#include "EngineArchetype.h"
#include "EngineInstanceJoint.h"

//

//

#define EI_F_DELETE_INSTANCE_HANDLER	(1<<1)

// ..........................................................................
// 
// EngineInstance
//
//
struct EngineInstance
{
public: // Types
	
public: // Data

	INSTANCE_INDEX	inst_index;			// The instance index of this according to the Engine.

	U32 ei_f_flags;						// state flags, value is combination of EI_F_* above

	// The interface to the instance data.  This can be implemented
	// via an internal implementation (LocalEngineInstance) or by the
	// IEngine client.
	//
	IEngineInstance *instance_handler;

	EngineArchetype *archetype;			// The archetype of this individual instance
	EngineArchetype *root_archetype;	// The archetype of the uber-compound archetype, may be NULL 
	
	EngineInstance *parent;				// The parent of this instance, NULL if this is the root
	EngineInstanceJointList children;	// List of the children of this instance

	// Client-specific user data.  This is completely opaque
	// and the value is just handed to and from the client.
	//
	S32	user_data;
		
	// cache data
	//
	float		bounding_sphere_radius;
	Vector		bounding_sphere_center;

	const char	*part_name;				// pointer to a part name if part of a compound instance; NULL otherwise

public: // Interface

	EngineInstance( INSTANCE_INDEX _inst_index = INVALID_INSTANCE_INDEX, 
					IEngineInstance *_instance_handler = NULL, 
					EngineArchetype *_archetype = NULL, 
					EngineArchetype *_root_archetype = NULL );
	EngineInstance( const EngineInstance &ei );
	~EngineInstance();
	EngineInstance &operator=( const EngineInstance &ei );
	bool operator==( const EngineInstance &ei );

	bool is_compound( void );
	
	bool add_child( EngineInstance *child, const JointInfo *joint_info );
	bool remove_child( EngineInstance *child );
	EngineInstanceJoint *get_descendant_joint( EngineInstance *child );
	EngineInstanceJoint *get_child_joint( EngineInstance *child );
	EngineInstanceJoint *get_child_joint_after( EngineInstance *child );
	
	bool set_dirty_bounding_sphere( bool dirty_self_also );
	bool get_bounding_sphere( bool recursive, float *out_radius, Vector *out_center );

	bool update_joint( const EngineInstanceJoint *joint );
	bool update_joint_derivatives( const EngineInstanceJoint *j );
};

//

EngineInstance::EngineInstance( INSTANCE_INDEX _inst_index, IEngineInstance *_instance_handler, EngineArchetype *_archetype, EngineArchetype *_root_archetype )
{
	ei_f_flags = 0;
	parent = NULL;
	instance_handler = _instance_handler;
	archetype = _archetype;
	root_archetype = _root_archetype;
	inst_index = _inst_index;
	user_data = 0;
	bounding_sphere_radius = -1.0f;
	part_name = NULL;
}

//

EngineInstance::EngineInstance( const EngineInstance &ei )
{
	operator=( ei );
}

//

EngineInstance::~EngineInstance( void )
{
	children.clear();	// TODO: do we need to do more?

#ifdef _DEBUG
	// invalid data items, for debugging! (jy)
	instance_handler = NULL;
	user_data = 0xCBCBCBCB;
	
	archetype = NULL;
	root_archetype = NULL;
#endif
}

//

EngineInstance &EngineInstance::operator=( const EngineInstance &ei )
{
	instance_handler = ei.instance_handler;
	archetype = ei.archetype;
	root_archetype = ei.root_archetype;
	inst_index = ei.inst_index;
	user_data = ei.user_data;

	parent = ei.parent;
	children = ei.children;

	bounding_sphere_radius = ei.bounding_sphere_radius;
	bounding_sphere_center = ei.bounding_sphere_center;

	ei_f_flags = ei.ei_f_flags;

	return *this;
}

//

bool EngineInstance::operator==( const EngineInstance &ei )
{
	return inst_index == ei.inst_index;
}

//

bool EngineInstance::is_compound( void )
{
	return (parent || children.size() > 0 );
}

//

EngineInstanceJoint *EngineInstance::get_descendant_joint( EngineInstance *child )
{
	EngineInstanceJointList::iterator c_beg = children.begin();
	EngineInstanceJointList::iterator c_end = children.end();
	EngineInstanceJointList::iterator c_kid;
	EngineInstanceJoint *joint;

	// check joints to our immediate children
	//
	for( c_kid = c_beg; c_kid != c_end; c_kid++ ) {
		if( (*c_kid).child_instance == child ) {
			return &(*c_kid);
		}
	}

	// have our children check their children 
	for( c_kid = c_beg; c_kid != c_end; c_kid++ ) {
		if( (joint = c_kid->child_instance->get_descendant_joint( child )) != NULL ) {
			return joint;
		}
	}

	return NULL;
}

//

EngineInstanceJoint *EngineInstance::get_child_joint( EngineInstance *child )
{
	if( child == NULL ) {
		return NULL;
	}

	EngineInstanceJointList::iterator c_beg = children.begin();
	EngineInstanceJointList::iterator c_end = children.end();
	EngineInstanceJointList::iterator c_kid;
	
	for( c_kid = c_beg; c_kid != c_end; c_kid++ ) {
		if( (*c_kid).child_instance == child ) {
			return &(*c_kid);
		}
	}

	return NULL;
}

//

EngineInstanceJoint *EngineInstance::get_child_joint_after( EngineInstance *child )
{
	EngineInstanceJointList::iterator c_beg = children.begin();
	EngineInstanceJointList::iterator c_end = children.end();
	EngineInstanceJointList::iterator c_kid;
	

	if( child == NULL ) {
		if( c_beg != c_end ) {
			return &(*c_beg);
		}
	}
	else {
		for( c_kid = c_beg; c_kid != c_end; c_kid++ ) {
			if( (*c_kid).child_instance == child ) {
				c_kid++;
				if( c_kid != c_end ) {
					return &(*c_kid);
				}
				else {
					return NULL;
				}
			}
		}
	}

	return NULL;
}

//

bool EngineInstance::remove_child( EngineInstance *child )
{
	EngineInstanceJointList::iterator c_beg = children.begin();
	EngineInstanceJointList::iterator c_end = children.end();
	EngineInstanceJointList::iterator c_kid;

	for( c_kid = c_beg; c_kid != c_end; c_kid++ ) {
		if( (*c_kid).child_instance == child ) {
			children.erase( c_kid );
			child->parent = NULL;
			return true;
		}
	}

	return false;
}

//
bool EngineInstance::add_child( EngineInstance *child, const JointInfo *joint_info )
{
	ASSERT( child->parent == NULL );

#if !defined(FINAL_RELEASE)
	// Check for cycles in our tree.
	if( child->get_descendant_joint( this ) ) {
		return false;
	}
#endif
	
	EngineInstanceJoint joint;

	joint.info = *joint_info;
	joint.child_instance = child;

	children.push_back( joint );

	child->parent = this;

	set_dirty_bounding_sphere( true );

	return true;
}

//

bool EngineInstance::set_dirty_bounding_sphere( bool dirty_self_also )
{
	EngineInstance *instance;

	if( dirty_self_also ) {
		instance = this;
	}
	else {
		instance = parent;
	}

	while( instance && instance->bounding_sphere_radius != -1.0f ) {
		instance->bounding_sphere_radius = -1.0f;
		instance = instance->parent;
	}

	return true;
}

//

bool EngineInstance::get_bounding_sphere( bool recursive, float *out_radius, Vector *out_center )
{
	// THIS FUNCTION ASSUMES THAT THE CURRENT STATE IS CORRECT
	// i.e. that any lazy evaluation has been updated to current.
	//

	ASSERT( instance_handler );

	EngineInstanceJointList::const_iterator c_beg = children.begin();
	EngineInstanceJointList::const_iterator c_end = children.end();
	if( !recursive ) {
		// return only this instance's sphere
		//
		instance_handler->get_centered_radius( inst_index, out_radius, out_center );
		return true;
	}

	if( bounding_sphere_radius > 0.0f) {
		// return cached value
		//
		*out_radius = bounding_sphere_radius;
		*out_center = bounding_sphere_center;
		return true;
	}

	// compute current value
	//
	
	float compound_radius, root_radius, c_radius;
	Vector compound_center, root_center, c_center;

	// get center and transform to world space
	//
	const Transform & root_trans = instance_handler->get_transform( inst_index );
	
	instance_handler->get_centered_radius( inst_index, &root_radius, &root_center );

	root_center = root_trans * root_center;
	
	float min_x = root_center.x - root_radius;
	float max_x = root_center.x + root_radius;
	float min_y = root_center.y - root_radius;
	float max_y = root_center.y + root_radius;
	float min_z = root_center.z - root_radius;
	float max_z = root_center.z + root_radius;
	
	EngineInstanceJointList::const_iterator c_kid;
	EngineInstance *child_instance;

	// find a good center
	//
	for( c_kid = c_beg; c_kid != c_end; c_kid++ ) {
		
		child_instance = (*c_kid).child_instance;
		ASSERT( child_instance && child_instance->instance_handler );
		
		// get center and transform to world space
		//
		const Transform & c_trans = child_instance->instance_handler->get_transform( child_instance->inst_index );

		child_instance->get_bounding_sphere( true, &c_radius, &c_center );
		
		c_center = c_trans * c_center;
		
		min_x = Tmin( min_x, c_center.x - c_radius );
		max_x = Tmax( max_x, c_center.x + c_radius );

		min_y = Tmin( min_y, c_center.y - c_radius );
		max_y = Tmax( max_y, c_center.y + c_radius );

		min_z = Tmin( min_z, c_center.z - c_radius );
		max_z = Tmax( max_z, c_center.z + c_radius );
	}

	compound_center = 0.5f * Vector( max_x + min_x, max_y + min_y, max_z + min_z );
	compound_radius = 0.5f * Tmax( Tmax( max_x - min_x, max_y - min_y ), max_z - min_z );

	// check root radius
	//
	float r_rad = (compound_center - root_center).magnitude() + root_radius;
	if( r_rad > compound_radius ) {
		compound_radius = r_rad;
	}

	// check child radii
	//
	for( c_kid = c_beg; c_kid != c_end; c_kid++ ) {
		
		child_instance = (*c_kid).child_instance;
		ASSERT( child_instance && child_instance->instance_handler );

		const Transform & c_trans = child_instance->instance_handler->get_transform( child_instance->inst_index );

		child_instance->get_bounding_sphere( true, &c_radius, &c_center );
	
		c_radius += ((c_trans * c_center) - compound_center).magnitude();

		if( c_radius > compound_radius ) {
			compound_radius = c_radius;
		}
	}

	// back to root local
	compound_center = compound_center * root_trans;

	// update cache
	*out_radius = bounding_sphere_radius = compound_radius;
	*out_center = bounding_sphere_center = compound_center;
	
	ASSERT( !_isnan( compound_radius ) );
	ASSERT( _finite( compound_radius ) );
	ASSERT( compound_radius >= 0.0f );

	return true;
}

bool EngineInstance::update_joint_derivatives( const EngineInstanceJoint *j )
{
	ASSERT( j );

	EngineInstance *child = j->child_instance;

	ASSERT( instance_handler != NULL );
	ASSERT( child->instance_handler != NULL );

	const Transform & pX = instance_handler->get_transform( inst_index );
	const Matrix & pR = pX;
	const Vector & pT = pX.translation;

	Transform cX ( pX );
	Matrix & cR = cX;
	Vector & cT = cX.translation;

	const Vector & pV = instance_handler->get_velocity( inst_index );
	const Vector & paV = instance_handler->get_angular_velocity( inst_index );

	// Compute base velocity of child's C.M. ignoring any joint motion.
	Vector r = cT - pT;
	Vector cV = pV + cross_product(paV, r);
	Vector caV = paV;

	switch( j->info.type ) {

	case JT_FIXED:
	{
		break;
	}

	case JT_REVOLUTE:
	{	
		Vector axis = pR * j->info.axis;
		Vector d = cR * j->info.child_point;

		// Angular velocity w = parent's w + joint rotation velocity.
		caV += axis * j->state.q_dot;

		// Velocity v = rvel + joint rotation coriolis term.
		// NOTE vector "d" is reversed from ABM definition, hence backwards cross product.
		cV += cross_product(d, axis) * j->state.q_dot;
		break;
	}

	case JT_PRISMATIC:
	{		
		Vector axis = pR * j->info.axis;

		// Velocity v = rvel + joint translation velocity.
		cV += axis * j->state.q_dot;
		break;
	}

	case JT_CYLINDRICAL:
	{
		Vector axis = pR * j->info.axis;
		Vector d = cR * j->info.child_point;

		caV += axis * j->state.r_dot;
		cV += cross_product(axis, d) * j->state.r_dot + axis * j->state.p_dot;
		break;
	}

	case JT_SPHERICAL:
	case JT_TRANSLATIONAL:
	case JT_LOOSE:
		// just propagate parent values assuming joint is not moving
		break;

	default:
		break;
	}

	
	child->instance_handler->set_velocity( child->inst_index, cV );
	child->instance_handler->set_angular_velocity( child->inst_index, caV );

	return true;
}

//

bool EngineInstance::update_joint( const EngineInstanceJoint *j )
{
	ASSERT( j );

	EngineInstance *child = j->child_instance;

	ASSERT( instance_handler != NULL );
	ASSERT( child->instance_handler != NULL );

	const Transform & pX = instance_handler->get_transform( inst_index );
	const Matrix & pR = pX;
	const Vector & pT = pX.translation;

	Transform cX ( pX );
	Matrix & cR = cX;
	Vector & cT = cX.translation;
	
	bool changed = true;

	switch( j->info.type ) {

	case JT_FIXED:
	{
		cR *= j->info.rel_orientation;
		cT += pR * j->info.rel_position;
		break;
	}

	case JT_REVOLUTE:
	{
		const Matrix Rrot ( Quaternion( j->info.axis, j->state.q) );
		cR *= Rrot * j->info.rel_orientation;
		cT += pR * j->info.parent_point - cR * j->info.child_point;
		break;
	}

	case JT_PRISMATIC:
	{
		cR *= j->info.rel_orientation;
		cT += pR * (j->info.parent_point + (j->info.axis * j->state.q)) - cR * j->info.child_point;
		break;
	}

	case JT_CYLINDRICAL:
	{
		// Combine revolute and prismatic along same axis.
		const Matrix Rrot ( Quaternion(j->info.axis, j->state.r) );
		cR *= j->info.rel_orientation * Rrot;
		cT += pR * (j->info.parent_point + (j->info.axis * j->state.p)) - cR * j->info.child_point;
		break;
	}

	case JT_SPHERICAL:
	{
		const Matrix Rrot ( Quaternion( j->state.w, j->state.x, j->state.y, j->state.z ) );
		cR *= Rrot * j->info.rel_orientation;
		cT += pR * j->info.parent_point - cR * j->info.child_point;
		break;
	}

	case JT_TRANSLATIONAL:
	{
		cR *= j->info.rel_orientation;
		cT += pR * (j->info.rel_position + Vector(j->state.px, j->state.py, j->state.pz));
		break;
	}

	case JT_LOOSE:
	{
		const Matrix Rrot ( Quaternion( j->state.w, j->state.x, j->state.y, j->state.z ) );
		cR *= Rrot * j->info.rel_orientation;
		cT += pR * (j->info.rel_position + Vector(j->state.px, j->state.py, j->state.pz));
		break;
	}

	default:
		changed = false;
		break;
	}

	if( changed ) {
		child->instance_handler->set_transform( child->inst_index, cX );
		set_dirty_bounding_sphere( true );
	}

	return true;
}

//

#endif // EOF

//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "filesys.h"
#include "instance.h"
#include "stddat.h"
#include "engine.h"
#include "rigid.h"

//

bool Instance::use_forces = true;

//

//Instance::Instance(RigidBody * arch, S32 idx)
Instance::Instance(RigidBodyPhysics * rbp, ARCHETYPE_INDEX arch_idx, S32 idx, DynamicState dyn_state)
{
//	owns_body = false;

	phy = rbp;

	init(arch_idx);
	index	= idx;

	state	= RB_MAX;

	if (mass < 1e-2)
	{
//		DebugPrint("Zero mass in physics. Making object non-dynamic.\n");
		dynamic = DS_NONDYNAMIC;
	}
	else
	{
		dynamic	= dyn_state;
	}

	x.zero();
	R.set_identity();
	q.set_identity();

	v.zero();
	p.zero();
	w.zero();
	L.zero();

	F_applied.zero();
	T_applied.zero();

	F_internal.zero();
	T_internal.zero();

	sphere_center_valid = box_center_valid = false;
	sphere_center.zero();
	box_center.zero();

	q_dot = q_dot_dot = 0.0f;
	parameter = 0;
}

//

//void Instance::init(RigidBody * body)
void Instance::init(ARCHETYPE_INDEX arch_idx)
{
	body_idx = arch_idx;

	RigidBody * body = phy->GetArchetype(arch_idx);
	if (body)
	{
		mass	= body->mass;
		Ibody	= body->Ibody;
		Iinv	= Ibody.get_inverse();
	}
}

//
/*
void Instance::sync_to_engine(struct IEngine * engine)
{
	set_orientation(engine->get_orientation(index));
	set_position(engine->get_position(index));
}
*/
//-------------------------------------------------------------------
// put data back in engine
void Instance::sync_data_to_engine (struct IEngine * engine)
{
	ASSERT(engine);

	Transform m = R;

	m.translation = get_position();
	engine->set_transform(index,m);
	engine->set_velocity(index,v);
	engine->set_angular_velocity(index,w);
}
//-------------------------------------------------------------------
// get data from engine
void Instance::sync_data_from_engine (struct IEngine * engine)
{
	ASSERT(engine);

	const Transform & m = engine->get_transform(index);
	Vector vec;
	bool bOrientChanged=false;

	vec = engine->get_velocity(index);
	if (memcmp(&vec, &v, sizeof(v)) != 0)		// if velocity has changed
		set_velocity(vec);

	if (memcmp(&m, &R, sizeof(R)) != 0)		// if orientation has changed
	{
		bOrientChanged=true;
		set_orientation(m);
	}
	set_position(m.translation);

	vec = engine->get_angular_velocity(index);
	if (bOrientChanged || memcmp(&vec, &w, sizeof(w)) != 0)		// if ang_velocity has changed
		set_angular_velocity(vec);
}

//

void Instance::sync_velocity_to_engine (struct IEngine * engine)
{
	ASSERT(engine);

	engine->set_velocity(index,v);
	engine->set_angular_velocity(index,w);
}

//

void Instance::get_state_vector(SINGLE * dst) const
{
	ASSERT(dst);

	switch (state)
	{
		case RB_MAX:
			*(dst++) = x.x;
			*(dst++) = x.y;
			*(dst++) = x.z;

			if (use_forces)
			{
				*(dst++) = p.x;
				*(dst++) = p.y;
				*(dst++) = p.z;
			}

			*(dst++) = q.w;
			*(dst++) = q.x;
			*(dst++) = q.y;
			*(dst++) = q.z;

			if (use_forces)
			{
				*(dst++) = L.x;
				*(dst++) = L.y;
				*(dst++) = L.z;
			}
			break;

		case RB_GEN:
			break;
	}
}

//

void Instance::set_state_vector(SINGLE * src)
{
	ASSERT(src);

	switch (state)
	{
		case RB_MAX:
			x.x = *(src++);
			x.y = *(src++);
			x.z = *(src++);

			if (use_forces)
			{
				p.x = *(src++);
				p.y = *(src++);
				p.z = *(src++);
			}

			q.w = *(src++);
			q.x = *(src++);
			q.y = *(src++);
			q.z = *(src++);
			q.normalize();

			if (use_forces)
			{
				L.x = *(src++);
				L.y = *(src++);
				L.z = *(src++);
			}
			break;

		case RB_GEN:
			break;
	}

	R = q;

	Matrix I = R * Ibody * R.get_transpose();
	Iinv = I.get_inverse();

	if (use_forces)
	{
		v = p / mass;
		w = Iinv * L;
	}
}

//

void Instance::get_derivative_vector(SINGLE * dst) const
{
	ASSERT(dst);

	switch (state)
	{
		case RB_MAX:
		{
			*(dst++) = v.x;
			*(dst++) = v.y;
			*(dst++) = v.z;

			if (use_forces)
			{
				*(dst++) = F_applied.x + F_internal.x;
				*(dst++) = F_applied.y + F_internal.y;
				*(dst++) = F_applied.z + F_internal.z;
			}

		//
		// qdot = 1/2 * w * q.
		//
			Quaternion qw(w);
			Quaternion qdot = qw * q / 2.0f;
			*(dst++) = qdot.w;
			*(dst++) = qdot.x;
			*(dst++) = qdot.y;
			*(dst++) = qdot.z;

			if (use_forces)
			{
				*(dst++) = T_applied.x + T_internal.x;
				*(dst++) = T_applied.y + T_internal.y;
				*(dst++) = T_applied.z + T_internal.z;
			}
			break;
		}
		case RB_GEN:
			break;
	}
}

//


void Instance::zero_applied_force_and_torque(void)
{
	F_applied.zero();
	T_applied.zero();
}

//

void Instance::zero_internal_force_and_torque(void)
{
	F_internal.zero();
	T_internal.zero();
}

//

void Instance::add_internal_force_at_point(const Vector & force, const Vector & point)
{
	F_internal += force;

// Compute torque introduced by force.
	Vector r = point - x;
	Vector torque = cross_product(r, force);
	T_internal += torque;
}

//

void Instance::set_position(const Vector & pos)
{
	RigidBody * body = phy->GetArchetype(body_idx);
	if (body)
	{
		x = pos - R * body->arm;
	}
}

//

Vector Instance::get_position(void) const
{
	RigidBody * body = phy->GetArchetype(body_idx);
	if (body)
	{
		return x + R * body->arm;
	}
	else
	{
		return x;
	}
}

//

Vector Instance::get_last_position(void) const
{
	RigidBody * body = phy->GetArchetype(body_idx);
	if (body)
	{
		return last_x + last_R * body->arm;
	}
	else
	{
		return last_x;
	}
}


//

void Instance::set_orientation(const Matrix & m)
{
	R = m;
	q.set(R);

	Matrix I = R * Ibody * R.get_transpose();
	Iinv = I.get_inverse();

	if (use_forces)
	{
		w = Iinv * L;
	}
}

//

void Instance::set_center_of_mass(const Vector & center)
{
	RigidBody * body = phy->GetArchetype(body_idx);
	if (body)
	{
		body->arm = -center;
	}
}

//

// IRigidBodyState
DynamicState COMAPI Instance::get_dynamic( void ) const 
{
	return dynamic;
}

//

bool COMAPI Instance::set_dynamic( DynamicState dstate )
{
	phy->set_dynamic( index, dstate );
	return true;
}

//

SINGLE COMAPI Instance::get_mass( void ) 
{
	return mass;
}

//

bool COMAPI Instance::set_mass( SINGLE new_mass ) 
{
	SINGLE old_mass = mass;
	mass = new_mass;

	Ibody *= new_mass / old_mass;

	Matrix I = R * Ibody * R.get_transpose();
	Iinv = I.get_inverse();

	p = v * mass;
	L = I * w;

	return true;
}

//

Vector COMAPI Instance::get_center_of_mass_in_object( void )
{
	return phy->get_local_center_of_mass( index );
}

//

bool COMAPI Instance::set_center_of_mass_in_object( const Vector &center_o_mass ) 
{
	phy->set_local_center_of_mass( index, center_o_mass );
	return true;
}

//

Vector COMAPI Instance::get_center_of_mass_in_world( void ) 
{
	return phy->get_center_of_mass( index );
}

//

bool COMAPI Instance::set_center_of_mass_in_world( const Vector &center_o_mass ) 
{
//	phy->set_center_of_mass( index, center_o_mass );
	ASSERT( 0 );
	return true;
}

//

const Matrix *COMAPI Instance::get_inertial_tensor( void ) 
{
	return &Ibody;
}

//
	
bool COMAPI Instance::set_inertial_tensor( const Matrix &tensor ) 
{
	Ibody = tensor;
	
	Matrix I = R * Ibody * R.get_transpose();
	Iinv = I.get_inverse();

	L = I * w;

	return true;
}

//

Vector COMAPI Instance::get_velocity( void ) const 
{
	return v;
}

//

bool COMAPI Instance::set_velocity( const Vector &vel ) 
{
	v = vel;
	p = vel * mass;

	return true;
}

//

Vector COMAPI Instance::get_angular_velocity( void ) const 
{
	return w;
}

//

bool COMAPI Instance::set_angular_velocity( const Vector &ang ) 
{
	w = ang;
	// Group things so that we have 3 matrix * vector operations instead of matrix * matrix.
	L = (R * (Ibody * (w * R)));

	return true;
}

//

Vector COMAPI Instance::get_momentum( void ) const 
{
	return p;
}

//

Vector COMAPI Instance::get_angular_momentum( void ) const 
{
	return L;
}

//

bool COMAPI Instance::add_impulse( const Vector &impulse ) 
{
	phy->add_impulse( index, impulse );
	return false;
}

//

bool COMAPI Instance::add_impulse_at_point( const Vector &impulse, const Vector &world_space_point ) 
{
	phy->add_impulse_at_point( index, impulse, world_space_point );
	return false;
}

//

bool COMAPI Instance::add_force( const Vector &force ) 
{
	phy->add_force( index, force );
	return false;
}

//

bool COMAPI Instance::add_force_at_point( const Vector &force, const Vector &world_space_point ) 
{
	phy->add_force_at_point( index, force, world_space_point );
	return false;
}

//

bool COMAPI Instance::add_torque( const Vector &torque ) 
{
	phy->add_torque( index, torque );
	return false;
}

//


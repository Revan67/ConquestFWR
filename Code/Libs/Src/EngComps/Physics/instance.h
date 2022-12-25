//
//
//

#ifndef INSTANCE_H
#define INSTANCE_H

//

#include "3dmath.h"
#include "physics.h"
#include "arch.h"
#include "engcomp.h"

#include "IRigidBodyState.h"

//

typedef enum
{
	RB_MAX,		// Maximal coordinates. Full 6 DOF object.
	RB_GEN		// Generalized coordinates. Usually(?) 1 DOF.
} RBCoord;

//

struct Instance : public IRigidBodyState
{
	RBCoord			state;

	DynamicState	dynamic;


//
// This stuff is basic state information for all rigid bodies.
//
	Vector		x;				// position.
	Matrix		R;				// orientation matrix.
	Quaternion	q;				// orientation quaternion.

	Vector		v;				// linear velocity.
	Vector		p;				// linear momentum.

	Vector		w;				// angular velocity.
	Vector		L;				// angular momentum.

	SINGLE		mass;

	Matrix		Ibody;			// Inertia tensor in body frame.
	Matrix		Iinv;			// Inverse inertia tensor in world frame.

	Vector		F_applied;		// Applied force and torque.
	Vector		T_applied;	

	Vector		F_internal;		// Force and torque resulting from connections
	Vector		T_internal;		// such as constraints and springs.

	Vector		last_x;			// Save previous positions for collision response purposes.
	Matrix		last_R;
	Vector		last_v;
	Vector		last_w;

	struct RigidBodyPhysics * phy;	// unfortunately we need an instance of RigidBodyPhysics to get at archetypes.

	ARCHETYPE_INDEX	body_idx;
	INSTANCE_INDEX	index;		// What's one more integer?

	float		q_dot;
	float		q_dot_dot;
	float		parameter;

	mutable bool	sphere_center_valid:1;
	mutable bool	box_center_valid:1;
	mutable Vector	sphere_center;
	mutable Vector 	box_center;

	static bool	use_forces;

	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}
	void operator delete( void *ptr )
	{
		free( ptr );
	}

	Instance(void)
	{
		state = RB_MAX;

		dynamic = DS_NONDYNAMIC;
		x.zero();
		R.set_identity();
		q.set_identity();
		v.zero();
		p.zero();
		w.zero();
		L.zero();
		mass = 1;
		Ibody.set_identity();
		Iinv = Ibody;
		F_applied.zero();
		T_applied.zero();
		F_internal.zero();
		T_internal.zero();

		phy = NULL;
		body_idx = -1;
//		index = -1;

		last_x.zero();
		last_R.set_identity();
		last_v.zero();
		last_w.zero();

		sphere_center_valid = 
		box_center_valid = false;

		box_center.zero();
		sphere_center.zero();

		q_dot = q_dot_dot = 0;
	}

	//Instance(RigidBody * arch, INSTANCE_INDEX idx);
	Instance(RigidBodyPhysics * phy, ARCHETYPE_INDEX arch_idx, INSTANCE_INDEX idx, DynamicState dyn_state);
	~Instance(void)
	{
	}


//	void init(RigidBody * rb);
	void init(ARCHETYPE_INDEX arch_idx);

	int	get_state_length(void) const
	{
		if (use_forces)
		{
		// 3 vectors and a quaternion for maximal coordinates, a joint variable and
		// its velocity for generalized coordinates.
			return ((state == RB_MAX) ? 3 * 3 + 4 : 2);
		}
		else
		{
			return 3 + 4;
		}
	}

	void	get_state_vector(SINGLE * dst) const;
	void	set_state_vector(SINGLE * src);
	void	get_derivative_vector(SINGLE * dst) const;

	void	zero_applied_force_and_torque(void);
	void	zero_internal_force_and_torque(void);

	void	add_internal_force_at_point(const Vector & force, const Vector & point);

	void		set_position(const Vector & pos);
	Vector		get_position(void) const;
	Vector		get_last_position(void) const;
	void		set_orientation(const Matrix & m);

	void sync_data_to_engine (struct IEngine * engine);		// put data back in engine
	void sync_data_from_engine (struct IEngine * engine);	// get data from engine
	void sync_velocity_to_engine (struct IEngine * engine); // put velocities into engine

	inline Quaternion get_orientation(void) const
	{
		return q;
	}

	inline Matrix get_rotation_matrix(void) const
	{
		return R;
	}

	inline void set_momentum(const Vector & mom)
	{
		p = mom;
		v = p / mass;
	}

	inline void set_angular_momentum(const Vector & ang_mom)
	{
		L = ang_mom;
		w = Iinv * L;
	}

	//

	void set_center_of_mass(const Vector & center);

	//

	// IDAComponent
	GENRESULT COMAPI QueryInterface( const char *iid, void **out_iif )
	{
		if( strcmp( iid, IID_IRigidBodyState ) == 0 ) {
			*out_iif = static_cast<IRigidBodyState*>(this);
			return GR_OK;
		}
		else if( strcmp( iid, IID_IDAComponent ) == 0 ) {
			*out_iif = static_cast<IRigidBodyState*>(this);
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


	// IRigidBodyState
	DynamicState	COMAPI get_dynamic( void ) const ;
	bool			COMAPI set_dynamic( DynamicState dstate ) ;
	SINGLE			COMAPI get_mass( void ) ;
	bool			COMAPI set_mass( SINGLE mass ) ;
	Vector			COMAPI get_center_of_mass_in_object( void ) ;
	bool			COMAPI set_center_of_mass_in_object( const Vector &center_o_mass ) ;
	const Matrix *	COMAPI get_inertial_tensor( void ) ;
	bool			COMAPI set_inertial_tensor( const Matrix &Ibody ) ;
	Vector			COMAPI get_center_of_mass_in_world( void ) ;
	bool			COMAPI set_center_of_mass_in_world( const Vector &center_o_mass ) ;
	Vector			COMAPI get_velocity( void ) const ;
	bool			COMAPI set_velocity( const Vector &velocity ) ;
	Vector			COMAPI get_angular_velocity( void ) const ;
	bool			COMAPI set_angular_velocity( const Vector &angular_velocity ) ;
	Vector			COMAPI get_momentum( void ) const ;
	Vector			COMAPI get_angular_momentum( void ) const ;
	bool			COMAPI add_impulse( const Vector &impulse ) ;
	bool			COMAPI add_impulse_at_point( const Vector &impulse, const Vector &world_space_point ) ;
	bool			COMAPI add_force( const Vector &force ) ;
	bool			COMAPI add_force_at_point( const Vector &force, const Vector &world_space_point ) ;
	bool			COMAPI add_torque( const Vector &torque ) ;
};

//

#endif

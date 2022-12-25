//
//
//

#ifndef RIGID_H
#define RIGID_H

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

//

#include "dacom.h"
#include "tcomponent.h"
#include "SysConsumerDesc.h"
#include "engcomp.h"
#include "engine.h"
#include "force.h"
#include "physics.h"

//

#include "handlemap.h"

//

#include "ode.h"
#include "instance.h"
#include "llist.h"
#include "spatial.h"

//

#define VERBOSE

//

typedef LinkedList<ForceElement *> ForceElementList;
typedef LinkedList<INSTANCE_INDEX> InstanceList;

//

struct Contact
{
	Instance *	a;
	Instance *	b;

	Vector		p;	// contact point.
	Vector		N;	// contact normal.

	Vector compute_Ndot(void) const
	{
	// THIS IS PROBABLY TOO SIMPLISTIC:
		Vector result = (b) ? cross_product(b->w, N) : Vector(0, 0, 0);
		return result;
	}
};

//

struct DACOM_NO_VTABLE RigidBodyPhysics : public IEngineComponent, 
										  public IPhysics, 
										  public IODE
{
	BEGIN_DACOM_MAP_INBOUND(RigidBodyPhysics)
	DACOM_INTERFACE_ENTRY(IPhysics)
	DACOM_INTERFACE_ENTRY(IODE)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY(IEngineComponent)
	DACOM_INTERFACE_ENTRY2(IID_IPhysics,IPhysics)
	DACOM_INTERFACE_ENTRY2(IID_IODE,IODE)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IEngineComponent,IEngineComponent)
	END_DACOM_MAP()

//
// Implementation-specific stuff.
//
	IEngine *	engine;
	IODESolver *solver;
	SINGLE		time;

	CollisionCallback	collision_func;

	InstanceList		dynamic_list;

	typedef inst_handlemap< Instance* >		inst_map;
	typedef arch_handlemap< Archetype* >	arch_map;
	
	mutable inst_map Instances;
	mutable arch_map Archetypes;

	float min_dt;

	DynamicState default_dyn_state;

	ForceElementList	forces;

	bool				use_forces;
	bool				collision_friction;
	bool				joint_dynamics;

	static int			buffer_length;
	static float *		buffer1;
	static float *		buffer2;

	static CollisionStats	collision_stats;

	int num_dynamic;
	int num_nondynamic;
	int num_fixed;

	DynamicArray< TPointer<IJointDriver> >	joint_drivers;

	SpatialMatrix		coll2body;
	SpatialMatrix		body2coll;
	struct ABMLink *	coll_root;
	struct ABMLink *	coll_link;

protected: // Interface

// ABM stuff.
	void compute_link_velocities(INSTANCE_INDEX root);
	void init_links(INSTANCE_INDEX root);
	void compute_ABM_dynamics(Instance * root);
	bool compute_ABM_collision_matrix(Matrix & K, Instance * root, Instance * body, const Matrix & Rcoll, const Vector & rcoll, int index);
	void impulse_response(struct ABMLink * root, struct ABMLink * k, const SpatialVector & pcoll);
	void propagate_impulse(Instance * root, Instance * obj, const Vector & p, const Matrix & Rcoll, const Vector & pcoll, int index);

	virtual void COMAPI set_joint_driver(JOINT_INDEX jidx, IJointDriver * jd);

// contact stuff.
	void compute_contact_forces(const struct Contact * contacts, int n);

// General integration stuff.
	void apply_forces_to_tree(INSTANCE_INDEX root, const Vector & f, const Vector & t);
	void compute_internal_forces(void);
	void compute_motion(SINGLE time_step);


	inline IDAComponent * get_base(void)
	{
		return (IEngineComponent *) this;
	}

public: // Interface

	RigidBodyPhysics (void) 
	{
		engine = NULL;
		solver = NULL;
		collision_func = NULL;
	}

	~RigidBodyPhysics(void);

	DA_HEAP_DEFINE_NEW_OPERATOR(RigidBodyPhysics);

	GENRESULT init(SYSCONSUMERDESC * info);

	// IAggregateComponent
	virtual GENRESULT	COMAPI Initialize(void);

	// IEngineComponent
	BOOL32 COMAPI create_archetype( ARCHETYPE_INDEX arch_index, struct IFileSystem *filesys ) ;
	void COMAPI	duplicate_archetype( ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index ) ;
	void COMAPI destroy_archetype( ARCHETYPE_INDEX arch_index ) ;
	GENRESULT COMAPI query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif ) ;
	BOOL32 COMAPI create_instance( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index ) ;
	void COMAPI destroy_instance( INSTANCE_INDEX inst_index ) ;
	void COMAPI update_instance( INSTANCE_INDEX inst_index, SINGLE dt ) ;
	enum vis_state COMAPI render_instance( struct ICamera *camera, INSTANCE_INDEX inst_index, float lod_fraction, U32 flags, const Transform *modifier_transform ) ;
	GENRESULT COMAPI query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif ) ;
	void COMAPI update(SINGLE dt) ;

	// IPhysics methods.
	DynamicState COMAPI get_dynamic(INSTANCE_INDEX object) const;
	void COMAPI set_dynamic(INSTANCE_INDEX object, DynamicState dynamic);
	SINGLE COMAPI get_mass(INSTANCE_INDEX object) const;
	Vector COMAPI get_center_of_mass(INSTANCE_INDEX object) const;
	Vector COMAPI get_velocity(INSTANCE_INDEX object) const;
	Vector COMAPI get_momentum(INSTANCE_INDEX object) const;
	Vector COMAPI get_angular_velocity(INSTANCE_INDEX object) const;
	Vector COMAPI get_angular_momentum(INSTANCE_INDEX object) const;
	void COMAPI set_mass(INSTANCE_INDEX object, SINGLE mass);
	void COMAPI set_inertia_tensor(INSTANCE_INDEX object, const Matrix & tensor);
	void COMAPI set_local_center_of_mass (INSTANCE_INDEX object, const Vector & center_of_mass);
	void COMAPI set_velocity(INSTANCE_INDEX object, const Vector & vel);
	void COMAPI set_angular_velocity(INSTANCE_INDEX object, const Vector & ang);
	void COMAPI add_force(INSTANCE_INDEX object, const Vector & force);
	void COMAPI add_force_at_point(INSTANCE_INDEX object, const Vector & force, const Vector & absolute);
	void COMAPI add_torque(INSTANCE_INDEX object, const Vector & torque);
	void COMAPI add_impulse(INSTANCE_INDEX object, const Vector & impulse);
	void COMAPI add_impulse_at_point(INSTANCE_INDEX object, const Vector & impulse, const Vector & absolute);
	void	COMAPI set_collision_callback(const CollisionCallback func);
	HANDLE	COMAPI add_force_element(ForceElement * fe);
	void  	COMAPI remove_force_element(HANDLE fe_handle);
	void	COMAPI compute_collision_response(const CollisionData & data, INSTANCE_INDEX obj1, INSTANCE_INDEX obj2);
	void	COMAPI compute_hierarchy_collision_response(const CollisionData & data, INSTANCE_INDEX obj1, INSTANCE_INDEX obj2);
	void	COMAPI enable(PhyState state);
	void	COMAPI disable(PhyState state);
	bool	COMAPI is_enabled(PhyState state);
	bool	COMAPI get_extent(const BaseExtent ** extent, INSTANCE_INDEX object);
	GENRESULT COMAPI get_mass (ARCHETYPE_INDEX, SINGLE&) const;
    const Matrix * COMAPI get_inertia_tensor(INSTANCE_INDEX idx) const;
	Vector COMAPI get_local_center_of_mass(INSTANCE_INDEX idx) const;
	bool COMAPI get_archetype_extent(const BaseExtent ** extent, ARCHETYPE_INDEX arch);
	void	COMAPI get_stats(PhyInstanceStats & stats) const;
	bool COMAPI is_valid (INSTANCE_INDEX idx) const;


	// IODE methods.
	S32		get_y_length(void);
	void	get_y(SINGLE * dst, SINGLE t);
	void	get_dydt(SINGLE * dst, SINGLE * y, SINGLE t);
	void	set_y(SINGLE * src);
	SINGLE	get_t(void) const;
	void	set_t(SINGLE time);

	void	compute_child_velocity( INSTANCE_INDEX joint_index );

	// ICollision methods.
/*	bool COMAPI intersect_ray_with_extent(	Vector & point_of_intersection, Vector & normal,
											const Vector & ray_origin, const Vector & ray_direction, 
											const BaseExtent & object, 
											const Transform & obj_transform);

	bool COMAPI intersect_ray_with_extent_hierarchy(	
											Vector & point_of_intersection, Vector & normal,
											const Vector & ray_origin, const Vector & ray_direction, 
											const BaseExtent & object, 
											const Transform & obj_transform, bool find_closest);

	void COMAPI get_collision_stats(CollisionStats & stats);

	// OLD-STYLE INTERSECTION-AND-CONTACT functions.
	virtual bool COMAPI collide_extents(	CollisionData & data, 
											const BaseExtent * extent1, const Transform & T1,
											const BaseExtent * extent2, const Transform & T2);

	virtual bool COMAPI recurse_collide_extents(CollisionData & data, 
												const BaseExtent * e1, const Transform & T1,
												const BaseExtent * e2, const Transform & T2);

	virtual bool COMAPI collide_extent_hierarchies(	CollisionData & data, 
													const BaseExtent * root1, const Transform & T1,
													const BaseExtent * root2, const Transform & T2);

	// NEW INTERSECTION-ONLY functions.
	virtual bool COMAPI collide_extents(const BaseExtent * root1, const Transform & T1,
										const BaseExtent * root2, const Transform & T2, float epsilon);


	virtual bool COMAPI recurse_collide_extents(const BaseExtent *& x1, const BaseExtent *& x2, 
												const BaseExtent * e1, const Transform & T1,
												const BaseExtent * e2, const Transform & T2, float epsilon);

	virtual bool COMAPI collide_extent_hierarchies( const BaseExtent *& x1, const BaseExtent *& x2,
													const BaseExtent * root1, const Transform & T1,
													const BaseExtent * root2, const Transform & T2, float epsilon);

	// NEW CONTACT-ONLY functions.
	virtual void COMAPI compute_contact(CollisionData & data, const BaseExtent * x1, const Transform & T1,
																			 const BaseExtent * x2, const Transform & T2);


*/
	//
	// Misc.
	//
	RigidBody * GetArchetype(ARCHETYPE_INDEX a) const
	{
		if( a == INVALID_ARCHETYPE_INDEX ) {
			return NULL;
		}

		arch_map::iterator arch;

		if( (arch = Archetypes.find( a )) == Archetypes.end() ) {
			return NULL;
		}

		return arch->second;
	}

	Instance * GetInstance (INSTANCE_INDEX i) const
	{
		if( i == INVALID_ARCHETYPE_INDEX ) {
			return NULL;
		}

		inst_map::iterator inst;

		if( (inst = Instances.find( i )) == Instances.end() ) {
			return NULL;
		}

		return inst->second;
	}

	virtual void COMAPI set_min_dt(float _min_dt)
	{
		min_dt = _min_dt;
	}
	virtual float COMAPI get_min_dt(void) const
	{
		return min_dt;
	}

};



#endif

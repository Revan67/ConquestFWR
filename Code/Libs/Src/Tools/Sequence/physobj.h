#ifndef PHYSOBJ_H
#define PHYSOBJ_H

//

#include "3DMath.h"
#include "object.h"
#include "LList.h"

//

struct IPhysicalObject
{
// assumed to be center-of-mass position.

	virtual int					get_object_index(void) const = 0;
	virtual void				set_object_index(int idx) = 0;

	virtual const Vector &		get_position(void) const = 0;
	virtual void				set_position(const Vector & pos) = 0;

	virtual const Matrix &		get_orientation_mat(void) const = 0;
	virtual void				set_orientation(const Matrix & R) = 0;

	virtual const Quaternion &	get_orientation_quat(void) const = 0;
	virtual void				set_orientation(const Quaternion & q) = 0;

	virtual const Transform		get_transform(void) const = 0;

	virtual const Vector &		get_velocity(void) const = 0;
	virtual void				set_velocity(const Vector & v) = 0;

	virtual const Vector &		get_angular_velocity(void) const = 0;
	virtual void				set_angular_velocity(const Vector & w) = 0;

	virtual const Vector &		get_momentum(void) const = 0;
	virtual void				set_momentum(const Vector & p) = 0;

	virtual const Vector &		get_angular_momentum(void) const = 0;
	virtual void				set_angular_momentum(const Vector & L) = 0;

	virtual float				get_mass(void) const = 0;
	virtual float				get_inverse_mass(void) const = 0;

	virtual const Matrix &		get_inertia_tensor_body(void) const = 0;
	virtual const Matrix &		get_inertia_tensor_world(void) const = 0;
	virtual const Matrix &		get_inverse_inertia_tensor_world(void) const = 0;

	virtual const Vector &		get_net_force(void) const = 0;
	virtual const Vector &		get_net_torque(void) const = 0;

	virtual void				add_force(const Vector & F) = 0;
	virtual void				add_torque(const Vector & T) = 0;

	virtual void				add_impulse_at_point(const Vector & impulse, const Vector & point) = 0;

	virtual void				compute_auxiliary_vars(void) = 0;

	virtual void				reset(void) = 0;

	virtual void				add_controller(struct IDynamicController * ctrl) = 0;
	virtual void				update_controllers(void) = 0;

	virtual const BaseExtent *	get_extent(void) const = 0;
	virtual int					get_leaf_extents(const BaseExtent ** leaves) const = 0;

	virtual float				get_radius(void) const = 0;
	virtual float				get_cross_sectional_area(void) const = 0;
	virtual float				get_drag_coefficient(void) const = 0;
	virtual void				get_AABB(float & x0, float & x1, float & y0, float & y1, float & z0, float & z1) const = 0;

// 0.5 * cross-sectional area * drag coefficient.
	virtual float				get_drag_factor(void) const = 0;

	virtual bool				is_sleeping(void) const = 0;
	virtual void				sleep(void) = 0;
	virtual void				wake_up(void) = 0;

	virtual void				clear_sleep_counter(void) = 0;
	virtual void				increment_sleep_counter(void) = 0;
	virtual int					get_sleep_counter(void) const = 0;


	virtual void				save_state(void) = 0;
	virtual void				restore_state(void) = 0;

	virtual void				get_state_vector(float * dst) const = 0;
	virtual void				set_state_vector(float * src) = 0;
	virtual unsigned int		get_state_index(void) const = 0;
	virtual void				set_state_index(unsigned int idx) = 0;

// Interpolates between SAVED state and CURRENT state.
	virtual void				interpolate_state(float * dst, float t) = 0;

// These just set flags for use in PhysicalSystem::update().
	virtual bool				is_colliding(void) const = 0;
	virtual void				set_colliding(void) = 0;
	virtual void				clear_colliding(void) = 0;

	virtual bool				is_tangible(void) const = 0;
	virtual void				set_tangible(bool) = 0;

// This notifies the object that it collided with something in the last frame.
	virtual void				notify_colliding(const Vector & N) = 0;

	virtual bool				is_connected(void) const = 0;
	virtual bool				is_child(void) const = 0;
};

//
// Basic implementation of IPhysicalObject interface.
//
struct PhysicalObject : public GameObject, public IPhysicalObject
{
	Quaternion	q;

	float		mass;
	float		inv_mass;

	Matrix		Ibody;
	Matrix		Iworld;
	Matrix		inv_Iworld;

	Vector		v;
	Vector		w;

	Vector		p;
	Vector		L;

	Vector		F_external;
	Vector		T_external;

	const BaseExtent *	extent;
	int					num_leaf_extents;
	const BaseExtent ** leaf_extents;

	float		radius;	// of outermost sphere extent.
	float		area;	// cross-sectional area. Derived from extents. 
	float		drag_coefficient;
	float		drag_factor;

	int			sleep_cnt;

	bool		sleeping:1;
	bool		colliding:1;
	bool		tangible:1;
	bool		saved_state_valid:1;
	float		saved_state[19];		// normal state vector, plus force and torque.

	int			object_index;			// index of this object in the physical system's object list.

	int			system_index;			// index of this object's state in state vector of entire system.

	LinkedList<struct IDynamicController *> controllers;

	struct ABMLink *link;

	void init(class SequenceApp * app);

	virtual ~PhysicalObject(void);

// base class overloads.
	virtual void Render(struct ICamera * the_camera, U32 flags);

// IPhysicalObject functions.
	virtual int					get_object_index(void) const;
	virtual void				set_object_index(int idx);
	virtual const Vector &		get_position(void) const;
	virtual void				set_position(const Vector & pos);
	virtual const Matrix &		get_orientation_mat(void) const;
	virtual void				set_orientation(const Matrix & _R);
	virtual const Quaternion &	get_orientation_quat(void) const;
	virtual void				set_orientation(const Quaternion & _q);
	virtual const Transform		get_transform(void) const;
	virtual const Vector &		get_velocity(void) const;
	virtual void				set_velocity(const Vector & _v);
	virtual const Vector &		get_angular_velocity(void) const;
	virtual void				set_angular_velocity(const Vector & _w);
	virtual const Vector &		get_momentum(void) const;
	virtual void				set_momentum(const Vector & _p);
	virtual const Vector &		get_angular_momentum(void) const;
	virtual void				set_angular_momentum(const Vector & _L);
	virtual float				get_mass(void) const;
	virtual float				get_inverse_mass(void) const;
	virtual const Matrix &		get_inertia_tensor_body(void) const;
	virtual const Matrix &		get_inertia_tensor_world(void) const;
	virtual const Matrix &		get_inverse_inertia_tensor_world(void) const;
	virtual const Vector &		get_net_force(void) const;
	virtual const Vector &		get_net_torque(void) const;
	virtual void				add_force(const Vector & F);
	virtual void				add_torque(const Vector & T);
	virtual void				add_impulse_at_point(const Vector & impulse, const Vector & point);
	virtual void				compute_auxiliary_vars(void);
	virtual void				reset(void);
	virtual void				add_controller(struct IDynamicController * ctrl);
	virtual void				update_controllers(void);
	virtual const BaseExtent *	get_extent(void) const;
	virtual int					get_leaf_extents(const BaseExtent ** leaves) const;
	virtual float				get_radius(void) const;
	virtual float				get_cross_sectional_area(void) const;
	virtual float				get_drag_coefficient(void) const;
	virtual void				get_AABB(float & x0, float & x1, float & y0, float & y1, float & z0, float & z1) const;
	virtual float				get_drag_factor(void) const;
	virtual bool				is_sleeping(void) const;
	virtual void				sleep(void);
	virtual void				wake_up(void);
	virtual void				clear_sleep_counter(void);
	virtual void				increment_sleep_counter(void);
	virtual int					get_sleep_counter(void) const;
	virtual void				save_state(void);
	virtual void				restore_state(void);
	virtual void				get_state_vector(float * dst) const;
	virtual void				set_state_vector(float * src);
	virtual unsigned int		get_state_index(void) const;
	virtual void				set_state_index(unsigned int idx);
	virtual void				interpolate_state(float * dst, float t);
	virtual bool				is_colliding(void) const;
	virtual void				set_colliding(void);
	virtual void				clear_colliding(void);
	virtual void				notify_colliding(const Vector & N);
	virtual bool				is_tangible(void) const;
	virtual void				set_tangible(bool);
	virtual bool				is_connected(void) const;
	virtual bool				is_child(void) const;
};

//

#endif
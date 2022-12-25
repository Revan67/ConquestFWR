//
// Featherstone's Articulated Body Method for computing constrained dynamics
// of tree-linked rigid bodies.
//

//
// Joint limit ideas:
//
// If joint is at limit and acceleration is toward violation of limit, zero velocity
// and treat as a JT_FIXED.
// If joint acceleration is away from joint violation, treat normally.
// This means computing accelerations multiple times, i.e. once to check for limit
// violation and once to compute fixed inertias for violated joints.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "filesys.h"
#include "instance.h"
#include "abm.h"
#include "rigid.h"
#include "Debugprint.h"

//

bool FixJoints = false;

//


Vector ZeroVector(0.0f, 0.0f, 0.0f);
Matrix ZeroMatrix(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

//

inline Vector link_to_world(const ABMLink * link, const Vector & vec)
{
	return link->owner->R * vec;
}

//

inline Vector world_to_link(const ABMLink * link, const Vector & vec)
{
	return link->owner->R.get_transpose() * vec;
}

//

inline Vector prev_to_link(const ABMLink * link, const Vector & vec)
{
	return link->R_rel * vec;
}

//

inline Vector link_to_prev(const ABMLink * link, const Vector & vec)
{
	return link->R_rel.get_transpose() * vec;
}

//

inline Matrix ComputeRelativeTransform(const Matrix & src, const Matrix & dst)
{
	return dst.get_transpose() * src;
}

//

float NormalizeAngle(float q)
{
	float result;
	static const float two_pi = 2.0f * PI;

	if (q > PI)
	{
 		result = q - two_pi;
	}
	else if (q < -PI)
	{
		result = two_pi + q;
	}
	else
	{
		result = q;
	}
	return result;
}

//

SpatialMatrix ComputeSpatialTransform(const Matrix & R, const Vector & r)
{
	Matrix rR = dual(-r) * R;
	SpatialMatrix result(R, ZeroMatrix, rR, R);
	return result;										 
}

//

// Doubling this size because the we are running into objects that have more than 128 joints in
// Freelancer! Is this right? -TNB
#define MAX_TREE_LINKS	256

INSTANCE_INDEX	abm_root1;
ABMLink			links1[MAX_TREE_LINKS];
INSTANCE_INDEX	abm_root2;
ABMLink			links2[MAX_TREE_LINKS];
	

struct EngineJoint 
{
	INSTANCE_INDEX parent;
	INSTANCE_INDEX child;
	const JointInfo *joint;
};
								  
int			NumJoints1;
EngineJoint joints1[MAX_TREE_LINKS];
int			NumJoints2;
EngineJoint joints2[MAX_TREE_LINKS];

SpatialMatrix	cb1, cb2;

int				CurrentIndex;

//
// Allocate links as requested.
//

ABMLink * AllocLink(INSTANCE_INDEX idx)
{
	ABMLink * l = (CurrentIndex == 0) ? links1 : links2;

	unsigned int i = 0;

	while ((l->index != idx) && (l->index != INVALID_INSTANCE_INDEX) && (i < MAX_TREE_LINKS))
	{
		l++;
		i++;
	}

	if (i >= MAX_TREE_LINKS)
	{
		// Made this fatal because no one is checking the return value of this method.
		// Returning from this report will guarantee a crash.
		GENERAL_FATAL ("Failed to allocate link. Exceeded max links.\n");
		l = NULL;
	}

	return l;
}

ABMLink * GetLink(INSTANCE_INDEX idx)
{
	ABMLink * l = (CurrentIndex == 0) ? links1 : links2;

	unsigned int i = 0;

	while ((l->index != idx) && (l->index != INVALID_INSTANCE_INDEX) && (i < MAX_TREE_LINKS))
	{
		l++;
		i++;
	}

	if (l->index == INVALID_INSTANCE_INDEX || (i >= MAX_TREE_LINKS))
		l = NULL;

	return l;
}

//

void RigidBodyPhysics::compute_link_velocities(INSTANCE_INDEX root)
{
	JOINT_INDEX j = INVALID_INSTANCE_INDEX;
	INSTANCE_INDEX parent;
	float joint_q_dot;

	while( (j = engine->get_instance_child_next(root, 0, j)) != INVALID_JOINT_INDEX)
	{
		const JointInfo *joint = engine->get_joint_info(j);

		engine->get_joint_state( j, IE_JST_FIRST_DERIVATIVE, &joint_q_dot );

		parent = engine->get_instance_parent( j );

		ABMLink * h = GetLink(parent);
		ABMLink * i = GetLink(j);

		//EMAURER sometimes instances can have no physical properties.
		//Check for that and bail if necessary.
		if (h && i && h->owner && i->owner)
		{
			Matrix R = ComputeRelativeTransform(h->owner->R, i->owner->R);
			Vector r = world_to_link(i, i->owner->x - h->owner->x);


	//#define JOINT_FRICTION
	#ifdef JOINT_FRICTION
		// Add a little friction.
			switch (joint->type)
			{
				case JT_PRISMATIC:
				{
					Vector force = i->u * -(i->q * 10 + i->q_dot * 2.5f) * h->owner->mass;
		float fm = force.magnitude();
		if (fm > 1e+4)
		{
			PHYTRACE11("fm: %f\n", fm);
		}
		//DebugPrint("q: %f q_dot: %f force: %f, %f, %f\n", i->q, i->q_dot, force.x, force.y, force.z);
					//force.zero();
					SpatialVector actuator_force = SpatialVector(force, ZeroVector);
					i->Q = dot(i->ss.transpose(), actuator_force);
				}
				break;
				case JT_REVOLUTE:
				{
				// damping.
					Vector torque = i->u * i->q_dot * -21.0f * i->owner->mass;
    			// artificial driver force:
					torque += i->u * -100.0f * i->owner->mass;

				// convert to spatial vector.
					SpatialVector actuator_force(ZeroVector, torque);
					i->Q = dot(i->ss.transpose(), actuator_force);
				}
				break;
			case JT_FIXED:
				i->Q = 0;
				break;
			}
	#else
			if (!FixJoints && i->driver)
			{
				float force, torque;
				i->driver->drive( parent, j, joint, force, torque);
				SpatialVector driver_force(i->u * force, i->u * torque);
				i->Q = dot(i->ss.transpose(), driver_force);
			}
			else
			{
				i->Q = 0.0f;
			}
	#endif

		// Store these guys for later use.
			i->R_rel = R;
			i->r = r;

			i->omega = R * h->omega;
			i->v = R * h->v + cross_product(i->omega, r);

			switch (joint->type)
			{
				case JT_PRISMATIC:
					if (FixJoints)
					{
					// inherit parent's velocities.
					}
					else
					{
						i->v += joint_q_dot * i->u;
					}
					break;
				case JT_REVOLUTE:
					if (FixJoints)
					{
					// inherit parent's velocities.
					}
					else
					{
						i->omega += joint_q_dot * i->u;
						i->v += joint_q_dot * cross_product(i->u, i->d);
					}
					break;
				case JT_FIXED:
				// inherit parent's velocities with no alteration.
					break;
			}
		}
	}
}

//

void InitLink(ABMLink * link, ABMLink * parent, JointType type)
{
	ASSERT (link);

	Vector v1 = -world_to_link(link, link->owner->F_applied + link->owner->F_internal);
	Vector v2 = cross_product(link->omega, link->owner->Ibody * link->omega);
	link->ZA = SpatialVector(v1, v2);

	Matrix Mi;
	Mi.set_identity();
	Mi *= link->owner->mass;
#if 1
	link->IA = SpatialMatrix(ZeroMatrix, Mi, link->owner->Ibody, ZeroMatrix);
#else
	link->IA = SpatialABI(Mi, ZeroMatrix, link->owner->Ibody);
#endif

	Vector nu = (FixJoints || (type == JT_FIXED)) ? Vector(0, 0, 0) : link->q_dot * link->u;
	Vector wh = (parent) ? prev_to_link(link, parent->omega) : Vector(0, 0, 0);

	switch (type)
	{
		case JT_PRISMATIC:
		{
			Vector vec = cross_product(wh, cross_product(wh, link->r)) + 
						 2.0f * cross_product(wh, nu);
			link->cs = SpatialVector(ZeroVector, vec);
			break;
		}
		case JT_REVOLUTE:
		{
			Vector nu_cross_d = cross_product(nu, link->d);

			Vector v1 = cross_product(wh, nu);
			Vector v2 = cross_product(wh, cross_product(wh, link->r)) + 
						2.0f * cross_product(wh, nu_cross_d) +
						cross_product(nu, nu_cross_d);
			link->cs = SpatialVector(v1, v2);
			break;
		}
		case JT_FIXED:
		{
			Vector vec = cross_product(wh, cross_product(wh, link->r));
			link->cs = SpatialVector(ZeroVector, vec);
			break;
		}
	}
}

//

void RigidBodyPhysics::init_links(INSTANCE_INDEX root)
{
	EngineJoint * jnt_ptr = (CurrentIndex == 0) ? joints1 : joints2;
	int n = (CurrentIndex == 0) ? NumJoints1 : NumJoints2;

	for (int jn = 0; jn < n; jn++, jnt_ptr++)
	{
		const JointInfo* joint = jnt_ptr->joint;

		ABMLink * h = GetLink(jnt_ptr->parent);
		ABMLink * i = GetLink(jnt_ptr->child);

		InitLink(i, h, jnt_ptr->joint->type);
	}

	ABMLink * root_link = GetLink(root);
	if (root_link && (root_link->owner->dynamic == DS_DYNAMIC))
	{
		InitLink(root_link, NULL, JT_REVOLUTE);
	}
}

//

void RigidBodyPhysics::compute_ABM_dynamics(Instance * root)
{
	ASSERT(root);

	CurrentIndex = 0;

	for (int i = 0; i < MAX_TREE_LINKS; i++)
	{
		links1[i].index = -1;
	}

// Init link data structures.

	NumJoints1 = 0;
	ABMLink * root_link = AllocLink(root->index);
	root_link->index = root->index;
	root_link->owner = root;

	root_link->v = world_to_link(root_link, root->v);
	root_link->omega = world_to_link(root_link, root->w);
	root_link->ss.zero();

	ABMLink * first_child = NULL;

	JOINT_INDEX j = INVALID_INSTANCE_INDEX;
	while( (j = engine->get_instance_child_next(root->index, 0, j)) != INVALID_JOINT_INDEX)
	{
		const JointInfo* joint = engine->get_joint_info(j);
		if (joint->type == JT_PRISMATIC || joint->type == JT_REVOLUTE || joint->type == JT_FIXED)
		{
			INSTANCE_INDEX p_inst = engine->get_instance_parent(j);
			float joint_q;

			Instance * pinst = GetInstance(p_inst);
			Instance * cinst = GetInstance(j);
			if (pinst && cinst)
			{
				joints1[NumJoints1++].child = j;
				joints1[NumJoints1++].parent = p_inst;
				joints1[NumJoints1++].joint = joint;

				ABMLink * p = AllocLink(p_inst);
				p->index = p_inst;
				p->owner = pinst;
				//p->ss.zero();

				ABMLink * c = AllocLink(j);
				c->index = j;
				c->owner = cinst;
				c->u = world_to_link(c, p->owner->R * joint->axis);
				c->d = -joint->child_point;

				switch (joint->type)
				{
					case JT_PRISMATIC:
						engine->get_joint_state( j, IE_JST_BASIC, &joint_q );
						c->ss = SpatialVector(Vector(0.0f, 0.0f, 0.0f), c->u);
						c->q = NormalizeAngle(joint_q);
						break;
					case JT_REVOLUTE:
						engine->get_joint_state( j, IE_JST_BASIC, &joint_q );
						c->ss = SpatialVector(c->u, cross_product(c->u, c->d));
						c->q = joint_q;
						break;
					case JT_FIXED:
						c->ss.zero();
						break;
				}

				c->q_dot = c->owner->q_dot;

				if (0)//joint->type == JT_PRISMATIC || joint->type == JT_REVOLUTE)
				{
					if (c->q <= joint->min0)
					{
						c->at_limit = (c->q_dot <= 0);
					}
					else if (c->q >= joint->max0)
					{
						c->at_limit = (c->q_dot >= 0);
					}
					else
					{
						c->at_limit = false;
					}
				}
				else
				{
					c->at_limit = false;
				}

				c->driver = joint_drivers[j];

				if (!first_child && (p == root_link))
				{
					first_child = c;
				}
			}
		}

	}

	if (NumJoints1 == 0)
	{
		return;
	}

	//EMAURER link database now setup. Just use it from here on out.

	compute_link_velocities(root->index);
	init_links(root->index);

	int bottom = (root_link->owner->dynamic == DS_DYNAMIC) ? -1 : 0;

	EngineJoint *jnt_ptr = joints1 + NumJoints1 - 1;
	for (i = NumJoints1 - 1; i > bottom; i--, jnt_ptr--)
	{
		const JointInfo* joint = jnt_ptr->joint;

		ABMLink * h = GetLink(jnt_ptr->parent);
		ABMLink * i = GetLink(jnt_ptr->child);

		//EMAURER sometimes instances can have no physical properties.
		//Check for that and bail if necessary.
		if (h && i)
		{
			Vector rback = world_to_link(h, h->owner->x - i->owner->x);
	#if 1
			SpatialMatrix h_to_i = ComputeSpatialTransform(i->R_rel, i->r);
			SpatialMatrix i_to_h = ComputeSpatialTransform(i->R_rel.get_transpose(), rback);
	#else
		SpatialTransform h_to_i(i->R_rel, i->r);
		SpatialTransform i_to_h(i->R_rel.get_transpose(), rback);
	#endif

			if (FixJoints || (joint->type == JT_FIXED))
			{
	#if 1
				h->IA += i_to_h * i->IA * h_to_i;
				h->ZA += i_to_h * (i->ZA + i->IA * i->cs);
	#else
				h->IA += i_to_h.transform_ABI(i->IA);
				h->ZA += i_to_h.transform_vector(i->ZA + i->IA * i->cs);
	#endif
			}
			else
			{
				if (i->at_limit)
				{
	#if 1
					h->IA += i_to_h * i->IA * h_to_i;
					h->ZA += i_to_h * (i->ZA + i->IA * i->cs);
	#else
					h->IA += i_to_h.transform_ABI(i->IA);
					h->ZA += i_to_h.transform_vector(i->ZA + i->IA * i->cs);
	#endif
				}
				else
				{
				// DEFINE IssI / sIs ? Spatial ABI?

					SpatialVector Is = i->IA * i->ss;
					i->sIs = dot(i->ss.transpose(), Is);
					i->Qs = i->Q - dot(i->ss.transpose(), i->ZA + i->IA * i->cs);
					SpatialVector sI = i->ss.transpose() * i->IA;
					SpatialMatrix IssI = Is * sI;

					h->IA += i_to_h * (i->IA - IssI / i->sIs) * h_to_i;
					h->ZA += i_to_h * (i->ZA + (i->IA * i->cs) + (Is * i->Qs) / i->sIs);
				}
			}
		}
	}

	ABMLink * link;
	if (root_link->owner->dynamic == DS_DYNAMIC)
	{
		link = root_link;
		link->Q = 0.0f;
 		link->as = -link->IA.get_inverse() * link->ZA;
		link->alpha = link->as.v1;
		link->a = link->as.v2;
	}
	else
	{
		link = first_child;
		links1->as.zero();
	}

	link->sIs = dot(link->ss.transpose(), link->IA * link->ss);
	link->Qs = link->Q - dot(link->ss.transpose(), link->ZA + link->IA * link->cs);

	jnt_ptr = joints1;

	for (j = 0; j < NumJoints1; j++, jnt_ptr++)
	{
		const JointInfo* joint = jnt_ptr->joint;

		ABMLink * h = GetLink(jnt_ptr->parent);
		ABMLink * i = GetLink(jnt_ptr->child);

		//EMAURER sometimes instances can have no physical properties.
		//Check for that and bail if necessary.
		if (h && i)
		{
			SpatialMatrix h_to_i = ComputeSpatialTransform(i->R_rel, i->r);
			SpatialVector ah = h_to_i * h->as;

			if (FixJoints || (jnt_ptr->joint->type == JT_FIXED))
			{
				i->q_dot_dot = 0;
			}
			else
			{
				SpatialVector Ia = i->IA * ah;
				float dotprod = dot(i->ss.transpose(), Ia);
				i->q_dot_dot = (i->Qs - dotprod) / i->sIs;
			}
						   
	//DebugPrint("joint %d: q: %f q_dot: %f q_dot_dot: %f\n", j, i->q, i->q_dot, i->q_dot_dot);

			i->as = ah + i->cs + i->q_dot_dot * i->ss;

	i->owner->q_dot = i->q_dot;
	i->owner->q_dot_dot = i->q_dot_dot;
		}
	}

// adjust forces:
	if (root_link->owner->dynamic == DS_DYNAMIC)
	{
		root_link->owner->F_applied.zero();
		root_link->owner->F_internal = root_link->owner->mass * link_to_world(root_link, root_link->a);
		root_link->owner->T_applied.zero();
		root_link->owner->T_internal = link_to_world(root_link, root_link->owner->Ibody * root_link->alpha);
	}
}

//

bool RigidBodyPhysics::compute_ABM_collision_matrix(Matrix & K, Instance * root, Instance * body, const Matrix &Rcoll, const Vector & rcoll, int index)
{
	ASSERT(root);

	if (index == 0)
	{
		CurrentIndex = 0;
		abm_root1 = root->index;
	}
	else
	{
		CurrentIndex = 1;
		abm_root2 = root->index;
	}
//
// NEED TO ALLOW FOR AT LEAST 2 TREE STRUCTURES TO COEXIST so we
// can collide multibodies against each other.
//
	ABMLink * l = (CurrentIndex == 0) ? links1 : links2;
	for (int i = 0; i < MAX_TREE_LINKS; i++, l++)
	{
		l->index = -1;
	}

// Init link data structures.

	int & NumJoints = (CurrentIndex == 0) ? NumJoints1 : NumJoints2;
	NumJoints = 0;
	ABMLink * root_link = AllocLink(root->index);
	root_link->index = root->index;
	root_link->owner = root;
	root_link->parent = NULL;

	root_link->v = world_to_link(root_link, root->v);
	root_link->omega = world_to_link(root_link, root->w);
	root_link->ss.zero();

	coll_root = root_link;
	coll_link = NULL;

	ABMLink * first_child = NULL;
	float joint_q;
	EngineJoint *joints = (CurrentIndex == 0) ? joints1 : joints2;
	JOINT_INDEX j = INVALID_INSTANCE_INDEX;
	while( (j = engine->get_instance_child_next(root->index, 0, j)) != INVALID_JOINT_INDEX)
	{
		const JointInfo* joint = engine->get_joint_info(j);
		if (joint->type == JT_PRISMATIC || joint->type == JT_REVOLUTE || joint->type == JT_FIXED)
		{
			INSTANCE_INDEX p_inst = engine->get_instance_parent(j);
			
			Instance * pinst = GetInstance(p_inst);
			Instance * cinst = GetInstance(j);

			if (pinst && cinst)
			{
				joints[NumJoints].parent = p_inst;
				joints[NumJoints].child = j;
				joints[NumJoints].joint = joint;
				NumJoints++;

				ABMLink * p = AllocLink(p_inst);
				p->index = p_inst;
				p->owner = pinst;

				ABMLink * c = AllocLink(j);
				c->to_parent = joint;
				c->index = j;
				c->owner = cinst;
				c->u = world_to_link(c, p->owner->R * joint->axis);
				c->d = -joint->child_point;
				
				if (joint->type == JT_PRISMATIC)
				{
					c->ss = SpatialVector(Vector(0.0f, 0.0f, 0.0f), c->u);
				}
				else if (joint->type == JT_REVOLUTE)
				{
					c->ss = SpatialVector(c->u, cross_product(c->u, c->d));
				}
				c->parent = p;
				if (c->owner == body)
				{
					coll_link = c;
				}

				if (joint->type == JT_REVOLUTE)
				{
					engine->get_joint_state( j, IE_JST_BASIC, &joint_q );
					c->q = NormalizeAngle(joint_q);
				}
				else
				{
					engine->get_joint_state( j, IE_JST_BASIC, &joint_q );
					c->q = joint_q;
				}
				c->q_dot = c->owner->q_dot;

				if (!first_child && (p == root_link))
				{
					first_child = c;
				}
			}
		}

	}

	if (NumJoints == 0) return false;

	ASSERT(body);

	if (!coll_link)
	{
		coll_link = coll_root;
	}
	compute_link_velocities(root->index);
	init_links(root->index);

	//int bottom = (root_link->owner->dynamic == DS_DYNAMIC) ? -1 : 0;
	int bottom = -1;

	EngineJoint *jnt_ptr = joints + NumJoints - 1;
	for (i = NumJoints - 1; i > bottom; i--, jnt_ptr--)
	{
		const JointInfo* joint = jnt_ptr->joint;

		ABMLink * h = GetLink(jnt_ptr->parent);
		ABMLink * i = GetLink(jnt_ptr->child);

		//EMAURER sometimes instances can have no physical properties.
		//Check for that and bail if necessary.
		if (h && i)
		{
			SpatialMatrix h_to_i = ComputeSpatialTransform(i->R_rel, i->r);
			Vector rback = world_to_link(h, h->owner->x - i->owner->x);
			SpatialMatrix i_to_h = ComputeSpatialTransform(i->R_rel.get_transpose(), rback);

			if (FixJoints || (joint->type == JT_FIXED))
			{
				h->IA += i_to_h * i->IA * h_to_i;
			}
			else
			{
				i->sIs = dot(i->ss.transpose(), i->IA * i->ss);
				SpatialVector Is = i->IA * i->ss;
				SpatialVector sI = i->ss.transpose() * i->IA;
				SpatialMatrix IssI = Is * sI;

				h->IA += i_to_h * (i->IA - IssI / i->sIs) * h_to_i;
			}
		}
	}

//
// EVERYTHING ABOVE IS EXACTLY IDENTICAL TO compute_ABM_dynamics().
//
// Rcoll is collision-->world.
//
	Matrix I; I.set_identity();

	Vector rc = rcoll;
	Vector rk = body->x;
	Matrix Rc = Rcoll;
	Matrix Rk = body->R;

	Matrix R0 = Rk.get_transpose() * Rc;
	Vector r0 = Rk.get_transpose() * (rk - rc);

	Matrix R1 = Rc.get_transpose() * Rk;
	Vector r1 = Rc.get_transpose() * (rc - rk);

	SpatialMatrix & cb = (CurrentIndex == 0) ? cb1 : cb2;

	cb.init(R0, ZeroMatrix, dual(-r0) * R0, R0);
	body2coll.init(R1, ZeroMatrix, dual(-r1) * R1, R1);

	Vector phi[3];
	for (i = 0; i < 3; i++)
	{
		Vector ei(I.d[0][i], I.d[1][i], I.d[2][i]);
		SpatialVector pcoll = cb * SpatialVector(ei, ZeroVector);

		impulse_response(root_link, coll_link, pcoll);

		SpatialVector v = body2coll * coll_link->delta_v;
		phi[i] = v.v2; 
	}

	K.set_i(phi[0]);
	K.set_j(phi[1]);
	K.set_k(phi[2]);

	return true;
}

//

void RigidBodyPhysics::impulse_response(ABMLink * root, ABMLink * k, const SpatialVector & pcoll)
{
	ASSERT(root);
	ASSERT(k);

	k->YA = -pcoll;
	ABMLink * i = k;

	int num_on_path = 1;
	ABMLink * path[32];
	path[0] = k;

	SpatialMatrix I;
	I.set_identity();

	while (i->parent)
	{
		ABMLink * h = i->parent;

		path[num_on_path++] = h;

		Vector rback = world_to_link(h, h->owner->x - i->owner->x);
		SpatialMatrix i_to_h = ComputeSpatialTransform(i->R_rel.get_transpose(), rback);

		if (FixJoints || (i->to_parent->type == JT_FIXED))
		{
			h->YA = i_to_h * i->YA;
		}
		else
		{
			SpatialVector Is = i->IA * i->ss;
			SpatialMatrix Iss = Is * i->ss.transpose();
			h->YA = i_to_h * (I - Iss / i->sIs) * i->YA;
		}

		i = h;
	}

	root->as.zero();
	num_on_path -= 1;

	ABMLink * h = path[num_on_path--];

	if (h->owner->dynamic == DS_DYNAMIC)
	{
		SpatialMatrix hIAinv = h->IA.get_inverse();
		h->delta_v = -hIAinv * h->YA;
	}
	else
	{
		h->delta_v.zero();
	}

	while (i != k)
	{
		i = path[num_on_path--];

		SpatialMatrix h_to_i = ComputeSpatialTransform(i->R_rel, i->r);

		SpatialVector hdv = h_to_i * h->delta_v;

		if (FixJoints || (i->to_parent->type == JT_FIXED))
		{
			i->delta_q_dot = 0;
			i->delta_v = hdv;
		}
		else
		{
			i->delta_q_dot = -dot(i->ss.transpose() / i->sIs, i->IA * hdv + i->YA);
			i->delta_v = hdv + i->delta_q_dot * i->ss;
		}

		h = i;
	}
}

//

void RigidBodyPhysics::propagate_impulse(Instance * root, Instance * obj, const Vector & p, const Matrix & Rcoll, const Vector & rcoll, int index)
{
	ASSERT(root);
	ASSERT(obj);

	CurrentIndex = index;

	coll_root = GetLink(root->index);
	coll_link = GetLink(obj->index);

// initialize link impulses to zero.
	ABMLink * link = (CurrentIndex == 0) ? links1 : links2;
	for (int j = 0; j < MAX_TREE_LINKS; j++, link++)
	{
		if (link->index != -1)
		{
			link->YA.zero();
		}
	}

// ignore root and body, we've already saved off the root and colliding links.
	ABMLink * k = coll_link;

	SpatialMatrix & cb = (CurrentIndex == 0) ? cb1 : cb2;

	SpatialVector pcoll = cb * SpatialVector(p, ZeroVector);
	k->YA = -pcoll;

	SpatialMatrix I;
	I.set_identity();
	
	ABMLink * i = k;
	while (i->parent)
	{
		ABMLink * h = i->parent;

		Vector rback = world_to_link(h, h->owner->x - i->owner->x);
		SpatialMatrix i_to_h = ComputeSpatialTransform(i->R_rel.get_transpose(), rback);

		if (FixJoints || (i->to_parent->type == JT_FIXED))
		{
			h->YA = i_to_h * i->YA;
		}
		else
		{
			SpatialVector Is = i->IA * i->ss;
			SpatialMatrix Iss = Is * i->ss.transpose();

			float sIs = dot(i->ss.transpose(), Is);

			h->YA = i_to_h * (I - Iss / i->sIs) * i->YA;
		}

		i = h;
	}

	coll_root->as.zero();
	if (coll_root->owner->dynamic == DS_DYNAMIC)
	{
		SpatialMatrix hIAinv = coll_root->IA.get_inverse();
		coll_root->delta_v = -hIAinv * coll_root->YA;
	}
	else
	{
		coll_root->delta_v.zero();
	}

	EngineJoint *jnt_ptr = (CurrentIndex == 0) ? joints1 : joints2;
	int & NumJoints = (CurrentIndex == 0) ? NumJoints1 : NumJoints2;
	for (j = 0; j < NumJoints; j++, jnt_ptr++)
	{
		const JointInfo* joint = jnt_ptr->joint;

		ABMLink * h = GetLink(jnt_ptr->parent);
		ABMLink * i = GetLink(jnt_ptr->child);

		//EMAURER sometimes instances can have no physical properties.
		//Check for that and bail if necessary.
		if (h && i)
		{
			SpatialMatrix h_to_i = ComputeSpatialTransform(i->R_rel, i->r);

			SpatialVector hdv = h_to_i * h->delta_v;

			if (FixJoints || (joint->type == JT_FIXED))
			{
				i->delta_q_dot = 0;
				i->delta_v = hdv;
			}
			else
			{
				i->delta_q_dot = -dot(i->ss.transpose() / i->sIs, i->IA * hdv + i->YA);
	//DebugPrint("dqdot: %f\n", i->delta_q_dot);
				i->delta_v = hdv + i->delta_q_dot * i->ss;
			}
		}
	}


	if( joint_dynamics )
	{
	//
	// Now propagate velocity changes.
	//
	// 1. Adjust joint velocities.
	//
		link = (CurrentIndex == 0) ? links1 : links2;
		for (j = 0; j < MAX_TREE_LINKS; j++, link++)
		{
			if (link->index != -1)
			{
				if (link->parent)
				{
					link->q_dot += link->delta_q_dot;
					link->owner->q_dot = link->q_dot;
				}
				else // root
				{
					link->omega += link->delta_v.v1;
					link->v += link->delta_v.v2;
				}
			}
		}

	//
	// 2. Notify joints of new velocities.
	//
		j = INVALID_INSTANCE_INDEX;
		while( (j = engine->get_instance_child_next(coll_root->index, 0, j)) != INVALID_JOINT_INDEX )
		{
			const JointInfo * joint = engine->get_joint_info(j);

			//EMAURER joint velocity not defined for fixed joints
			if (joint->type != JT_FIXED)
			{
				Instance * child = GetInstance(j);

				if (child)
				{
					engine->set_joint_state( j, IE_JST_FIRST_DERIVATIVE, &child->q_dot );
		//			joint->q_dot = child->q_dot;
				}
			}
		}

	//TODO: at some point these new joint velocities should probably damp out
	}
	else // root only (no joints)
	{
		coll_root->omega += coll_root->delta_v.v1;
		coll_root->v += coll_root->delta_v.v2;
	}

	// update root instance velocity in engine
	coll_root->owner->set_angular_velocity(link_to_world(coll_root, coll_root->omega));
	coll_root->owner->set_velocity(link_to_world(coll_root, coll_root->v));
	coll_root->owner->sync_data_to_engine(engine);

	// update child velocities
	engine->update_instance( root->index, EN_UPDATE_WO_COMPONENTS, 0.0f );
}

//


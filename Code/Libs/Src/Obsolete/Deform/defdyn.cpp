//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <float.h>

#include <3dmath.h>
#include <engine.h>
#include <model.h>
#include <physics.h>
#include "eng.h"
#include "EulerAngles.h"

//

void DebugPrint (char *fmt, ...);

//

using namespace Deform;

//

static void EnableFPUExceptions(void)
{
	unsigned int cw = _controlfp(0, 0);
// Set the exception masks OFF, turn exceptions on.
	cw &= ~(EM_OVERFLOW|EM_ZERODIVIDE|EM_DENORMAL|EM_INVALID);
	_controlfp(cw, MCW_EM);
}

static void DisableFPUExceptions(void)
{
	unsigned int cw = _controlfp(0, 0);
// Set the exception masks ON, turn exceptions off.
	cw |= (EM_OVERFLOW|EM_ZERODIVIDE|EM_DENORMAL|EM_INVALID);
	_controlfp(cw, MCW_EM);
}

//

const Vector acceleration_of_gravity(0, -30, 0);
//const Vector acceleration_of_gravity(0, 0, 0);

//

Matrix ParallelAxisTheorem(const Matrix & Ibody, const Vector & R, float mass)
{
	Matrix J = Ibody;

	float Rsq = dot_product(R, R);
	Matrix RR;

	float xx = R.x * R.x;
	float xy = R.x * R.y;
	float xz = R.x * R.z;
	float yy = R.y * R.y;
	float yz = R.y * R.z;
	float zz = R.z * R.z;

	RR.d[0][0] = xx;
	RR.d[0][1] = xy;
	RR.d[0][2] = xz;

	RR.d[1][0] = xy;
	RR.d[1][1] = yy;
	RR.d[1][2] = yz;

	RR.d[2][0] = xz;
	RR.d[2][1] = yz;
	RR.d[2][2] = zz;

	Matrix Id; Id.set_identity();

	J += mass * (Rsq * Id - RR);

	return J;
}

//

struct DynLink
{
	const char * name;

// in world frame:
	Vector	p;			// position of proximal joint.
	Vector	v;			// velocity of proximal joint.
	Vector	fE;			// external force acting on link at pE (see below).
	Vector	gE;			// external torque on link.

// in local frame:
	Vector	a;			// acceleration of proximal joint.
	Vector	w;			// angular velocity of link.
	Vector	wdot;		// angular acceleration.
	Vector	c;			// Vector from proximal joint to center of mass of link.
	Vector	f;			// force which link exerts on its parent at proximal hinge.
	Vector	g;			// torque which link exerts on its parent at proximal hinge.
	Vector	pE;			// vector from proximal hinge to the point of application of the external force fE.

	Matrix	Ibody;		// inertia tensor of link about CM.
	Matrix	J;			// inertia tensor of link about its proximal hinge.

// in parent's frame:
	Vector	l;			// vector from parent's proximal joint to proximal hinge. (constant in parent's frame).

// rotation matrices.
	Matrix	R;			// transforms vectors from link frame to parent's frame.
	Matrix	RI;			// transforms from link frame to world frame (regular object orientation).

	Vector	du;			// delta rotation.

	float	mass;

	INSTANCE_INDEX	index;
	JOINT_INDEX		joint_index;	// index of joint between this link and its parent.

	bool	visited:1;

// Some pre-computed constants:
	Matrix	mI;			// mass * Identity matrix.
	Vector	mc;			// mass * c
	Matrix	mcd;		// mass * dual(c)
	Matrix	ld;			// dual(l)
	Vector	maG;		// mass * acceleration_of_gravity.

	float	a0, a1, a2;
	Matrix	Rmid;		// midpoint orientation.

// stuff that's computed every frame:
	Vector	ac;
	Matrix	Q;
	Matrix	M;
	Matrix	T;
	Matrix	K;

	Vector	fs;
	Vector	g1s;
	Vector	gs;
	Vector	d;
	Vector	fp;
	Vector	fpp;

// linkages.
	DynLink * parent;
	DynLink * child;
	DynLink * sibling;

	float	submass;	// mass of this link + all its children.
	float	stiffness;


// INVERSE DYNAMICS STUFF.
	Vector	f_base;
	Vector	t_base;
	Vector	f_inboard;
	Vector	accel_cm;
	Vector	accel_org;
	Vector	t_inboard;
	Vector	cm;

// IMPULSE STUFF.
	Vector	delta_p;	// change in linear momentum.
	Vector	delta_L;	// change in angular momentum.
	Matrix	articulated_mass;
	Matrix	articulated_inertia;
	Matrix	pTp;

	DynLink(void)
	{
		memset(this, 0, sizeof(*this));
	}
};

//

int NumLinks = 0;
DynLink * Links[128];

//

DynLink * root;
float total_mass = 0;

//

void InitLink(DynLink * link, const Joint * joint)
{
// in local frame:

// Vector from proximal joint to center of mass of link.
	if (joint)
	{
		link->cm.zero();

		link->c = link->cm - joint->child_point;

	// Compute "midpoint" joint value, i.e. one in which all 3 
	// angles are at the middle of their ranges.
		link->a0 = 0.5 * (joint->min0 + joint->max0);
		link->a1 = 0.5 * (joint->min1 + joint->max1);
		link->a2 = 0.5 * (joint->min2 + joint->max2);
	 	MatrixFromEulerAngles(link->Rmid, link->a0, link->a1, link->a2);
//link->Rmid.set_identity();
	}
	else
	{
		link->c.zero();
	}
// inertia matrix of link about its proximal hinge.

	//link->J = *(PHYSICS->get_inertia_tensor(link->index));
	//link->mass = PHYSICS->get_mass(link->index);

	PHYSICS->set_dynamic(link->index, DS_NONDYNAMIC);

// in parent's frame:
	DynLink * parent = link->parent;
	if (parent)
	{
		Matrix Rp = ENG->get_orientation(parent->index);
		DynLink * grandpa = parent->parent;
		if (grandpa)
		{
			JOINT_INDEX j1 = MODEL->find_joint(grandpa->index, parent->index);
			JOINT_INDEX j2 = MODEL->find_joint(parent->index, link->index);

			const Joint * jnt1 = MODEL->get_joint(j1);
			const Joint * jnt2 = MODEL->get_joint(j2);

		// by definition, joint points are in parent's frame.
			link->l = jnt2->parent_point - jnt1->child_point;
		}
		else
		{
			JOINT_INDEX j = MODEL->find_joint(parent->index, link->index);
			const Joint * jnt = MODEL->get_joint(j);

			//link->l = jnt->parent_point - PHYSICS->get_local_center_of_mass(parent->index);
			link->l = jnt->parent_point - ENG->get_position(parent->index);
		}
	}
	else
	{
		link->l.zero();
	}

// HAVE NO DECENT DATA.
#if 1
float lmag = link->l.magnitude();
link->Ibody.set_identity();
float factor = (lmag > 1e-5) ? lmag*10 : 1;

link->mass = factor * 1;

link->Ibody *= factor * 10;

#else

	link->Ibody = *PHYSICS->get_inertia_tensor(link->index);
	link->mass = PHYSICS->get_mass(link->index);

#endif

// link->c is vector from proximal hinge to center of mass.
	link->J = ParallelAxisTheorem(link->Ibody, link->c, link->mass);


	Matrix I; I.set_identity();
	link->mI	= link->mass * I;
	link->mc	= link->mass * link->c;
	link->mcd	= dual(link->mc);
	link->ld	= dual(link->l);
	link->maG	= link->mass * acceleration_of_gravity;

	link->name = MODEL->get_name(link->index);
	link->stiffness = 1.0;

	total_mass += link->mass;
}

//

void BuildSubTree(INSTANCE_INDEX ridx, DynLink * rlink)
{
	rlink->index = ridx;

	DynLink * prev = NULL;

	INSTANCE_INDEX child = MODEL->get_child(ridx);
	while (child != INVALID_INSTANCE_INDEX)
	{
		DynLink * child_link = new DynLink;
		Links[NumLinks++] = child_link;
		child_link->parent = rlink;

		if (prev)
		{
			prev->sibling = child_link;
		}
		else
		{
			rlink->child = child_link;
		}
		prev = child_link;

		BuildSubTree(child, child_link);

		JOINT_INDEX ji = MODEL->find_joint(ridx, child);
		const Joint * joint = MODEL->get_joint(ji);
		InitLink(child_link, joint);
		child_link->joint_index = ji;

		child = MODEL->get_child(ridx, child);
	}
}

//

void BuildTree(INSTANCE_INDEX root_idx)
{
	root = new DynLink;
	root->index = root_idx;
	InitLink(root, NULL);
	Links[NumLinks++] = root;

	BuildSubTree(root_idx, root); 

	root->p = ENG->get_position(root->index);

	for (int i = 0; i < NumLinks; i++)
	{
		DynLink * link = Links[i];
		link->RI = ENG->get_orientation(link->index);

		link->submass = link->mass;
	}

	for (i = NumLinks - 1; i >= 0; i--)
	{
		DynLink * link = Links[i];

		DynLink * child = link->child;
		while (child)
		{
			link->submass += child->submass;
			child = child->sibling;
		}
	}
}

//

void slow_integ(void)
{
	for (int i = NumLinks - 1; i >= 0; i--)
	{
		DynLink * link = Links[i];

	// update orientation.
		Quaternion q(link->RI);
		Quaternion qw(link->RI * link->du);
		Quaternion qdot = qw * q / 2.0;
		q.w += qdot.w;
		q.x += qdot.x;
		q.y += qdot.y;
		q.z += qdot.z;
		q.normalize();
		link->RI = q;
		link->du.zero();

		DynLink * child = link->child;
		while (child)
		{
			child->R = link->RI.get_transpose() * child->RI;
			child = child->sibling;
		}
	}
}

//
// Each link is guaranteed to follow its parents in the Link list.
//

void slowband_in(void)
{
	Matrix I; I.set_identity();

	int stop = 0;
	for (int i = NumLinks - 1; i >= stop; i--)
	{
		DynLink * link = Links[i];

		Matrix Wsum; Wsum.zero();
		Matrix Wlsum; Wlsum.zero();

		DynLink * child = link->child;
		while (child)
		{
			child->ac = cross_product(link->w, cross_product(link->w, child->l));

			child->Q = child->R * child->M * child->R.get_transpose();

			Matrix W = child->ld * child->Q;
			Wsum += W;
			Wlsum += W * child->ld;

			child = child->sibling;
		}

		Matrix Tinv = link->J + Wlsum;
		link->T = Tinv.get_inverse();

		link->K = link->T * (Wsum - link->mcd);

		Matrix Qsum; Qsum.zero();

		child = link->child;
		while (child)
		{
			Qsum += child->Q * (I - child->ld * link->K);

			child = child->sibling;
		}

		link->M = link->mcd * link->K - link->mI + Qsum;

		link->g1s = -cross_product(link->w, link->J * link->w);
		link->g1s += link->gE * link->RI;											// transpose!
		link->g1s += cross_product(link->pE, link->fE * link->RI);					// transpose.
		link->g1s += cross_product(link->mc, acceleration_of_gravity * link->RI);	// transpose.

		link->fs = -cross_product(link->w, cross_product(link->w, link->mc)) + (link->fE + link->maG) * link->RI;
	}
}

//

void fastband_in(void)
{
	int stop = 0;
	for (int i = NumLinks - 1; i >= stop; i--)
	{
		DynLink * link = Links[i];

		link->gs = link->g1s - link->g;

		Vector cross_sum(0, 0, 0);

		DynLink * child = link->child;
		while (child)
		{
			link->gs += child->R * child->g;

			cross_sum += cross_product(child->l, (child->fpp + child->Q * child->ac));

			child = child->sibling;
		}

		link->d = link->T * (link->gs + cross_sum);

		link->fp = link->fs + cross_product(link->mc, link->d);
		child = link->child;
		while (child)
		{
			link->fp += child->fpp + child->Q * (child->ac - cross_product(child->l, link->d));
			child = child->sibling;
		}

		link->fpp = link->R * link->fp;
	}
}

//

void fastband_out(void)
{
	root->a = -(root->M.get_inverse() * root->fp);
	root->wdot = root->K * root->a + root->d;

root->a.zero();
root->wdot.zero();

	for (int i = 1; i < NumLinks; i++)
	{
		DynLink * link = Links[i];
		DynLink * parent = link->parent;

		link->a = (link->ac + parent->a - cross_product(link->l, parent->wdot)) * link->R;	// transpose.
		link->wdot = link->K * link->a + link->d;

	// Force link exerts on its parent at the proximal hinge.
		link->f = link->M * link->a + link->fp;
	}
}

//

void fast_integ(float dt)
{
	for (int i = 0; i < NumLinks; i++)
	{
		DynLink * link = Links[i];

		link->w += dt * link->wdot;
		link->du += dt * link->w;
	}

	root->v += dt * root->RI * root->a;
	root->p += dt * root->v;
}

//

void UpdateTree(void)
{
	for (int i = 0; i < NumLinks; i++)
	{
		DynLink * link = Links[i];

		link->RI = ENG->get_orientation(link->index);

		if (link->parent)
		{
		   const Joint * jnt = MODEL->get_joint(link->joint_index);
		   link->p = ENG->get_position(link->index) + link->RI * jnt->child_point;

		   link->R = link->parent->RI.get_transpose() * link->RI;

link->Rmid = link->R;
		}
		else
		{
			link->p = ENG->get_position(link->index);
		}
	}
}

//

Vector qlog(const Quaternion & q);
Quaternion qexp(const Vector & v);
Vector qrel(const Quaternion & q1, const Quaternion & _q2);
int CheckLimits(const Joint * joint, float x, float y, float z);

void ComputeInverseDynamics(const Vector & desired_accel);

//

void compute_dynamics(float dt)
{
//	root->fE = -total_mass * acceleration_of_gravity;
	root->pE.zero();

	slow_integ();

	for (int i = 1; i < NumLinks; i++)
	{
		DynLink * link = Links[i];

//		link->g.zero();

	// Damping torque.
		Vector rel_w = link->parent->w * link->R - link->w; // transpose
		link->g += -link->mass * rel_w * 0.5;

		//Vector L = link->Ibody * rel_w;
		//link->g += -L * 0.9;

/*
	// Maintenance torque.
		Quaternion q0(link->R);
		Quaternion q1(link->Rmid);
		Vector omega = -qrel(q0, q1);

		float omag = omega.magnitude();
		if (omag > 1e-5)
		{
			link->g += omega * link->mass * link->stiffness;
		}
*/
	}

	slowband_in();
	fastband_in();
	fastband_out();
	fast_integ(dt);
}

//

void ApplyImpulse(const Vector & p, const Vector & loc, DynLink * hit);

//

void ComputeDynamics(float dt)
{
	EnableFPUExceptions();

//ApplyImpulse(Vector(0, 0, -1), Vector(0, 0, 0), Links[NumLinks-1]);

// update root position & orientation.
	if (MODEL->get_parent(root->index) == INVALID_INSTANCE_INDEX)
	{
		ENG->set_position(root->index, root->p);
		ENG->set_orientation(root->index, root->RI);
	}

	DynLink * last = Links[NumLinks - 1];

	const int inverse_frequency = 0;
	int inverse_counter = inverse_frequency;

	float time_left = dt;
	const float min_step = 1.0/120;
	while (time_left > 0)
	{
		float step = __min(time_left, min_step);

		if ((inverse_counter > 0) && (inverse_counter-- == 0))
		{
			ComputeInverseDynamics(Vector(0, 0, 0));

			for (int i = 0; i < NumLinks; i++)
			{
				DynLink * link = Links[i];

			// link->g is torque link exerts on its parent at inboard joint.
			// link->t_inboard is torque exerted on link by its parent.

				link->g = link->t_inboard;
			}

			inverse_counter = inverse_frequency;
		}
		else
		{
			for (int i = 0; i < NumLinks; i++)
			{
				DynLink * link = Links[i];
				link->g.zero();
			}
		}

		compute_dynamics(step);

		time_left -= min_step;
	}

//Vector a = last->RI * last->accel_org;
//DebugPrint("a: %f, %f, %f\n", a.x, a.y, a.z);

// munge orientations into joint settings that we can pass on to IModel.
	for (int i = 1; i < NumLinks; i++)
	{
		DynLink * link = Links[i];

		const Joint * jnt = MODEL->get_joint(link->joint_index);
		Matrix Rj = link->parent->RI.get_transpose() * link->RI * jnt->rel_orientation.get_transpose();
		Quaternion q(Rj);

		MODEL->set_joint_data(link->joint_index, &(q.w));
	}

	DisableFPUExceptions();
}

//

void FillBoneList(HWND list)
{
	for (int i = 0; i < NumLinks; i++)
	{
		SendMessage(list, LB_INSERTSTRING, i, (LONG) Links[i]->name);
	}
}

void GetBoneProperties(int bone, Matrix & J, float & mass, float & stiffness)
{
	DynLink * link = Links[bone];

	J = link->J;
	mass = link->mass;
	stiffness = link->stiffness;
}

//

void SetBoneProperties(int bone, const Matrix & J, float mass, float stiffness)
{
	DynLink * link = Links[bone];

	link->J = J;
	link->mass = mass;
	link->stiffness = stiffness;
}

//

void ComputeInverseDynamics(const Vector & desired_accel)
{
	for (int i = 0; i < NumLinks; i++)
	{
		if (Links[i]->child == NULL)
		{
			DynLink * last = Links[i];

			if (0)//(strcmp(last->name, "rthumb") == 0) || (strcmp(last->name, "rfingers") == 0))
			{
			// f_inboard is opposite (reaction force) of force link exerts on its parent.
				last->f_inboard = -last->f;
				last->t_inboard = -last->g;
			/*
				//last->f_inboard = last->mass * (Vector(0, -30, 0) - acceleration_of_gravity) * last->RI;
				const Joint * joint = MODEL->get_joint(last->joint_index);
				Vector r = joint->child_point;
				last->t_inboard = cross_product(r, last->f_inboard);
			*/
			}
			else
			{
				last->f_inboard = last->mass * (desired_accel - acceleration_of_gravity) * last->RI;
				last->t_inboard.zero();
			}

		}
	}

	root->accel_org.zero();

	for (i = 1; i < NumLinks; i++)
	{
		DynLink * link = Links[i];

	// REALLY DON'T NEED TO DO ANY WORK FOR LEAF NODES.
		if (link->child == NULL) continue;

		DynLink * parent = link->parent;

		Matrix A = link->RI.get_transpose() * parent->RI;

	//
	// Compute accleration at origin of link coordinate system.
	//
	// Relative vector from parent's origin to link's origin, expressed in parent's coordinate system.
	//
		Vector r = (ENG->get_position(link->index) - ENG->get_position(parent->index)) * parent->RI;
		link->accel_org = A * (parent->accel_org + cross_product(parent->wdot, r) + cross_product(parent->w, cross_product(parent->w, r)));

	//
	// Compute base force on object (not accounting for forces from children).
	//
		Vector grav_local = acceleration_of_gravity * link->RI;
		link->f_base = link->mass * (link->accel_org - grav_local);

	// 
	// Compute base torque;
	//
		link->t_base = link->Ibody * link->wdot + cross_product(link->w, link->Ibody * link->w);

	//
	// We're resolving forces/torques at the origin of the local coordinate system,
	// so force at center of mass may induces torque at origin.
	//

// DOES THIS TERM EVEN MAKE SENSE?
//		r = PHYSICS->get_local_center_of_mass(link->index);
//		link->t_base += cross_product(r, link->f_base);
	}

	for (i = NumLinks - 2; i >= 0; i--)
	{
		DynLink * link = Links[i];
		if (link->child == NULL) continue;

		const Joint * j0 = MODEL->get_joint(link->joint_index);
		Vector p_inboard = j0->child_point;

		link->f_inboard = link->f_base;
		link->t_inboard = link->t_base;

		DynLink * child = link->child;
		while (child)
		{
			Matrix A = link->RI.get_transpose() * child->RI;

			Vector f_outboard = -(A * child->f_inboard);
			Vector t_outboard = -(A * child->t_inboard);

			link->f_inboard -= f_outboard;
			link->t_inboard -= t_outboard;

		// Force applied at outboard joint induces torque at inboard joint:
			const Joint * joint = MODEL->get_joint(child->joint_index);
			Vector p_outboard = joint->parent_point;

			Vector r = p_outboard - p_inboard;
			link->t_inboard += cross_product(r, f_outboard);

			child = child->sibling;
		}
	}
}

//

Vector compute_impulse_response(const Vector & p, const Vector & loc, DynLink * link)
{
	Vector dv;

	link->delta_p = -p;

	Vector r = (loc - ENG->get_position(link->index)) * link->RI;
	link->delta_L = -cross_product(r, p);

	DynLink * parent = link->parent;
	while (parent)
	{
//		parent->delta_p = ;
//		parent->delta_L = ;

		link = parent;
		parent = link->parent;
	}

	return dv;
}

//

void ApplyImpulse(const Vector & p, const Vector & loc, DynLink * hit)
{
	Matrix I; I.set_identity();

// walk backward through links, computing articulated inertias.
	for (int i = NumLinks; i >= 0; i--)
	{
		DynLink * link = Links[i];

		link->articulated_mass		= link->mass * I;
		link->articulated_inertia	= link->Ibody;

		if (link->child)
		{
			Vector link_pos = ENG->get_position(link->index);

			DynLink * child = link->child;
			while (child)
			{
				Vector pi = (ENG->get_position(child->index) - link_pos) * link->RI;
				Matrix pt = dual(pi);
				child->pTp = (pt * link->articulated_mass * pt).get_inverse();

				link->articulated_mass += child->R * (child->articulated_mass - child->articulated_mass * pt * child->pTp * pt * child->articulated_mass) * child->R.get_transpose();

				child = child->sibling;
			}
		}
	}


// compute K by applying test impulse in x, y, z directions.
	Vector e[3], phi[3];
	e[0].set(1, 0, 0);
	e[1].set(0, 1, 0);
	e[2].set(0, 0, 1);

	for (i = 0; i < 3; i++)
	{
		Vector pcoll = e[i] * hit->RI;

		phi[i] = compute_impulse_response(pcoll, loc, hit);
	}

	Matrix K;
	K.set_i(phi[0]);
	K.set_j(phi[1]);
	K.set_k(phi[2]);
}


//

/*
void ApplyImpulse(const Vector & p, const Vector & loc, DynLink * hit)
{
	for (int i = 0; i < NumLinks; i++)
	{
		DynLink * link = Links[i];
		link->delta_p.zero();
		link->delta_L.zero();
	}

	Matrix I; I.set_identity();

// walk backward through links, computing articulated inertias.
	for (i = NumLinks; i >= 0; i--)
	{
		DynLink * link = Links[i];

		link->articulated_mass		= link->mass * I;
		link->articulated_inertia	= link->Ibody;

		if (link->child)
		{
			Vector link_pos = ENG->get_position(link->index);

			DynLink * child = link->child;
			while (child)
			{
				Vector pi = (ENG->get_position(child->index) - link_pos) * link->RI;
				Matrix pt = dual(pi);
				child->pTp = (pt * link->articulated_mass * pt).get_inverse();

				link->articulated_mass += child->R * (child->articulated_mass - child->articulated_mass * pt * child->pTp * pt * child->articulated_mass) * child->R.get_transpose();

				child = child->sibling;
			}
		}
	}

	hit->delta_p = -p * hit->RI;
	Vector r = (loc - ENG->get_position(hit->index));
	hit->delta_L = cross_product(r, -p) * hit->RI;

	DynLink * link = hit;
	while (link && link != root)
	{
		DynLink * parent = link->parent;

	// link->R goes from link to parent.

		Vector pi = (ENG->get_position(link->index) - ENG->get_position(parent->index)) * link->RI;
		Matrix ps = dual(pi);
		Matrix T = (ps * link->articulated_mass * ps).get_inverse();

		parent->delta_p = link->R * ((I - link->articulated_mass * link->pTp) * link->delta_p);
		//parent->delta_L = ;

		link = link->parent;
	}
}
*/
//
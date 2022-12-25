//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


#include <float.h>
#include "IDumpText.h"
#include "Deform.h"
#include "Model.h"
#include "eng.h"
#include "EulerAngles.h"
#include "RPUL\MTPrimitiveBuilder.h"
#include "timer.h"
#include "IKinfo.h"
#include "Mat.h"

//

using namespace Deform;

//

void DebugPrint (char *fmt, ...);

//

void IKJoint::init(JOINT_INDEX jnt, CHANNEL_INSTANCE_INDEX chan)
{
	idx = jnt;
	channel = chan;

	const Joint * joint = MODEL->get_joint(idx);
}

//

HANDLE DeformableObject::start_aim(const AimDesc & desc, float transition_duration)
{
	int				num_joints = 0;
	JOINT_INDEX		joints[MAX_IK_JOINTS];

	bool child_offset = false;

	int num_dofs = 0;

	JOINT_INDEX jtemp[MAX_IK_JOINTS];

	INSTANCE_INDEX obj = desc.end_effector;
	while (obj != desc.root) 
	{
		INSTANCE_INDEX parent = MODEL->get_parent(obj);
		jtemp[num_joints] = MODEL->find_joint(parent, obj);

		const Joint * jnt = MODEL->get_joint(jtemp[num_joints]);
		if (jnt->type == JT_SPHERICAL)
		{
			if (jnt->min0 < jnt->max0)
			{
				num_dofs++;
			}
			if (jnt->min1 < jnt->max1)
			{
				num_dofs++;
			}
			if (jnt->min2 < jnt->max2)
			{
				num_dofs++;
			}

			float cm = jnt->child_point.magnitude();
			child_offset = (cm > 1e-5);
		}

		num_joints++;
		obj = parent;
	}

// REVERSE JOINTS!
	for (int i = 0; i < num_joints; i++)
	{
		joints[i] = jtemp[num_joints - i - 1];
	}

	ScriptLink * link = active_scripts.alloc();
	link->ik_script = true;
	link->ik = new IKScriptLink(desc.target, desc.R);
	link->ik->handle = link;
	link->ik->num_joints = num_joints;
	link->ik->joints = new JOINT_INDEX[num_joints];
	memcpy(link->ik->joints, joints, sizeof(JOINT_INDEX) * num_joints);
	link->ik->channels = new CHANNEL_INSTANCE_INDEX[num_joints];
	link->ik->data = new Quaternion[num_joints];
	link->ik->flags = desc.flags;
	link->ik->root = desc.root;
	link->ik->end_effector = desc.end_effector;

	link->ik->Rmid = new Matrix[num_joints];

	link->ik->locked = new bool[num_joints];
	memset(link->ik->locked, 0, sizeof(bool) * num_joints);
	link->ik->damping_factor = desc.damping_factor;

	Channel::StartParms parms;
	parms.target.type = Channel::JOINT;

	link->ik->num_locked_joints = desc.num_locked_children;

	for (i = 0; i < num_joints; i++)
	{
		CHANNEL_INSTANCE_INDEX idx = CHANNEL->create_channel_instance(link->ik, Channel::DT_QUATERNION);
		link->ik->channels[i] = idx;

		parms.target.joint = joints[i];
		parms.transition_duration = transition_duration;
		parms.overlay = true;

		CHANNEL->start(idx, &parms);

		const Joint * joint = MODEL->get_joint(joints[i]);
	// Compute "midpoint" joint value, i.e. one in which all 3 
	// angles are at the middle of their ranges.
		float a0 = 0.5 * (joint->min0 + joint->max0);
		float a1 = 0.5 * (joint->min1 + joint->max1);
		float a2 = 0.5 * (joint->min2 + joint->max2);
	 	MatrixFromEulerAngles(link->ik->Rmid[i], a0, a1, a2);

Quaternion qmid;
MODEL->get_joint_data(joints[i], &qmid.w);
link->ik->Rmid[i] = Matrix(qmid);
		INSTANCE_INDEX child = joint->child;
		for (int j = 0; j < desc.num_locked_children; j++)
		{
			if (desc.locked_children[j] == child)
			{
				link->ik->locked[i] = true;
				break;
			}
		}
	}

	link->ik->child_offset = child_offset;

	return (HANDLE) link;
}

//

void DeformableObject::end_aim(HANDLE aim)
{
	ScriptLink * link = (ScriptLink *) aim;
	link->ik->done = true;
}

//

static inline float deg_to_rad(float deg)
{
	return deg * 3.14159 / 180;
}

static inline float rad_to_deg(float rad)
{
	return rad * 180.0 / 3.14159;
}

//

int CheckLimits(const Joint * joint, float x, float y, float z)
{
	int result = 0;
	if (x < joint->min0 || x > joint->max0)
	{
		result++;
	}
	if (y < joint->min1 || y > joint->max1) 
	{
		result++;
	}
	if (z < joint->min2 || z > joint->max2) 
	{
		result++;
	}
	return result;
}

//

static Matrix EnforceLimits(const Joint * joint, const Matrix & R)
{
	Matrix result;

	float x, y, z;
	EulerAnglesFromMatrix(x, y, z, R, false);

	int v0 = CheckLimits(joint, x, y, z);
	if (v0)
	{
		float x0, y0, z0;
		EulerAnglesFromMatrix(x0, y0, z0, R, true);
		int v1 = CheckLimits(joint, x0, y0, z0);

		if (v1 <= v0)
		{
			x = x0;
			y = y0;
			z = z0;
		}
	}

	bool limit = false;
	if (x < joint->min0)
	{
		x = joint->min0;
		limit = true;
	}
	if (x > joint->max0) 
	{
		x = joint->max0;
		limit = true;
	}
	if (y < joint->min1) 
	{
		y = joint->min1;
		limit = true;
	}
	if (y > joint->max1) 
	{
 		y = joint->max1;
		limit = true;
	}
	if (z < joint->min2) 
	{
		z = joint->min2;
		limit = true;
	}
	if (z > joint->max2) 
	{
		z = joint->max2;
		limit = true;
	}

	if (limit)
	{
		MatrixFromEulerAngles(result, x, y, z);
	}
	else
	{
		result = R;
	}

	return result;
}

//

Matrix ComputeGoalOrientation(const Vector & look, const Matrix & R, U32 flags)
{
	Vector i, j, k;

	if (flags & AimDesc::AD_AIM_I)
	{
		i = look;
		k = R.get_k();
		j = cross_product(k, i);
		j.normalize();
		k = cross_product(i, j);
	}
	else if (flags & AimDesc::AD_AIM_J)
	{
		j = look;
		i = R.get_i();
		k = cross_product(i, j);
		k.normalize();
		i = cross_product(j, k);
	}
	else if (flags & AimDesc::AD_AIM_K)
	{
		k = look;
/*
		i = R.get_i();
		j = cross_product(k, i);
		j.normalize();
		i = cross_product(j, k);
*/
		j = R.get_j();
		i = cross_product(j, k);
		i.normalize();
		j = cross_product(k, i);
	}

	Matrix result(i, j, k);
	return result;
}

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

int IKScriptLink::update(void * dst, U32 channel_idx, const Channel::Target & target, float time)
{
//EnableFPUExceptions();

	if (done)
	{
		for (U32 i = 0; i < num_joints; i++)
		{
			if (channels[i] == channel_idx)
			{
				MODEL->get_joint_data(joints[i], (float *) dst);
				break;
			}
		}

		return 1;
	}

	if (channel_idx == channels[num_joints - 1])
	{
	// Do IK computation.
		if (target.type == Channel::JOINT)
		{
			const Joint * joint = MODEL->get_joint(target.joint);

			if (num_joints == 1)
			{
				INSTANCE_INDEX effector = joint->child;
				INSTANCE_INDEX parent = joint->parent;

				Matrix Rrel;

				Matrix Rp = ENG->get_orientation(parent);

				if (flags & AimDesc::AD_EE_ORIENT)
				{
				// FORGET THE STUPID goal orientation flags. Just make people specify an orientation dammit.
					Rrel = Rp.get_transpose() * orient;
					if ((flags & AimDesc::AD_IGNORE_LIMITS) == 0)
					{
						Rrel = EnforceLimits(joint, Rrel);
					}
				}
				else	// else using the goofy AD_AIM_I, etc. flags. Not recommended.
				{
					Vector ee_pos = ENG->get_position(effector);
					Vector aim = (flags & AimDesc::AD_POINT) ? point - ee_pos : point;
					aim.normalize();

					Matrix Rgoal = ComputeGoalOrientation(aim, Rp, flags);
					Rrel = Rp.get_transpose() * Rgoal;

					if ((flags & AimDesc::AD_IGNORE_LIMITS) == 0)
					{
						Rrel = EnforceLimits(joint, Rrel);
					}
				}

				*data = Quaternion(Rrel);
			}
			else
			{
				if (child_offset)
				{
					solve_sr_child_offset();
				}
				else
				{
					solve_sr();
				}
			}
		}
	}

//DisableFPUExceptions();

// Feed channel data.
	for (U32 i = 0; i < num_joints; i++)
	{
		if (channels[i] == channel_idx)
		{
			*((Quaternion *) dst) = data[i];
			break;
		}
	}

	return 0;
}

//

const int max_dofs = MAX_IK_JOINTS * 3;

//

//

inline float square(float f)
{
	return f*f;
}

//
// logarithmic map qlog: S3 --> R3
//
Vector qlog(const Quaternion & q)
{
	Vector result;
	
	if (fabs(1.0 - q.s) < 1e-5)
	{
		result.zero();
	}
	else
	{
		result = (acos(q.s) / sqrt(1 - square(q.s))) * q.v;
	}
	return result;
}

//
// exponential map qexp: R3 --> S3
//
Quaternion qexp(const Vector & v)
{
	Quaternion result;

	float vmag = v.magnitude();
	if (vmag > 1e-5)
	{
		result.s = cos(vmag);
		result.v = sin(vmag) / vmag * v;
	}
	else
	{
		result.set_identity();
	}

	return result;
}

//
// Get relative rotation vector between 2 quaternions.
//
Vector qrel(const Quaternion & q1, const Quaternion & _q2)
{
// Pick shorter arc.

	Quaternion sum, dif, q2;

	sum.w = q1.w + _q2.w;
	sum.x = q1.x + _q2.x;
	sum.y = q1.y + _q2.y;
	sum.z = q1.z + _q2.z;

	dif.w = q1.w - _q2.w;
	dif.x = q1.x - _q2.x;
	dif.y = q1.y - _q2.y;
	dif.z = q1.z - _q2.z;

	float s1 = dif.w * dif.w + dif.x * dif.x + dif.y * dif.y + dif.z * dif.z;
	float s2 = sum.w * sum.w + sum.x * sum.x + sum.y * sum.y + sum.z * sum.z;

	if (s1 > s2)
	{
		q2.w = -_q2.w;
		q2.x = -_q2.x;
		q2.y = -_q2.y;
		q2.z = -_q2.z;
	}
	else
	{
		q2 = _q2;
	}
	
	Vector omega = 2.0 * qlog(q2 * q1.get_conjugate());
	return omega;
}

//

void IKScriptLink::compute_forward_kinematics(Vector & p_eff, Matrix & R_eff, Vector * p_new, Matrix * R_new) const
{
}

//

#define MAX_IK_ITERATIONS	32

//#define SECONDARY_TASK	// use secondary task for joint limit avoidance

//

static Vector p[MAX_IK_JOINTS + 1], ptemp[MAX_IK_JOINTS + 1];
static Matrix R[MAX_IK_JOINTS + 1], Rtemp[MAX_IK_JOINTS + 1];

//

bool IKScriptLink::solve(void)
{
    bool compute_position		= (flags & AimDesc::AD_EE_POS) != 0;
	bool compute_orientation	= (flags & AimDesc::AD_EE_ORIENT) != 0;
	bool damping = false;

// Last joint doesn't affect position of end effector (ASSUMES joint->child_point = 0)
	U32 num_dofs = ((compute_orientation) ? num_joints : (num_joints - 1)) * 3;
	const U32 num_coords = ((compute_orientation) ? 3 : 0) + ((compute_position) ? 3 : 0);

	ASSERT((num_coords + num_dofs) < max_dofs);

// Set up initial orientations.
	p[0] = ptemp[0] = ENG->get_position(root);
	R[0] = Rtemp[0] = ENG->get_orientation(root);
	for (U32 i = 0; i < num_joints; i++)
	{
		MODEL->get_joint_data(joints[i], &data[i].w);
		const Joint * joint = MODEL->get_joint(joints[i]);
		p[i+1] = ENG->get_position(joint->child);
		R[i+1] = ENG->get_orientation(joint->child);
	}

	int eff_idx = num_joints;
	Vector pstart = p[eff_idx];

// Compute IK goal position and orientation.
	Vector pgoal;
	Matrix Rgoal;

	if (compute_position)
	{
		pgoal = point;
	}
	if (compute_orientation)
	{
		Rgoal = orient;
	}

	if (0)
	{
		pgoal = pgoal;
		const Vector & ee_pos = pstart;
		Vector aim = (flags & AimDesc::AD_POINT) ? point - ee_pos : point;
		aim.normalize();

		const Matrix & Rp = R[0];
		Rgoal = ComputeGoalOrientation(aim, Rp, flags);
	}

	bool out_of_whack = false;
	float dp_mag;
	if (compute_position)
	{
		Vector dp_total = pgoal - p[eff_idx];
		dp_mag = dp_total.magnitude();
		out_of_whack = (dp_mag > 1e-3);
	}

	Quaternion qgoal(Rgoal);
	if (!out_of_whack && compute_orientation)
	{
		Quaternion qcurr(R[eff_idx]);
		Vector dR_total = qrel(qcurr, qgoal);//2.0 * qlog(qgoal * qcurr.get_conjugate());
		float dR_mag = dR_total.magnitude();
		out_of_whack = (dR_mag > 1e-2);
	}

	if (!out_of_whack)
	{
		return true;
	}

	int R_row = (compute_position) ? 3 : 0;

	int num_steps = 1;
	float step_size = 1.0 / num_steps;

	float percent_goal = 0.0;
//	int num_iterations = 0;

	Vector dpos;

	Mat<max_dofs> J(num_coords, num_dofs);
	Mat<max_dofs> Jt(num_dofs, num_coords);

	int dim = (damping) ? num_coords + num_dofs : num_coords;

	Mat<max_dofs> Jdamp(num_coords + num_dofs, num_dofs);
	Mat<max_dofs> Jtdamp(num_dofs, num_coords + num_dofs);
	Vec<max_dofs> dtheta(num_dofs);
	Vec<max_dofs> delta_x(num_coords);

	U32 last_joint = (compute_orientation) ? num_joints : num_joints - 1;

	bool done = false;
	while (!done)
	{
	// Construct Jacobian matrix.

		float t = 2.0;
		float err;
		float last_err = FLT_MAX;
		float delta_err;

		Vector temp_goal;

	// BUILD JACOBIAN.
		int x = 0;

		U32 end_joint = (compute_orientation) ? num_joints+1 : num_joints;
		for (i = 1; i < end_joint; i++, x += 3)
		{
			if (compute_position)
			{
			// ASSUMES joint->child_point == 0.
				Vector r;
				if (i == num_joints)
				{
					r.zero();
				}
				else
				{
					const Joint * jnext = MODEL->get_joint(joints[i]);
					r = R[i] * jnext->parent_point;
				}

			// Copy dual(-r) into appropriate block of Jacobian.
				J.d[0][x+0] =
				J.d[1][x+1] =
				J.d[2][x+2] = 0;

				J.d[0][x+1] =  r.z;
				J.d[0][x+2] = -r.y;

				J.d[1][x+0] = -r.z;
				J.d[1][x+2] =  r.x;

				J.d[2][x+0] =  r.y;
				J.d[2][x+1] = -r.x;
			}

		// do orientation part:
			if (compute_orientation)
			{
				J.d[R_row+0][x+0] = 1.0;
				J.d[R_row+0][x+1] = 0.0;
				J.d[R_row+0][x+2] = 0.0;

				J.d[R_row+1][x+0] = 0.0;
				J.d[R_row+1][x+1] = 1.0;
				J.d[R_row+1][x+2] = 0.0;

				J.d[R_row+2][x+0] = 0.0;
				J.d[R_row+2][x+1] = 0.0;
				J.d[R_row+2][x+2] = 1.0;
			}
		}

	// jacobian is built. invert it using SVD.
		int rank_J;
		Jt = J.get_pseudo_inverse(rank_J);
//Jt = J.get_transpose();

		if (damping)
		{
		// Build and invert DLS jacobian.
			memcpy(Jdamp.d, J.d, sizeof(float) * max_dofs * max_dofs);
			for (i = num_coords; i < Jdamp.m; i++)
			{
				for (U32 j = 0; j < Jdamp.n; j++)
				{
					Jdamp.d[i][j] = ((i-num_coords) == j) ? damping_factor : 0.0;
				}
			}
			int rank_Jdamp;
			Jtdamp = Jdamp.get_pseudo_inverse(rank_Jdamp);
		}

		Vec<max_dofs> secondary_task(num_dofs);
		bool secondary = ((flags & AimDesc::AD_IGNORE_LIMITS) == 0);

		if (secondary)
		{
		// Compute projection onto null space of Jacobian: (I - JtJ)
		// JtJ computation could possibly be optimized?

		// USE DAMPED JACOBIAN FOR SECONDARY TASK?
			Mat<max_dofs> proj = Mat<max_dofs>::mul(Jt, J);
			for (i = 0; i < proj.m; i++)
			{
				for (unsigned int j = 0; j < proj.n; j++)
				{
					proj.d[i][j] = (i == j) ? 1.0 - proj.d[i][j] : 0.0 - proj.d[i][j];
				}
			}

		// Compute secondary task in joint space.
			Vec<max_dofs> sec(num_dofs);
			int sec_idx = 0;
		
			for (i = 0; i < last_joint; i++)
			{
				Matrix Rsec = R[i] * Rmid[i];

			// Compute angular difference between current pose and midpoint:
				Quaternion q0(R[i+1]);
				Quaternion q1(Rsec);
				Vector omega = qrel(q0, q1);//2.0 * qlog(q1 * q0.get_conjugate());
				sec.d[sec_idx+0] = omega.x;
				sec.d[sec_idx+1] = omega.y;
				sec.d[sec_idx+2] = omega.z;
				sec_idx += 3;
			}

			secondary_task = proj.mul(sec);
		}

		do
		{
			t *= 0.5;

		// Compute current offset.
			dpos = pgoal - p[eff_idx];

		// Compute desired change in end-effector state.
			dpos /= _MAX(num_steps, 1);
			num_steps--;

			Vec<max_dofs> dx(dim);

			if (compute_position)
			{
				dx.d[0] = dpos.x;									
				dx.d[1] = dpos.y;
				dx.d[2] = dpos.z;
			}

			if (compute_orientation)
			{
				Vector omega = qrel(Quaternion(R[eff_idx]), qgoal);//2.0 * qlog(qgoal * Quaternion(R[eff_idx]).get_conjugate());
				dx.d[R_row+0] = omega.x;
				dx.d[R_row+1] = omega.y;
				dx.d[R_row+2] = omega.z;
			}

		// Take step toward goal:

			if (damping)
			{
				if (secondary)
				{
					for (i = num_coords; i < secondary_task.len; i++)
					{
						dx.d[i] = 0.5 * secondary_task.d[i - num_coords];
					}
					//memcpy(&dx.d[num_coords], &secondary_task.d[0], sizeof(float) * secondary_task.len);
				}
				else
				{
					memset(&dx.d[num_coords], 0, sizeof(float) * (dx.len - num_coords));
				}
				dtheta = Jtdamp.mul(dx);
			}
			else
			{
			// Apply pseudo-inverse of Jacobian to get change in joint variables.
				dtheta = Jt.mul(dx);
				dtheta += secondary_task;
			}

		// Compute ACTUAL dx that dtheta gives us.
			Vector p0 = p[eff_idx];

			int x = 0;
			for (i = 0; i < last_joint; i++, x += 3)
			{
				const Joint * joint = MODEL->get_joint(joints[i]);

			// Pull omega from dtheta.
				Vector omega(dtheta[x], dtheta[x+1], dtheta[x+2]);
//				float wmag = omega.magnitude();
//				if (wmag > 0.08)
//				{
//					return false;
//				}

			// Compute new R[i+1] after rotation by omega.
				Matrix Rprime(qexp(0.5 * omega) * R[i+1]);

			// Set new R[i+1].
				Rtemp[i+1] = Rprime;

				ptemp[i+1] = ptemp[i] + Rtemp[i] * joint->parent_point;
			}

			if (compute_position)
			{
				Vector dp = ptemp[eff_idx] - p0;
				delta_x.d[0] = dp.x;
				delta_x.d[1] = dp.y;
				delta_x.d[2] = dp.z;
			}
			if (compute_orientation)
			{
				Quaternion qcurr(Rtemp[eff_idx]);
				Vector dR = qrel(qcurr, qgoal);//2.0 * qlog(qgoal * qcurr.get_conjugate());
				delta_x.d[R_row+0] = dR.x;
				delta_x.d[R_row+1] = dR.y;
				delta_x.d[R_row+2] = dR.z;
			}


		// Do error check:
			err = 0;
			for (i = 0; i < num_coords; i++)
			{
				err += square(dx[i] - delta_x[i]);
			}

			delta_err = err - last_err;
			last_err = err;

		// Don't continue if error isn't decreasing; we're probably in a singular configuration.
		} while ((err > 1e-2) && (delta_err < -(1e-5)));

	// We may have taken a smaller step than desired.
		percent_goal += t * step_size;
		t = 2.0;

		step_size = _MIN(step_size, 1.0f - percent_goal);

		done = (fabs(percent_goal - 1.0) <= 1e-2) || (delta_err >= 0);
	}


//	Vector dp = ptemp[eff_idx] - pgoal;
//	float dist = dp.magnitude();

// check current dist against prev dist. If no decrease, assume singlar configuration, 
// don't update joint data.

//	if (dist <= (dp_mag + 1e-4))
	{
		for (i = 0; i < last_joint; i++)
		{
		// Extract joint parameter, relative to R[i].
			data[i] = Quaternion(Rtemp[i].get_transpose() * Rtemp[i+1]);
		}
	}

	return true;
}

//
// singularity-robust IK.
//
void IKScriptLink::solve_sr(void)
{
//Timer t0;
    bool compute_position		= (flags & AimDesc::AD_EE_POS) != 0;
	bool compute_orientation	= (flags & AimDesc::AD_EE_ORIENT) != 0;

// Last joint doesn't affect position of end effector (ASSUMES joint->child_point = 0)
	U32 num_dofs = ((compute_orientation) ? num_joints : (num_joints - 1)) * 3;
	const U32 num_coords = ((compute_orientation) ? 3 : 0) + ((compute_position) ? 3 : 0);

	if (compute_orientation)
	{
		num_dofs -= num_locked_joints * 3;
	}
	else if (compute_position)
	{
	//  Last joint doesn't enter into position calculation. 
		int locked_joints_that_matter = num_locked_joints;
		if (num_locked_joints > 0)
		{
			if (locked[num_joints-1])
			{
				locked_joints_that_matter--;
			}
		}
		num_dofs -= locked_joints_that_matter * 3;
	}

	ASSERT((num_coords + num_dofs) < max_dofs);

// Set up initial orientations.
	p[0] = ptemp[0] = ENG->get_position(root);
	R[0] = Rtemp[0] = ENG->get_orientation(root);
	for (U32 i = 0; i < num_joints; i++)
	{
		MODEL->get_joint_data(joints[i], &data[i].w);
		const Joint * joint = MODEL->get_joint(joints[i]);
		p[i+1] = ENG->get_position(joint->child);
		R[i+1] = ENG->get_orientation(joint->child);
	}

	int eff_idx = num_joints;
	Vector pstart = p[eff_idx];

// Compute IK goal position and orientation.
	Vector pgoal;
	Matrix Rgoal;

	if (compute_position)
	{
		pgoal = point;
	}
	if (compute_orientation)
	{
		Rgoal = orient;
	}

	if (0)
	{
		pgoal = pgoal;
		const Vector & ee_pos = pstart;
		Vector aim = (flags & AimDesc::AD_POINT) ? point - ee_pos : point;
		aim.normalize();

		const Matrix & Rp = R[0];
		Rgoal = ComputeGoalOrientation(aim, Rp, flags);
	}

	bool out_of_whack = false;
	float dp_mag;
	if (compute_position)
	{
		Vector dp_total = pgoal - p[eff_idx];
		dp_mag = dp_total.magnitude();
		out_of_whack = (dp_mag > 1e-3);
	}

	float dR_mag;
	Quaternion qgoal(Rgoal);
	if (!out_of_whack && compute_orientation)
	{
		Quaternion qcurr(R[eff_idx]);
		Vector dR_total = qrel(qcurr, qgoal);//2.0 * qlog(qgoal * qcurr.get_conjugate());
		dR_mag = dR_total.magnitude();
		out_of_whack = (dR_mag > 1e-2);
	}

	if (!out_of_whack)
	{
		return;
	}

	int R_row = (compute_position) ? 3 : 0;

	int num_steps = 1;
	float step_size = 1.0 / num_steps;

	float percent_goal = 0.0;

	Vector dpos;

	Mat<max_dofs> J(num_coords, num_dofs);
	Mat<max_dofs> Jt(num_dofs, num_coords);

	Mat<max_dofs> Jdamp(num_coords + num_dofs, num_dofs);
	Vec<max_dofs> dtheta(num_dofs);
	Vec<max_dofs> delta_x(num_coords);

	U32 last_joint = (compute_orientation) ? num_joints : num_joints - 1;

	bool done = false;
	while (!done)
	{
	// Construct Jacobian matrix.

		float t = 2.0;
		float err;
		float last_err = FLT_MAX;
		float delta_err = 100;

		Vector temp_goal;

	// BUILD JACOBIAN.
		int x = 0;

		U32 end_joint = (compute_orientation) ? num_joints+1 : num_joints;
		for (i = 1; i < end_joint; i++)
		{
			if (compute_position)
			{
			// ASSUMES joint->child_point == 0.
				Vector r;
				if (i == num_joints)
				{
					r.zero();
				}
				else
				{
					const Joint * jnext = MODEL->get_joint(joints[i]);
					r = R[i] * jnext->parent_point;
				}

			// Copy dual(-r) into appropriate block of Jacobian.
				if (locked[i-1])
				{
				// add in to previous block.
					int y = x-3;

					J.d[0][y+1] +=  r.z;
					J.d[0][y+2] += -r.y;

					J.d[1][y+0] += -r.z;
					J.d[1][y+2] +=  r.x;

					J.d[2][y+0] +=  r.y;
					J.d[2][y+1] += -r.x;
				}
				else
				{
					J.d[0][x+0] =
					J.d[1][x+1] =
					J.d[2][x+2] = 0;

					J.d[0][x+1] =  r.z;
					J.d[0][x+2] = -r.y;

					J.d[1][x+0] = -r.z;
					J.d[1][x+2] =  r.x;

					J.d[2][x+0] =  r.y;
					J.d[2][x+1] = -r.x;
				}
			}

		// do orientation part:
			if (compute_orientation)
			{
				if (!locked[i-1])
				{
					J.d[R_row+0][x+0] = 1.0;
					J.d[R_row+0][x+1] = 0.0;
					J.d[R_row+0][x+2] = 0.0;

					J.d[R_row+1][x+0] = 0.0;
					J.d[R_row+1][x+1] = 1.0;
					J.d[R_row+1][x+2] = 0.0;

					J.d[R_row+2][x+0] = 0.0;
					J.d[R_row+2][x+1] = 0.0;
					J.d[R_row+2][x+2] = 1.0;
				}
			}

			if (!locked[i-1])
			{
				x += 3;
			}
		}

	// jacobian is built. invert it using SVD.
		Jt = J.get_transpose();

		Jdamp = Mat<max_dofs>::mul(Jt, J);
		//const float damping_factor = 1;
		float dsq = square(damping_factor);
		for (i = 0; i < Jdamp.m; i++)
		{
			for (U32 j = 0; j < Jdamp.n; j++)
			{
				if (i == j)
				{
					Jdamp.d[i][j] += dsq;
				}
			}
		}

	//
		//Mat<max_dofs> Jdamp_inv = Jdamp.get_inverse();
		int rnk;
		Mat<max_dofs> Jdamp_inv = Jdamp.get_pseudo_inverse(rnk);

		Vec<max_dofs> secondary_task(num_dofs);
		bool secondary = ((flags & AimDesc::AD_IGNORE_LIMITS) == 0);

		if (secondary)
		{
		// Compute secondary task in joint space.
			int sec_idx = 0;
		
			for (i = 0; i < last_joint; i++)
			{
				if (!locked[i])
				{
					Matrix Rsec = R[i] * Rmid[i];

				// Compute angular difference between current pose and midpoint:
					Quaternion q0(R[i+1]);
					Quaternion q1(Rsec);
					Vector omega = qrel(q0, q1);
					secondary_task.d[sec_idx+0] = omega.x;
					secondary_task.d[sec_idx+1] = omega.y;
					secondary_task.d[sec_idx+2] = omega.z;
					sec_idx += 3;
				}
			}
		}
									 
		do
		{
			t *= 0.5;

		// Compute current offset.
			dpos = pgoal - p[eff_idx];

		// Compute desired change in end-effector state.
			dpos /= _MAX(num_steps, 1);
			num_steps--;

			Vec<max_dofs> dx(num_coords);

		// dx is the actual difference between the goal and the end effector position.
			if (compute_position)
			{
				dx.d[0] = dpos.x;									
				dx.d[1] = dpos.y;
				dx.d[2] = dpos.z;
			}

			if (compute_orientation)
			{
				Vector omega = qrel(Quaternion(R[eff_idx]), qgoal);//2.0 * qlog(qgoal * Quaternion(R[eff_idx]).get_conjugate());
				dx.d[R_row+0] = omega.x;
				dx.d[R_row+1] = omega.y;
				dx.d[R_row+2] = omega.z;
			}

		// Take step toward goal:

			Vec<max_dofs> Jtdx = Jt.mul(dx);

			if (secondary)
			{
			// alpha = 0 means pure damped-least-squares, alpha = 1 means pure projection onto nullspace.
				const float alpha = 0.25;
				float damp_alpha = damping_factor * alpha;
				for (i = 0; i < Jtdx.len; i++)
				{
					Jtdx.d[i] += damp_alpha * secondary_task.d[i];
				}
			}

			dtheta = Jdamp_inv.mul(Jtdx);

		// Compute ACTUAL dx that dtheta gives us.
			Vector p0 = p[eff_idx];

			int x = 0;
			Vector omega;
			for (i = 0; i < last_joint; i++)
//			for (i = 0; i < end_joint; i++)
			{
				const Joint * joint = MODEL->get_joint(joints[i]);

				if (!locked[i])
				{
				// Pull omega from dtheta.
					omega.set(dtheta[x], dtheta[x+1], dtheta[x+2]);
					x += 3;
				}

			// Compute new R[i+1] after rotation by omega.
				Matrix Rprime(qexp(0.5 * omega) * R[i+1]);

			// Set new R[i+1].
				Rtemp[i+1] = Rprime;

				ptemp[i+1] = ptemp[i] + Rtemp[i] * joint->parent_point;
			}

		
		// delta_x is the difference between the goal and the end effector position after applying dtheta.
			if (compute_position)
			{
				Vector dp = ptemp[eff_idx] - p0;
				delta_x.d[0] = dp.x;
				delta_x.d[1] = dp.y;
				delta_x.d[2] = dp.z;
			}
			if (compute_orientation)
			{
				Quaternion qcurr(Rtemp[eff_idx]);
				Vector dR = qrel(qcurr, qgoal);//2.0 * qlog(qgoal * qcurr.get_conjugate());
				delta_x.d[R_row+0] = dR.x;
				delta_x.d[R_row+1] = dR.y;
				delta_x.d[R_row+2] = dR.z;
			}

		// Do error check:
			err = 0;
			for (i = 0; i < num_coords; i++)
			{
				err += square(dx[i] - delta_x[i]);
			}

			delta_err = err - last_err;
			last_err = err;
		
		// Don't continue if error isn't decreasing; we're probably in a singular configuration.
		} while ((err > 1e-2) && (delta_err < -(1e-5)));

	// We may have taken a smaller step than desired.
		percent_goal += t * step_size;
		t = 2.0;

		step_size = _MIN(step_size, 1.0f - percent_goal);

		done = (fabs(percent_goal - 1.0) <= 1e-2) || (delta_err >= 0);
	}

// check current dist against prev dist. If no decrease, assume singlar configuration, 
// don't update joint data.

//	if (dist <= (dp_mag + 1e-4))
	{
		for (i = 0; i < last_joint; i++)
		{
		// Extract joint parameter, relative to R[i].
			if (!locked[i])
			{
				data[i] = Quaternion(Rtemp[i].get_transpose() * Rtemp[i+1]);
			}
		}
	}
}

//
// singularity-robust solve with child offset, i.e. jnt->child_point != 0.
//
void IKScriptLink::solve_sr_child_offset(void)
{
//Timer t0;
    bool compute_position		= (flags & AimDesc::AD_EE_POS) != 0;
	bool compute_orientation	= (flags & AimDesc::AD_EE_ORIENT) != 0;

//
// ASSUME joint->child_point != 0, so last joint affects position of end effector.
//
	U32 num_dofs = num_joints * 3;
	const U32 num_coords = ((compute_orientation) ? 3 : 0) + ((compute_position) ? 3 : 0);

	if (compute_orientation)
	{
		num_dofs -= num_locked_joints * 3;
	}
	else if (compute_position)
	{
	//  Last joint enters into position calculation. 
		num_dofs -= num_locked_joints * 3;
	}

	ASSERT((num_coords + num_dofs) < max_dofs);

// Set up initial orientations.
	p[0] = ptemp[0] = ENG->get_position(root);
	R[0] = Rtemp[0] = ENG->get_orientation(root);
	for (U32 i = 0; i < num_joints; i++)
	{
		MODEL->get_joint_data(joints[i], &data[i].w);
		const Joint * joint = MODEL->get_joint(joints[i]);
		p[i+1] = ENG->get_position(joint->child);
		R[i+1] = ENG->get_orientation(joint->child);
	}

	int eff_idx = num_joints;
	Vector pstart = p[eff_idx];

// Compute IK goal position and orientation.
	Vector pgoal;
	Matrix Rgoal;

	if (compute_position)
	{
		pgoal = point;
	}
	if (compute_orientation)
	{
		Rgoal = orient;
	}

	bool out_of_whack = false;
	float dp_mag;
	if (compute_position)
	{
		Vector dp_total = pgoal - p[eff_idx];
		dp_mag = dp_total.magnitude();
		out_of_whack = (dp_mag > 1e-3);
	}

	float dR_mag;
	Quaternion qgoal(Rgoal);
	if (!out_of_whack && compute_orientation)
	{
		Quaternion qcurr(R[eff_idx]);
		Vector dR_total = qrel(qcurr, qgoal);
		dR_mag = dR_total.magnitude();
		out_of_whack = (dR_mag > 1e-2);
	}

	if (!out_of_whack)
	{
		return;
	}

	int R_row = (compute_position) ? 3 : 0;

	int num_steps = 1;
	float step_size = 1.0 / num_steps;

	float percent_goal = 0.0;

	Vector dpos;

	Mat<max_dofs> J(num_coords, num_dofs);
	Mat<max_dofs> Jt(num_dofs, num_coords);

	Mat<max_dofs> Jdamp(num_coords + num_dofs, num_dofs);
	Vec<max_dofs> dtheta(num_dofs);
	Vec<max_dofs> delta_x(num_coords);

//U32 last_joint = (compute_orientation) ? num_joints : num_joints - 1;
	U32 last_joint = num_joints;

	bool done = false;
	while (!done)
	{
	// Construct Jacobian matrix.

		float t = 2.0;
		float err;
		float last_err = FLT_MAX;
		float delta_err = 100;

		Vector temp_goal;

	// BUILD JACOBIAN.
		int x = 0;

//U32 end_joint = (compute_orientation) ? num_joints+1 : num_joints;
		U32 end_joint = num_joints+1;
		for (i = 1; i < end_joint; i++)
		{
			if (compute_position)
			{
// THIS IS BOGUS.
				Vector r;
				const Joint * jnext = MODEL->get_joint(joints[i]);
				r = R[i] * jnext->parent_point;

			// Copy dual(-r) into appropriate block of Jacobian.
				if (locked[i-1])
				{
				// add in to previous block.
					int y = x-3;

					J.d[0][y+1] +=  r.z;
					J.d[0][y+2] += -r.y;

					J.d[1][y+0] += -r.z;
					J.d[1][y+2] +=  r.x;

					J.d[2][y+0] +=  r.y;
					J.d[2][y+1] += -r.x;
				}
				else
				{
					J.d[0][x+0] =
					J.d[1][x+1] =
					J.d[2][x+2] = 0;

					J.d[0][x+1] =  r.z;
					J.d[0][x+2] = -r.y;

					J.d[1][x+0] = -r.z;
					J.d[1][x+2] =  r.x;

					J.d[2][x+0] =  r.y;
					J.d[2][x+1] = -r.x;
				}
			}

		// do orientation part:
			if (compute_orientation)
			{
				if (!locked[i-1])
				{
					J.d[R_row+0][x+0] = 1.0;
					J.d[R_row+0][x+1] = 0.0;
					J.d[R_row+0][x+2] = 0.0;

					J.d[R_row+1][x+0] = 0.0;
					J.d[R_row+1][x+1] = 1.0;
					J.d[R_row+1][x+2] = 0.0;

					J.d[R_row+2][x+0] = 0.0;
					J.d[R_row+2][x+1] = 0.0;
					J.d[R_row+2][x+2] = 1.0;
				}
			}

			if (!locked[i-1])
			{
				x += 3;
			}
		}

	// jacobian is built. invert it using SVD.
		Jt = J.get_transpose();

		Jdamp = Mat<max_dofs>::mul(Jt, J);
		//const float damping_factor = 1;
		float dsq = square(damping_factor);
		for (i = 0; i < Jdamp.m; i++)
		{
			for (U32 j = 0; j < Jdamp.n; j++)
			{
				if (i == j)
				{
					Jdamp.d[i][j] += dsq;
				}
			}
		}

		int rnk;
		Mat<max_dofs> Jdamp_inv = Jdamp.get_pseudo_inverse(rnk);

		Vec<max_dofs> secondary_task(num_dofs);
		bool secondary = ((flags & AimDesc::AD_IGNORE_LIMITS) == 0);

		if (secondary)
		{
		// Compute secondary task in joint space.
			int sec_idx = 0;
		
			for (i = 0; i < last_joint; i++)
			{
				if (!locked[i])
				{
					Matrix Rsec = R[i] * Rmid[i];

				// Compute angular difference between current pose and midpoint:
					Quaternion q0(R[i+1]);
					Quaternion q1(Rsec);
					Vector omega = qrel(q0, q1);
					secondary_task.d[sec_idx+0] = omega.x;
					secondary_task.d[sec_idx+1] = omega.y;
					secondary_task.d[sec_idx+2] = omega.z;
					sec_idx += 3;
				}
			}
		}
									 
		do
		{
			t *= 0.5;

		// Compute current offset.
			dpos = pgoal - p[eff_idx];

		// Compute desired change in end-effector state.
			dpos /= _MAX(num_steps, 1);
			num_steps--;

			Vec<max_dofs> dx(num_coords);

			if (compute_position)
			{
				dx.d[0] = dpos.x;									
				dx.d[1] = dpos.y;
				dx.d[2] = dpos.z;
			}

			if (compute_orientation)
			{
				Vector omega = qrel(Quaternion(R[eff_idx]), qgoal);//2.0 * qlog(qgoal * Quaternion(R[eff_idx]).get_conjugate());
				dx.d[R_row+0] = omega.x;
				dx.d[R_row+1] = omega.y;
				dx.d[R_row+2] = omega.z;
			}

		// Take step toward goal:

			Vec<max_dofs> Jtdx = Jt.mul(dx);

			if (secondary)
			{
				const float alpha = 0.25;
				float damp_alpha = damping_factor * alpha;
				for (i = 0; i < Jtdx.len; i++)
				{
					Jtdx.d[i] += damp_alpha * secondary_task.d[i];
				}
			}

			dtheta = Jdamp_inv.mul(Jtdx);

		// Compute ACTUAL dx that dtheta gives us.
			Vector p0 = p[eff_idx];

			int x = 0;
			Vector omega;
			for (i = 0; i < last_joint; i++)
			{
				const Joint * joint = MODEL->get_joint(joints[i]);

				if (!locked[i])
				{
				// Pull omega from dtheta.
					omega.set(dtheta[x], dtheta[x+1], dtheta[x+2]);
					x += 3;
				}

			// Compute new R[i+1] after rotation by omega.
				Matrix Rprime(qexp(0.5 * omega) * R[i+1]);

			// Set new R[i+1].
				Rtemp[i+1] = Rprime;

				ptemp[i+1] = ptemp[i] + Rtemp[i] * joint->parent_point;
			}

		
			if (compute_position)
			{
				Vector dp = ptemp[eff_idx] - p0;
				delta_x.d[0] = dp.x;
				delta_x.d[1] = dp.y;
				delta_x.d[2] = dp.z;
			}
			if (compute_orientation)
			{
				Quaternion qcurr(Rtemp[eff_idx]);
				Vector dR = qrel(qcurr, qgoal);//2.0 * qlog(qgoal * qcurr.get_conjugate());
				delta_x.d[R_row+0] = dR.x;
				delta_x.d[R_row+1] = dR.y;
				delta_x.d[R_row+2] = dR.z;
			}

		// Do error check:
			err = 0;
			for (i = 0; i < num_coords; i++)
			{
				err += square(dx[i] - delta_x[i]);
			}

			delta_err = err - last_err;
			last_err = err;
		
		// Don't continue if error isn't decreasing; we're probably in a singular configuration.
		} while ((err > 1e-2) && (delta_err < -(1e-5)));

	// We may have taken a smaller step than desired.
		percent_goal += t * step_size;
		t = 2.0;

		step_size = _MIN(step_size, 1.0f - percent_goal);

		done = (fabs(percent_goal - 1.0) <= 1e-2) || (delta_err >= 0);
	}

// check current dist against prev dist. If no decrease, assume singlar configuration, 
// don't update joint data.

//	if (dist <= (dp_mag + 1e-4))
	{
		for (i = 0; i < last_joint; i++)
		{
		// Extract joint parameter, relative to R[i].
			if (!locked[i])
			{
				data[i] = Quaternion(Rtemp[i].get_transpose() * Rtemp[i+1]);
			}
		}
	}
}



//


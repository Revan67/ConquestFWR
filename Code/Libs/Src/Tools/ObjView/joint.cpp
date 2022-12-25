#include <windows.h>

#include "FDUMP.h"
#include "TSmartPointer.h"

#include "CmpndView.h"

//joint motion
float JointChangeRate = 20.0 * MUL_DEG_TO_RAD;
float JointCurPos = 0.0;

//RATE OF joint motion control
const float JointMotionChangeRate = 15.0f * MUL_DEG_TO_RAD;

void UpdateJointMotionChangeRate (bool inc, float dt)
{
	float delta = JointMotionChangeRate * dt;

	if (inc)
		JointChangeRate += delta;
	else 
		JointChangeRate -= delta;

	if (JointChangeRate < 1.0 * MUL_DEG_TO_RAD)
		JointChangeRate = 1.0 * MUL_DEG_TO_RAD;
}

void SetJointsToExtreme (INSTANCE_INDEX idx, bool min)
{
	JOINT_INDEX jnt_idx = -1;
	
	while (-1 != (jnt_idx = Engine->get_instance_child_next( idx, 0, jnt_idx )))
	{
		const JointInfo *ji;
		if( (ji = Engine->get_joint_info( jnt_idx )) != NULL ) {

			switch (ji->type)
			{
				case JT_PRISMATIC:
				case JT_REVOLUTE:
				{
					SINGLE d = (min) ? ji->min0 : ji->max0;
					Engine->set_joint_state( jnt_idx, IE_JST_BASIC, &d );
					break;
				}
			}
		}
	}
}

void ResetJoints (INSTANCE_INDEX idx)
{
	JOINT_INDEX jnt_idx = -1;
	
	while (-1 != (jnt_idx = Engine->get_instance_child_next(idx, 0, jnt_idx)))
	{
		const JointInfo *ji;
		if( (ji = Engine->get_joint_info( jnt_idx )) != NULL ) {

			switch (ji->type)
			{
				case JT_PRISMATIC:
				case JT_REVOLUTE:
				{
					SINGLE d = 0.0f;
					Engine->set_joint_state( jnt_idx, IE_JST_BASIC, &d );
					break;
				}
				case JT_SPHERICAL:
				{
					Quaternion d;
					d.set_identity ();
					Engine->set_joint_state( jnt_idx, IE_JST_BASIC, (float*)&d );
					break;
				}
			}
		}
	}
}

void MatrixFromEulerAngles(Matrix & R, float x, float y, float z)
{
	double sinx = sin(x);
	double cosx = cos(x);

	double siny = sin(y);
	double cosy = cos(y);

	double sinz = sin(z);
	double cosz = cos(z);

	R.d[0][0] = cosy * cosz;
	R.d[0][1] = siny * sinx * cosz - cosx * sinz;
	R.d[0][2] = siny * cosx * cosz + sinx * sinz;

	R.d[1][0] = cosy * sinz;
	R.d[1][1] = siny * sinx * sinz + cosx * cosz;
	R.d[1][2] = siny * cosx * sinz - sinx * cosz;

	R.d[2][0] = -siny;
	R.d[2][1] = cosy * sinx;
	R.d[2][2] = cosy * cosx;
}

void DoExerciseJoints (INSTANCE_INDEX obj, float dt)
{
	JOINT_INDEX jnt_idx = -1;
	
	while (-1 != (jnt_idx = Engine->get_instance_child_next(obj, 0, jnt_idx)))
	{
		const JointInfo *jnt ;

		if( (jnt = Engine->get_joint_info( jnt_idx )) != NULL ) {
			switch (jnt->type)
			{
				case JT_PRISMATIC:
				case JT_REVOLUTE:
				{
					JointCurPos += JointChangeRate * dt;

					float theta = sin (JointCurPos);

					SINGLE d;

					if (theta >= 0.0)
						d = theta * jnt->max0;
					else
						d = fabs (theta) * jnt->min0;

					Engine->set_joint_state( jnt_idx, IE_JST_BASIC, &d );
					break;
				}
				case JT_SPHERICAL:
				{
					JointCurPos += JointChangeRate * dt;

					float a0 = sin(JointCurPos);
					float a1 = sin(2.0/3 * JointCurPos);
					float a2 = sin(3.0/5 * JointCurPos);

					float d0 = (a0 >= 0.0) ? a0 * jnt->max0 : -a0 * jnt->min0;
					float d1 = (a1 >= 0.0) ? a1 * jnt->max1 : -a1 * jnt->min1;
					float d2 = (a2 >= 0.0) ? a2 * jnt->max2 : -a2 * jnt->min2;

					Matrix R;
					MatrixFromEulerAngles(R, d0, d1, d2);
					Quaternion q(R);

					Engine->set_joint_state( jnt_idx, IE_JST_BASIC, (float*)&q );

					break;
				}
			}
		}
	}
}


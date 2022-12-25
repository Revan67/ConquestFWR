// JointState.h
//
//
//

#ifndef __JointState_h__
#define __JointState_h__

//

//
// JointState
//
// Encapsulates the current state of a joint.
//
struct JointState
{
	union
	{
		// Joint variable and it's derivative for 1-DOF joints.
		//
		struct
		{
			SINGLE q;
			SINGLE q_dot;
		};

		// Variables and derivatives for cylindrical 2-DOF joints.
		//
		struct
		{
			SINGLE p;		// prismatic component.
			SINGLE p_dot;
			SINGLE r;		// revolute.
			SINGLE r_dot;
		};

		struct
		{
			SINGLE px;	// Translation vector for TRANSLATIONAL(3-DOF) & LOOSE(6-DOF) joints
			SINGLE py;
			SINGLE pz;

			SINGLE w;	// Quaternion for spherical joints
			SINGLE x;
			SINGLE y;
			SINGLE z;
		};
		
	};
};

//

#endif // EOF

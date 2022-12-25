//
//
//

#ifndef ABM_H
#define ABM_H

//

#include "3dmath.h"
#include "JointInfo.h"

//

#include "spatial.h"

//

struct ABMLink
{
	S32				index;

	SINGLE			q;			// Scalar position of inboard joint.
	SINGLE			q_dot;		// Scalar velocity of inboard joint.
	SINGLE			q_dot_dot;	// Scalar accleration of inboard joint.

	Vector			nu;			// Vector velocity of inboard joint.

	Matrix			R_rel;		// Rotation matrix from previous link's 
								// coordinates to this link's coordinates.

	Vector			r;			// Vector from inboard joint to link center
								// of mass, expressed in this link's coordinates.

	Vector			v;			// Linear velocity.
	Vector			omega;		// Angular velocity.

	Vector			a;			// Linear acceleration.
	Vector			alpha;		// Angular acceleration.

	Vector			u;			// Unit vector in direction of the axis of
								// link's inboard joint.

	Vector			d;			// Some vector from the axis of link's 
								// inboard joint to the link's position.

	SINGLE			Q;			// Joint actuator force.
#if 1
	SpatialMatrix	IA;			// Spatial articulated inertia.
#else
	SpatialABI		IA;
#endif

	SpatialVector	ZA;			// Spatial articulated zero-acceleration force.
	SpatialVector	YA;			// Spatial articulated impulse.

	SpatialVector	as;			// Spatial acceleration.
	SpatialVector	ss;			// Spatial joint axis of inboard joint.
	SpatialVector	cs;			// Spatial Coriolis force.

	SINGLE			sIs;
	SINGLE			Qs;

	float			delta_q_dot;
	SpatialVector	delta_v;

	Instance *		owner;
	ABMLink *		parent;
	const JointInfo * to_parent;

	bool			at_limit;

	struct IJointDriver * driver;
};

//

#endif
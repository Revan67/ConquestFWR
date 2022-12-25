//
//
//

#ifndef CONNECT_H
#define CONNECT_H

/*
	$Header: /Conquest/Libs/Include/connect.h 5     4/28/00 11:57p Rmarr $
*/

//
#ifndef _3DMATH_H
#include "3dmath.h"
#endif

//

typedef enum
{
	JT_FIXED,
	JT_PRISMATIC,
	JT_REVOLUTE
} JointType;

//

struct ConnectionInfo
{
	JointType	type;
	S32			size;
};

//

struct FixedChildInfo : public ConnectionInfo
{
	Vector	relative_position;
	Matrix	relative_orientation;

	FixedChildInfo (void)
	{
		type = JT_FIXED;
		size = sizeof (*this);
	}

	FixedChildInfo(const Vector & rel_pos, const Matrix & rel_orient)
	{
		type = JT_FIXED;
		size = sizeof(*this);

		relative_position = rel_pos;
		relative_orientation = rel_orient;
	}
};

//

#endif
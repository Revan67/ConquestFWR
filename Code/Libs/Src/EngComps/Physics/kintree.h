//
//
//

#ifndef KINTREE_H
#define KINTREE_H

//

#include "stddat.h"
#include "connect.h"
#include "engcomp.h"
#include "joint.h"
#include "ode.h"

//

//
// Link is the structure that holds the connectivity tree.
//
struct Link
{
	Instance *		owner;
	Link *			parent;
	Link *			sibling;
	Link *			child;

	virtual Vector get_relative_position(void) const = 0;
	virtual Matrix get_relative_orientation(void) const = 0;
};

//

//
// WE MAY WANT TO MAKE KinematicTree implement the IODE interface rather than
// RigidBodyPhysics...
//
struct KinematicTree : public IODE
{
	Instance * 		root;

	JointList		joints;

	KinematicTree *	prev;
	KinematicTree * next;

	void compute_velocities(void);
	void fixup(void);

// IODE methods.
	virtual S32		get_y_length(void);
	virtual void	get_y(SINGLE * dst, SINGLE t);
	virtual void	get_dydt(SINGLE * dst, SINGLE * y, SINGLE t);
	virtual void	set_y(SINGLE * src);

	virtual SINGLE	get_t(void) const { return 0.0; }
	virtual void	set_t(SINGLE time) {}
};

//

typedef LList<KinematicTree> KinematicTreeList;

//

#endif
//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//

#include "kintree.h"
#include "instance.h"

//

U32 Joint::hash(const void * joint)
{
	Joint * j = (Joint *) joint;
	int small, large;
	if (j->inboard_link < j->outboard_link)
	{
		small = j->inboard_link->index;
		large = j->outboard_link->index;
	}
	else
	{
		small = j->outboard_link->index;
		large = j->inboard_link->index;
	}

	return ((small & 0x0f) << 4) + (large & 0x0f);
}

//

BOOL32 Joint::compare(const void * joint)
{
	Joint * j = (Joint *) joint;
	int din = j->inboard_link->index;
	int dout = j->outboard_link->index;

	int sin = inboard_link->index;
	int sout = outboard_link->index;

	BOOL32 result = ((din == sin) && (dout == sout)) ||
					 ((din == sout) && (dout == sin));

	return result;
}

//

static Matrix ComputeRelativeTransform(const Matrix & source_to_world, const Matrix & dest_to_world)
{
	return dest_to_world.get_transpose() * source_to_world;
}

//

void KinematicTree::compute_velocities(void)
{
	int idx = 0;
	JointNode * node = joints.first();
	while (node)
	{
		Joint * j = node->joint;

		Instance * link_h = j->inboard_link;
		Instance * link_i = j->outboard_link;

		Matrix R = ComputeRelativeTransform(link_h->R, link_i->R);
		Vector r = link_i->R.get_transpose() * (link_i->x - link_h->x);

	// STORE VECTORS IN INSTANCE FRAME.
		//link_i->w = R * link_h->w;
		//link_i->v = R * link_h->v + cross_product(link_i->w, r);

	// STORE VECTORS IN GLOBAL FRAME.
		link_i->set_angular_velocity(link_h->w);
		link_i->set_velocity(link_h->v + cross_product(link_i->w, r));

		switch (j->type)
		{
			case JT_FIXED:
				break;

			case JT_PRISMATIC:
				//link_i->v += link_i->q_dot * link_i->u;
				break;

			case JT_REVOLUTE:
				//link_i->omega += link_i->q_dot * link_i->u;
				//link_i->v += link_i->q_dot * cross_product(link_i->u, link_i->d);
				break;
		}

		idx++;
		node = node->next;
	}
}

//

S32	KinematicTree::get_y_length(void)
{
	S32 result = root->get_state_length();
	return result;
}

//

void KinematicTree::get_y(SINGLE * dst, SINGLE t)
{
	root->get_state_vector(dst);
}

//

void KinematicTree::get_dydt(SINGLE * dst, SINGLE * y, SINGLE t)
{
	if (y)
	{
		set_y(y);
	}

	root->get_derivative_vector(dst);
}

//

void KinematicTree::set_y(SINGLE * src)
{
	root->set_state_vector(src);
	fixup();
}

//

void KinematicTree::fixup(void)
{
	compute_velocities();

	JointNode * node = joints.first();
	while (node)
	{
		Joint * j = node->joint;
		if (j->index != -1)
		{
			Instance * parent	= j->inboard_link;
			Instance * child	= j->outboard_link;
			switch (j->type)
			{
				case JT_FIXED:
					child->x = parent->x + parent->R * j->data.offset;
					child->q = j->data.orient * parent->get_orientation();
					child->R = child->q;
					break;
			}
		}
		node = node->next;
	}
}

//
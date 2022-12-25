//
// MotionStack methods.
//

#include <stdio.h>
#include "Channel.h"
#include "Pools.h"
#include "MStack.h"
#include "fdump.h"

//

void DebugPrint (char *fmt, ...)
{
	if (fmt)
	{
		char work[256];

		va_list va;
		va_start(va,fmt);
		vsprintf(work,fmt,va);
		va_end(va);

		OutputDebugString(work);
	}
}

//

static float RampTable[256];

//

float Ramp(int t)
{
	static bool init = false;
	if (!init)
	{
		float * r = RampTable;
		for (int i = 0; i < 256; i++, r++)
		{
			float d = 3.14159 * i / 255;
			*r = (1.0 - cos(d)) / 2.0;
		}

		init = true;
	}

	return RampTable[t];
}

//

using namespace Channel;

//

void MotionStack::push(MotionStackNode * node)
{
	ASSERT(index < MAX_STACK_ENTRIES-1);
	list[index++] = node;
}

//

MotionStackNode * MotionStack::pop(void)
{
	MotionStackNode * result;
	if (index)
	{
		index--;
		result = list[index];
	}
	else
	{
	// Stack empty.
		result = NULL;
	}
	return result;
}

//

void MotionStack::remove(U32 idx)
{
// TODO: Rearrange motion stack to eliminate object in question.
}

//

void MotionStack::evaluate(void * dst, int data_size)
{
	MotionStackNode * node = pop();
	if (node)
	{
		node->evaluate(dst, *this);
	}
}

//

int ChannelNode::update(float dt)
{
	return object->update(dt);
}

//

void ChannelNode::evaluate(void * dst, MotionStack & stack) const
{
	object->get_last_result(dst);
}

//

int TransitionNode::update(float dt)
{
	time += dt;
	int result = (time >= duration);
	return result;
}

//
// Recursively evaluate transitions.
//
void TransitionNode::evaluate(void * dst, MotionStack & stack) const
{
	MotionStackNode * m1 = stack.pop();
	MotionStackNode * m0 = stack.pop();

	float u0[7], u1[7];

	m0->evaluate(u0, stack);
	m1->evaluate(u1, stack);

// BLEND.
	int param = (duration > 0) ? (int) floor(time * inv_duration * 255) : 255;
	float t = Ramp(param);

	ChannelNode * chan = (ChannelNode *) ((m1->type == MSN_CHANNEL) ? m1 : m0);

// NEED TO BE ABLE TO BLEND BETWEEN 2 POSE NODES.

// do normal blend to get orientation:
	chan->object->blend(dst, u0, u1, t);

	if (special_blend)
	{
		Vector * v = (Vector *) dst;
		float height;
		if (chan->object->flags & XLAT_BLEND_X)
		{
			height = v->x;
		}
		else if (chan->object->flags & XLAT_BLEND_Y)
		{
			height = v->y;
		}
		else if (chan->object->flags & XLAT_BLEND_Z)
		{
			height = v->z;
		}
		else
		{
			//DebugPrint("error\n");
		}

		Vector offset = time * (v0 + 0.5 * dv * inv_duration * time);

		*v = pstart + offset;

		if (chan->object->flags & XLAT_BLEND_X)
		{
			v->x = height;
		}
		else if (chan->object->flags & XLAT_BLEND_Y)
		{
			v->y = height;
		}
		else if (chan->object->flags & XLAT_BLEND_Z)
		{
			v->z = height;
		}
		else
		{
			//DebugPrint("error\n");
		}
	}
}

//

PoseNode::PoseNode(const Target & t)
{
	type = MSN_POSE;
	switch (t.type)
	{
		case JOINT:
		{
			const JointInfo *ji;
			if( (ji = Engine->get_joint_info( t.joint )) != NULL ) {
				data_size = ji->get_state_vector_size();
				Engine->get_joint_state( t.joint, IE_JST_BASIC, data );
			}
			break;
		}
		case OBJECT:
			data_size = 7;
			*((Vector *) data) = Engine->get_position(t.object);
			*((Quaternion *) (data+3)) = Quaternion(Engine->get_orientation(t.object));
			break;
		default:
			data_size = 0;
			break;
	}
}

//

PoseNode::PoseNode(const ChannelNode * chan)
{
	ASSERT(chan);

	type = MSN_POSE;
	switch (chan->object->get_target().type)
	{
		case JOINT:
		{
			const JointInfo *ji;
			if( (ji = Engine->get_joint_info( chan->object->get_target().joint )) != NULL ) {
				data_size = ji->get_state_vector_size();
			}
			break;
		}
		case OBJECT:
			data_size = 7;
			break;
		default:
			data_size = 0;
			break;
	}

	MotionStack bogus;
	chan->evaluate(&data, bogus);
}

//

int PoseNode::update(float dt)
{
	return 0;
}

//

void PoseNode::evaluate(void * dst, MotionStack & stack) const
{
	if (data_size)
	{
		ASSERT(dst);
		memcpy(dst, data, data_size * sizeof(float));
	}
}

//
#ifndef MNODE_H
#define MNODE_H

//

#include "Stddat.h"
#include "channel.h"

using namespace Channel;

//

typedef enum
{
	MSN_CHANNEL,
	MSN_TRANSITION,
	MSN_POSE
} MSNType;

//

struct MotionStackNode
{
	MSNType				type;
	MotionStackNode *	prev;
	MotionStackNode *	next;
	bool				delete_me;

	MotionStackNode(void)
	{
		prev = next = 0;
		delete_me = false;
	}

	virtual int update(float dt) = 0;
	virtual void evaluate(void * dst, struct MotionStack & stack) const = 0;

	bool is_transition(void) const
	{
		return (type == MSN_TRANSITION);
	}

	bool is_transition_source(void) const
	{
		return (next && next->next && next->next->type == MSN_TRANSITION);
	}

	bool is_transition_destination(void) const
	{
		return (next && next->type == MSN_TRANSITION);
	}

	virtual void start_blend(float dur) {}
	virtual Vector get_translation(float dt) const = 0;
};

//

struct ChannelNode : public MotionStackNode
{
	class Object * object;
	ChannelNode(Object * _obj)
	{
		type = MSN_CHANNEL;
		object = _obj;
	}

	virtual int update(float dt);
	virtual void evaluate(void * dst, struct MotionStack & stack) const;

	virtual void start_blend(float dur)
	{
		object->start_blend(dur);
	}

	virtual Vector get_translation(float dt) const
	{
		return object->get_translation(dt);
	}
};

//

struct TransitionNode : public MotionStackNode
{
	float	time;
	float	duration;
	float	inv_duration;

	bool	remove_source:1;

// Hack special-case blending for root translation.
	bool	special_blend:1;
	Vector	pstart;
	Vector	v0, dv;

	TransitionNode(void)
	{
		type = MSN_TRANSITION;
		time = duration = 0;
		remove_source = true;

		special_blend = false;
	}

	TransitionNode(float dur, bool remove = true)
	{
		type = MSN_TRANSITION;
		time = 0;
		duration = dur;
		inv_duration = 1.0/dur;
		remove_source = remove;

		special_blend = false;
	}

	virtual int update(float dt);
	virtual void evaluate(void * dst, struct MotionStack & stack) const;

	bool is_valid_transition(void) const
	{
		return (prev && (prev->type == MSN_CHANNEL) && prev->prev);
	}

	virtual Vector get_translation(float dt) const
	{
		return prev->get_translation(dt);
	}
};

//

struct PoseNode : public MotionStackNode
{
// ASSUMES FullStream is as big as it gets:
	int		data_size;
	float	data[7];

	PoseNode(const PoseNode & pose)
	{
		type = MSN_POSE;
		data_size = pose.data_size;
		memcpy(data, pose.data, data_size * sizeof(float));
	}

	PoseNode(const Animation::Target & t);
	PoseNode(const ChannelNode * chan);

	virtual int update(float dt);
	virtual void evaluate(void * dst, struct MotionStack & stack) const;

	virtual Vector get_translation(float dt) const
	{
		return Vector(0, 0, 0);
	}
};

//

#endif
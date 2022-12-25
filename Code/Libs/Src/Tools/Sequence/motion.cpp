#include "stdafx.h"

#include <IAnim.h>
#include <Deform.h>
#include "motion.h"
#include "character.h"

//

const char * MotionTypeNames[MT_NUM_MOTION_TYPES] =
{
	"MT_NONE",
	"MT_IDLE",
	"MT_STOP",
	"MT_WALK",
	"MT_RUN",
	"MT_TURN_LEFT",
	"MT_TURN_RIGHT",
	"MT_MOVE_LEFT",
	"MT_MOVE_RIGHT",
	"MT_IDLE_SPECIAL",
	"MT_WALK_SPECIAL",
	"MT_TURN_LEFT_SPECIAL",
	"MT_TURN_RIGHT_SPECIAL",
	"MT_DRAW_WEAPON",
	"MT_HOLSTER_WEAPON",
	"MT_FIRE_WEAPON",
	"MT_AIM_WEAPON",
	"MT_DIE",
	"MT_ATTACK0",
	"MT_ATTACK1",
	"MT_ATTACK2",
	"MT_ATTACK3"
};

//

MotionSequence::~MotionSequence(void)
{
	if (num_wait_states)
	{
		for (int i = 0; i < num_wait_states; i++)
		{
			free(wait_events[i]);
			wait_events[i] = NULL;
			free(wait_targets[i]);
			wait_targets[i] = NULL;
		}

		if (wait_events)
		{
			delete [] wait_events;
			wait_events = NULL;
		}

		if (wait_targets)
		{
			delete [] wait_targets;
			wait_targets = NULL;
		}
	}

	free((void *) name);
	name = NULL;
	free((void *) script);
	script = NULL;
}

//

bool MotionSequence::start(Character * c, int which) const
{
	bool result = false;

    float time_scale = 1.0f;//(BruteApp.m_input->key_down(DIK_RCONTROL)) ? 3.0 : 1.0;

//DebugPrint("starting %s [%s]\n", name, MotionTypeNames[type]);
	U32 flags = ((reverse) ? Animation::BACKWARDS : Animation::FORWARD) | ((loop) ? Animation::LOOP : 0);
	float start_time = (reverse) ? Animation::END : Animation::BEGIN;
	if (num_wait_states)
	{
		assert(which >= 0 && which < num_wait_states);
		result = c->deform->start_motion(	wait_targets[which], start_time, transition_duration, time_scale, 1.0, flags);

		assert(result);
	}
	else
	{
		result = c->deform->start_motion(script, start_time, transition_duration, time_scale, 1.0, flags);

		assert(result);
	}

	c->current_sequence = this;
	c->pending_sequence = NULL;

	if (num_wait_states)
	{
		c->change_state = -1;
	}

	return result;
}

//

void MotionSequence::update(Character * c) const
{
}

//
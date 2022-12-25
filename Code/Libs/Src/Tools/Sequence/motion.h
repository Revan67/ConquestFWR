#ifndef MOTION_H
#define MOTION_H

//
// Change to base motion + modifier. Use high bits for modifiers.
//
typedef enum
{
	MT_NONE,
	MT_IDLE,
	MT_STOP,
	MT_WALK,
	MT_RUN,
	MT_TURN_LEFT,
	MT_TURN_RIGHT,
	MT_MOVE_LEFT,
	MT_MOVE_RIGHT,
	MT_IDLE_SPECIAL,
	MT_WALK_SPECIAL,
	MT_TURN_LEFT_SPECIAL,
	MT_TURN_RIGHT_SPECIAL,
	MT_DRAW_WEAPON,
	MT_HOLSTER_WEAPON,
	MT_FIRE_WEAPON,
	MT_AIM_WEAPON,
	MT_DIE,
	MT_ATTACK0,
	MT_ATTACK1,
	MT_ATTACK2,
	MT_ATTACK3,
	MT_NUM_MOTION_TYPES
} MotionType;

//

typedef enum
{
	MM_NORMAL = 0,
	MM_SPECIAL0
} MotionModifier;

//

extern const char * MotionTypeNames[MT_NUM_MOTION_TYPES];

//

struct MotionSequence
{
	BOOL				bVisited;
	MotionType			type;
	const char *		name;
	const char *		script;
	MotionSequence *	end;

	bool				loop:1;
	bool				reverse:1;
	bool				interrupt:1;
	float				transition_duration;

	int					num_wait_states;
	char **				wait_events;
	char **				wait_targets;


	MotionSequence(void)
	{
		memset(this, 0, sizeof(*this));
	}

	~MotionSequence(void);

	bool start(class Character * c, int which = 0) const;
	//void notify(Character * c, MotionChangeType change, void * data) const;
	void update(Character * c) const;
};

//

#endif
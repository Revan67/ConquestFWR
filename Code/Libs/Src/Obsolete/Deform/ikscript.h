#ifndef IKSCRIPT_H
#define IKSCRIPT_H

//

#include <stdlib.h>
#include "3DMath.h"
#include "engine.h"
#include "Model.h"

//

struct IKKeyFrame
{
	Vector		p_target;
	Quaternion	R_target;
	float		time;
};

//

#define MAX_IK_JOINTS	8

//

struct IKScriptDesc
{
protected:

	int				num_joints;
	JOINT_INDEX		joints[MAX_IK_JOINTS];

public:

	float			duration;

	INSTANCE_INDEX	root;			// not root of skeleton, but root of IK portion
	INSTANCE_INDEX	end_effector;

	int				num_keyframes;
	IKKeyFrame *	keyframes;

	IKScriptDesc(void)
	{
		duration = 0;
		root = end_effector = INVALID_INSTANCE_INDEX;
		num_keyframes = 0;
		keyframes = NULL;
	}

	IKScriptDesc(float dur, INSTANCE_INDEX rt, INSTANCE_INDEX ee, int n);

	~IKScriptDesc(void)
	{
		delete [] keyframes;
		keyframes = NULL;
	}
};

//

#endif
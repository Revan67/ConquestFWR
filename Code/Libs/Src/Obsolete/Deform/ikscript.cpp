//
//
//

#include "IKScript.h"
#include "Model.h"

//

extern IModel * MODEL;

//

IKScriptDesc::IKScriptDesc(float dur, INSTANCE_INDEX rt, INSTANCE_INDEX ee, int n)
{
	duration		= dur;
	root			= rt;
	end_effector	= ee;
	num_keyframes	= n;
	keyframes		= new IKKeyFrame[num_keyframes];

	INSTANCE_INDEX obj = end_effector;
	while (obj != root)
	{
		INSTANCE_INDEX parent = MODEL->get_parent(obj);
		joints[num_joints++] = MODEL->find_joint(parent, obj);
		obj = parent;
	}
}


void compute_IK(const IKScriptDesc & script, int frame)
{
	IKKeyFrame * key = script.keyframes + frame;

	
}

//
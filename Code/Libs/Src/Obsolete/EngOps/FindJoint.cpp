//$Header: /Libs/Src/EngOps/FindJoint.cpp 2     2/06/98 3:16p Emaurer $

#include "EngOps.h"

namespace EngineOps
{

JOINT_INDEX FindJoint (IModel* model, INSTANCE_INDEX root, const char* parent, const char* child)
{
	JOINT_INDEX result = INVALID_JOINT_INDEX;

	INSTANCE_INDEX pobj = FindChild (model, root, parent);

	if (INVALID_INSTANCE_INDEX != pobj)
	{
		INSTANCE_INDEX cobj = FindChild (model, root, child);

		if (INVALID_INSTANCE_INDEX != cobj)
		{
			result = model->find_joint (pobj, cobj);
		}
	}

	return result;
}

};
//$Header: /Libs/Src/EngOps/EngOps.h 4     7/21/98 7:10p Tleepaul $

#ifndef ENGOPS_H_
#define ENGOPS_H_

#include "Engine.h"
#include "Model.h"
#include "IHardpoint.h"

namespace EngineOps
{
	//'name' can be formed like a DOS path: child0\child1\target.  child0 is assumed to be a child
	//of 'idx', child1 will be a child of child0, etc.

	INSTANCE_INDEX FindChild (IModel* model, INSTANCE_INDEX idx, const char* name);

	JOINT_INDEX FindJoint (IModel* model, INSTANCE_INDEX root, const char* parent, const char* child);

	// search the passed instance and its children for the first hardpoint matching "name"
	//  fill "in_which" with the instance containing the hardpoint 
	//  fill "info" with the hardpoint's info;
	// return info's point and orientation as "root" relative
	// calls MODEL->update_tree on root to validate child state
	bool FindHardpoint( IEngine *, IModel *, IHardpoint *, 
						const INSTANCE_INDEX root, const char* name, 
						INSTANCE_INDEX & in_which, HardpointInfo & info );

	const unsigned int PARENTS = 1;
	const unsigned int CHILDREN = 2;

	unsigned int CountObj (IModel* model, unsigned int which);
};

#endif

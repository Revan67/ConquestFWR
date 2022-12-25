// (c) copyright 1998, Digital Anvil, Inc.
//$Header: /Libs/Src/EngOps/CntObjs.cpp 2     2/06/98 3:16p Emaurer $

#include "EngOps.h"

namespace EngineOps
{

unsigned int CountChildren (IModel* model, INSTANCE_INDEX idx)
{
	unsigned int result = 0;
	INSTANCE_INDEX child = -1;

	while (INVALID_INSTANCE_INDEX != (child = model->get_child (idx, child)))
	{
		result++;
		result += CountChildren (model, child);
	}
	
	return result;
}

unsigned int CountObj (IModel* model, unsigned int which)
{
	unsigned int result = 0;

	if (which & PARENTS)
	{
		INSTANCE_INDEX parent = -1;
		
		while (INVALID_INSTANCE_INDEX != (parent = model->traverse_roots (parent)))
			result++;		
	}

	if (which & CHILDREN)
	{
		INSTANCE_INDEX parent = -1;
		
		while (INVALID_INSTANCE_INDEX != (parent = model->traverse_roots (parent)))
			result += CountChildren (model, parent);
	}

	return result;
}

};
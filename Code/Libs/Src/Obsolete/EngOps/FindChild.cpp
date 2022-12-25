//$Header: /Libs/Dev/Src/EngOps/FindChild.cpp 3     10/06/99 5:29p Mstembera $

#include "EngOps.h"
#include <string.h>
#include "fdump.h"

namespace EngineOps
{

INSTANCE_INDEX FindChild (IModel* model, INSTANCE_INDEX idx, const char* _name)
{
	INSTANCE_INDEX result = INVALID_INSTANCE_INDEX;

	char* slash = strchr (_name, '\\');

	const char* name = _name;

	#define MAX_PARTNAME 64
	char b[MAX_PARTNAME];

	if (slash)
	{
		ASSERT (slash - _name < MAX_PARTNAME);

		memcpy (b, _name, slash - _name);
		b[slash - _name] = 0;

		name = b;
	}

	INSTANCE_INDEX child = -1;

	while (-1 != (child = model->get_child (idx, child)))
	{
		if ( !strcmp( model->get_name( child ), name) )
		{
			if (slash)
				result = FindChild (model, child, slash + 1);
			else
				result = child;

			break;
		}
	}

	return result;
}

};
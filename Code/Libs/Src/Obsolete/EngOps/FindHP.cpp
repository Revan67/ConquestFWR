#include "engops.h"

namespace EngineOps
{

bool recursive_hpfind( IEngine * ENGINE, IModel * MODEL, IHardpoint * HP,
					const INSTANCE_INDEX root, const C8 * hpName, 
					INSTANCE_INDEX & object, HardpointInfo & result )
{
	ARCHETYPE_INDEX arch = ENGINE->get_archetype( root );

	if (arch == INVALID_ARCHETYPE_INDEX) 
		return false;

	bool found= HP->retrieve_hardpoint_info (arch, hpName, result);
	
	ENGINE->release_archetype( arch );

	if ( found )
	{
		object= root;
		return true;
	}
	
	INSTANCE_INDEX child= INVALID_INSTANCE_INDEX;
	
	while( 1 )
	{
		child= MODEL->get_child( root, child );

		// never found hp in any of the children
		if (child == INVALID_INSTANCE_INDEX)
			return false;
		
		// found hp; return values all setup
		if ( recursive_hpfind( ENGINE, MODEL, HP, child, hpName, object, result ) )
		{
			return true;
		}
	}

	return false;
}

bool FindHardpoint( IEngine * ENGINE, IModel * MODEL, IHardpoint * HP,
					const INSTANCE_INDEX root, const C8 * hpName, 
					INSTANCE_INDEX & object, HardpointInfo & result )
{
	MODEL->update_tree( root );

	if (recursive_hpfind( ENGINE, MODEL, HP, root, hpName, object, result ))
	{
	// move hp from child frame (to the world which the child is in)
		Vector cx = ENGINE->get_position(object);
		Matrix cR = ENGINE->get_orientation(object);

		Vector hp = cx + (cR * result.point);
		Matrix hR = cR * result.orientation;

	// move hp (from the world) into to root frame
		Matrix rinv = ENGINE->get_orientation(root).get_transpose();

		result.point = (rinv * ( hp - ENGINE->get_position(root) ));
		result.orientation = rinv * hR;
		return true;
	}

	return false;
}

}

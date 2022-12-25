#ifndef ARCHHOLDER_H
#define ARCHHOLDER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                ArchHolder.h                              //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ArchHolder.h 2     11/18/99 8:22p Rmarr $
*/			    
//---------------------------------------------------------------------------
/*
	Use this class to keep track of references to archetypes. Automates the 
	release of archetypes after they aren't needed anymore.
*/

#ifndef ENGINE_H
#include <Engine.h>
#endif

//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
class HARCH
{
	ARCHETYPE_INDEX	index;

public:
	HARCH (void)
	{
		index = INVALID_ARCHETYPE_INDEX;
	}

	HARCH (INSTANCE_INDEX inst)
	{
		index = ENGINE->get_instance_archetype(inst);
	}

	~HARCH (void)
	{
		free();
	}

	void free (void)
	{
		if (index >= 0)
		{
			ENGINE->release_archetype(index);
			index = INVALID_ARCHETYPE_INDEX;
		}
	}

	void operator = (INSTANCE_INDEX inst)
	{
		free();
		index = ENGINE->get_instance_archetype(inst);
	}

	ARCHETYPE_INDEX setArchetype (ARCHETYPE_INDEX arch)
	{
		if (arch != index)
		{
			free();
			index = arch;
		}
		return index;
	}
	
	operator ARCHETYPE_INDEX (void) const 
	{
		return index;
	}
};

#endif

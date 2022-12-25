#ifndef __EngineArchetype_h__
#define __EngineArchetype_h__

//

#include "PersistCompound.h"

//

// ..........................................................................
// 
// EngineArchetype
//
// The EngineArchetype list keeps track of the names of archetypes which
// have been loaded by the various engine components.  A successful result
// from an archetype list search indicates that the named archetype has
// already been loaded, and can be used to create object instances without
// incurring delays due to file accesses or other overhead.  
//
struct EngineArchetype
{
public: // Types

	//

	struct EngineArchetypePart
	{
		C8 part_name[PARTNAME_MAX];
		EngineArchetype *part_archetype;
	};

	//

	struct EngineArchetypeJoint
	{
		EngineArchetypePart child_part;
		EngineArchetypePart parent_part;
		JointInfo			joint_info;
	};

	//

	typedef std::list<EngineArchetypePart>	EngineArchetypePartList;
	typedef std::list<EngineArchetypeJoint>	EngineArchetypeJointList;

	//

public: // Data	

	ARCHETYPE_INDEX					arch_index;	// The index of this archetype according to the Engine
	C8								arch_name[MAX_PATH];

	S32								ref_cnt;	// # of references to archetype, starts at 0

	EngineArchetypePartList			parts;		// VERY IMPORTANT: the archetypes in this list will not 
												// necessarily be DIRECTLY connected to this archetype.  
												// (They may be indirectly connected.)

	EngineArchetypeJointList		joints;

public: // Interface
	EngineArchetype( const char *_arch_name=NULL, ARCHETYPE_INDEX _arch_index=INVALID_ARCHETYPE_INDEX );
	EngineArchetype( const EngineArchetype &ea );
	~EngineArchetype( void );
	EngineArchetype &operator=( const EngineArchetype &ea );
	bool operator==( const EngineArchetype &ea );

	S32	add_ref( IEngine *engine );
	S32 release_ref( IEngine *engine );
	EngineArchetype *get_part_archetype( const char *part_name );
};

//

EngineArchetype::EngineArchetype( const char *_arch_name, ARCHETYPE_INDEX _arch_index )
{
	if( _arch_name ) {
		strcpy( arch_name, _arch_name );
	}
	else {
		arch_name[0] = 0;
	}

	arch_index = _arch_index;

	ref_cnt = 0;
}

//

EngineArchetype::EngineArchetype( const EngineArchetype &ea  )
{
	operator=( ea );
}

//

EngineArchetype::~EngineArchetype( void )
{
	strcpy( arch_name, "@@@DELETED@@@" );
}

//

EngineArchetype & EngineArchetype::operator=( const EngineArchetype &ea )
{
	strcpy( arch_name, ea.arch_name );

	arch_index = ea.arch_index;

	ref_cnt = ea.ref_cnt;
	
	parts = ea.parts;
	joints = ea.joints;

	return *this;
}

//

bool EngineArchetype::operator==( const EngineArchetype &ea )
{
	return arch_index == ea.arch_index;
}

//

S32	EngineArchetype::add_ref( IEngine *engine )
{
	ref_cnt++;

	EngineArchetypePartList::iterator part;
	EngineArchetypePartList::iterator pbeg = parts.begin();
	EngineArchetypePartList::iterator pend = parts.end();

	for( part = pbeg; part != pend; part++ ) {
		engine->hold_archetype( part->part_archetype->arch_index );
	}

	return ref_cnt;
}

//

S32 EngineArchetype::release_ref( IEngine *engine )
{
	ref_cnt--;

	EngineArchetypePartList::iterator part;
	EngineArchetypePartList::iterator pbeg = parts.begin();
	EngineArchetypePartList::iterator pend = parts.end();

	for( part = pbeg; part != pend; part++ ) {
		engine->release_archetype( part->part_archetype->arch_index );
	}

	return ref_cnt;
}

//

EngineArchetype *EngineArchetype::get_part_archetype( const char *part_name )
{
	EngineArchetypePartList::iterator part;
	EngineArchetypePartList::iterator pbeg = parts.begin();
	EngineArchetypePartList::iterator pend = parts.end();

	for( part = pbeg; part != pend; part++ ) {
		if( strcmp( part_name, part->part_name ) == 0 ) {
			return part->part_archetype;
		}
	}

	return NULL;
}

//

#endif	// EOF

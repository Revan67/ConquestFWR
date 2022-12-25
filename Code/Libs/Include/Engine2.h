#ifndef ENGINE2_H
#define ENGINE2_H

#include "Engine.h"

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define IENGINE2_VERSION 10
#define IID_IEngine2 MAKE_IID("IEngine2",10)

// -------------------------------------------------------------------------
// IEngine2
//                                                                          
struct DACOM_NO_VTABLE IEngine2 : public IDAComponent
{
	// Used in enumeration callbacks
	//
	typedef bool (*EngineArchetypeCallback2)( ARCHETYPE_INDEX parent_arch_index, ARCHETYPE_INDEX child_arch_index, const JointInfo* joint_info, void *user_data );

	// enumerate_archetype_joints
	//
	// enumerate all joints that make up the connectivity of an archetype as long as callback is
	// returning true.
	// The given callback is called for each joint in the archetype.  In the call,
	// parent_arch_index is the archetype index of the joint parent and 
	// child_arch_index is the joint child, and joint_info is the joint info.
	//
	// If arch_index does not exist in the database, this method returns GR_GENERIC.
	//
	// If arch_index references a non-compound archetype, callback will never be
	// called and this method will return GR_OK.
	//
	// user_data is completely opaque and simply passed through during calls to the 
	// callback.
	//
	virtual GENRESULT COMAPI enumerate_archetype_joints2( ARCHETYPE_INDEX arch_index, EngineArchetypeCallback2 callback, void *user_data ) const = 0;

	// get_archetype_part_name
	//
	// returns the part name of the given archetype if it is a part of a
	// compound archetype, or NULL otherwise.
	//
	virtual const C8* COMAPI get_archetype_part_name2( ARCHETYPE_INDEX arch_index ) const = 0;
};

#endif

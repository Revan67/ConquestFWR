//
// SystemObjects.h
//
// editor defs of objects contained in a sector
//

#include "RuseMap.h"
#include <DQuickSave.h>
#include <list>

namespace Editor
{
	struct ObjectList : std::list<struct IBaseObject*>
	{
	};

	// describes a location of a jump gate in some System
	struct JumpInfo
	{
		JumpInfo() : x(0), y(0), startX(0), startY(0), archID(0), bJumpAllowed(false)
		{
			ZeroMemory( &d, sizeof(d) );
			partName = "";
		}

		// used to be in Jump
		MT_QJGATELOAD d;
		S32 x,y;
		S32 startX,startY;
		U32 archID;
		bool bJumpAllowed;
		M_STRING partName;
	};

	// a SysEntry will contain a list of GateLinks
	struct SysGate : public GateLink
	{
		SysGate() : GateLink(0,0,0)	{}

		// a gate always has at least one valid jump
		JumpInfo jumpA;
		JumpInfo jumpB;
	};

	// this will contain the system kit, gate links, and an object list
	struct SysEntry : public System
	{
		bool          bValid;
		GT_SYSTEM_KIT systemKit;
		ObjectList    objectList;
		
		SysEntry() : bValid(false), System(0,0,0)
		{
			ZeroMemory( &systemKit, sizeof(systemKit) );
		}

		virtual ~SysEntry()
		{
		}
	};
}
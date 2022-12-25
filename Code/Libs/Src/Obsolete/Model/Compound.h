#ifndef __COMPOUND_H
#define __COMPOUND_H

/*
	$Header: /Libs/Dev/Src/EngComps/Model/Compound.h 3     8/03/99 11:08a Emaurer $
*/

#include "Engine.h"
#include "model.h"
#include "stddat.h"
#include "PersistCompound.h"
#include "allocLite.h"

#pragma warning (disable : 4786 4530)	//EMAURER identifier truncated, exception handling
#include <list>

struct IFileSystem;
//

namespace Compound
{
	class Archetype;

	//EMAURER this allocator only works for the MS std::list.
	//It maintains an ever-growing heap of list nodes.
	typedef AllocLite<Archetype*> ARCHETYPE_ALLOC;
	typedef std::list<Archetype*, ARCHETYPE_ALLOC> ARCHETYPE_LIST;

	struct ConnectionDefinition
	{
		Archetype* child;
		Archetype* parent;
		JointInfo connection;
	};

	//EMAURER this allocator only works for the MS std::list
	//It maintains an ever-growing heap of list nodes.
	typedef AllocLite<ConnectionDefinition> CNXNDEF_ALLOC;
	typedef std::list<ConnectionDefinition, CNXNDEF_ALLOC> CNXNDEF_LIST;

//	const int PARTNAME_MAX = PersistCompound::PARTNAME_MAX;

	class Archetype
	{
		public:

			Archetype (const char* _label, ARCHETYPE_INDEX _archetype);

			//VERY IMPORTANT: the archetypes in this list will not necessarily
			//be DIRECTLY connected to this archetype.  they may be indirectly connected

			const ARCHETYPE_LIST& get_children (void) const;
			const CNXNDEF_LIST& get_connections (void) const;

			~Archetype (void);

			const char* get_label (void) const;
			ARCHETYPE_INDEX get_archetype_index (void) const;
			
			static Archetype* Build (IFileSystem* compound_dir, 
										IEngine* engine, 
										IFileSystem* parts=NULL);

			//used to destroy all sub-archetypes that this compound archetype created.
			void decouple (IEngine* engine);

		protected:

			char label[PARTNAME_MAX];
			ARCHETYPE_INDEX archetype;

			CNXNDEF_LIST connections;
			ARCHETYPE_LIST children;

			Archetype* find_part (const char* partname);

			int read_sub_objects (IFileSystem* compound_dir, 
									IEngine* engine, 
									IFileSystem* parts=NULL);

			static Archetype* BuildRoot (IFileSystem* compound_dir, 
											IEngine* engine, 
											IFileSystem* parts=NULL);
	};
};

#endif
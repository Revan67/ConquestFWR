// Hardpoint.cpp
//
//
//

#pragma warning( disable : 4786 )
#pragma warning( disable : 4530 )

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <list>

//

#include "DACOM.h"
#include "TComponent.h"
#include "fdump.h"
#include "tempstr.h"
#include "Matrix.h"
#include "Vector.h"
#include "da_heap_utility.h"
#include "SysConsumerDesc.h"
#include "FileSys_Utility.h"
#include "Engine.h"
#include "EngComp.h"
#include "IHardpoint.h"
#include "ICamera.h"

//

#include "handlemap.h"
#include "Tfuncs.h"

//

#include "PersistHardpoint.h"

//

const unsigned int MAX_HARDPOINT_NAME = 64;

//

struct Hardpoint
{
	char label[MAX_HARDPOINT_NAME];
	HardpointInfo info;

	bool matches (const char* name) const;
};

//

bool Hardpoint::matches (const char* name) const
{
	return (!_strnicmp (name, label, MAX_HARDPOINT_NAME));
}

//

typedef std::list< Hardpoint >					HardpointArchetype;
typedef arch_handlemap< HardpointArchetype >	HardpointArchetypeMap;

//

struct HardPointComponent : public IHardpoint, 
							public IEngineComponent
{
public: // Data
	BEGIN_DACOM_MAP_INBOUND(HardPointComponent)
	DACOM_INTERFACE_ENTRY(IHardpoint)
	DACOM_INTERFACE_ENTRY2(IID_IHardpoint,IHardpoint)
	DACOM_INTERFACE_ENTRY(IEngineComponent)
	DACOM_INTERFACE_ENTRY2(IID_IEngineComponent,IEngineComponent)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()


protected: // Data

	mutable HardpointArchetypeMap archetypes;

	IEngine *engine;

public: // Interface

	HardPointComponent (void);
	~HardPointComponent (void);
	DA_HEAP_DEFINE_NEW_OPERATOR(HardPointComponent);
	GENRESULT init (SYSCONSUMERDESC* desc);

	// IAggregateComponent
	GENRESULT COMAPI Initialize (void);

	// IEngineComponent
	BOOL32 COMAPI create_archetype( ARCHETYPE_INDEX arch_index, struct IFileSystem *filesys ) ;
	void COMAPI	duplicate_archetype( ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index ) ;
	void COMAPI destroy_archetype( ARCHETYPE_INDEX arch_index ) ;
	GENRESULT COMAPI query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif ) ;
	BOOL32 COMAPI create_instance( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index ) ;
	void COMAPI destroy_instance( INSTANCE_INDEX inst_index ) ;
	void COMAPI update_instance( INSTANCE_INDEX inst_index, SINGLE dt ) ;
	enum vis_state COMAPI render_instance( struct ICamera *camera, INSTANCE_INDEX inst_index, float lod_fraction, U32 flags, const Transform *modifier_transform ) ;
	GENRESULT COMAPI query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif ) ;
	void COMAPI update(SINGLE dt) ;

	// IHardpoint
	bool COMAPI retrieve_hardpoint_info( ARCHETYPE_INDEX arch, const char* name, HardpointInfo& result ) const;
	bool COMAPI set_hardpoint_info (ARCHETYPE_INDEX arch, const char* name, const HardpointInfo& hp );
	void COMAPI enumerate_hardpoints( HARDPOINT_ENUM_CALLBACK, ARCHETYPE_INDEX arch, void* misc=0 ) const;
	int COMAPI connect( INSTANCE_INDEX parent, const char* parent_hardpoint, INSTANCE_INDEX child, const char* child_hardpoint );

protected: // Interface
	
	JointType jointtypename_to_jointtype( const char *jointtypename );
};

//

HardPointComponent::HardPointComponent( void ) 
{
}

//

HardPointComponent::~HardPointComponent (void)
{
	HardpointArchetypeMap::const_iterator hpt;

	for( hpt = archetypes.begin(); hpt != archetypes.end(); hpt++) {
		GENERAL_NOTICE( TEMPSTR( "Hardpoint: dtor: Archetype %d has dangling hardpoints\n", (*hpt).first ) ) ;
	}

	engine = NULL;
}

//

GENRESULT HardPointComponent::init( SYSCONSUMERDESC * )
{
	return GR_OK;
}

//

GENRESULT HardPointComponent::Initialize (void)
{
	GENRESULT result = GR_OK;

	if( FAILED( static_cast<IEngineComponent*>( this )->QueryInterface( IID_IEngine, (void**) &engine ) ) ) {
		GENERAL_ERROR( "HardPoint: Initialize: unable to acquire IID_IEngine, cannot continue" );
		return GR_GENERIC;
	}
	engine->Release();

	return GR_OK;
}

//

BOOL32 HardPointComponent::create_archetype( ARCHETYPE_INDEX arch_index, IFileSystem* fs )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( fs != NULL );

	HardpointArchetype archetype ;
	WIN32_FIND_DATA dir_find, hp_find;
	HANDLE hDir, hHP;
	DAFILEDESC desc, hp_desc;

	
	if( !fs->SetCurrentDirectory (HP_ROOT_NAME) ) {
		return FALSE;
	}

	if( (hDir = fs->FindFirstFile( "*", &dir_find )) == INVALID_HANDLE_VALUE ) {
		GENERAL_TRACE_1( "Hardpoint: create_archetype: archetype directory has no entries." );
		return FALSE;
	}

	do	// directory search loop
	{
		if( strcmp( dir_find.cFileName, "." )==0 || strcmp( dir_find.cFileName, ".." )==0 ) {
			continue;
		}

		if( fs->SetCurrentDirectory( dir_find.cFileName ) ) {

			// new style directory layout

			if( (hHP = fs->FindFirstFile( "*", &hp_find )) != INVALID_HANDLE_VALUE ) {
				do {
					if( fs->SetCurrentDirectory( hp_find.cFileName ) ) {

						Hardpoint hardpoint;

						strncpy( hardpoint.label, hp_find.cFileName, MAX_HARDPOINT_NAME );
						hardpoint.label[MAX_HARDPOINT_NAME - 1] = 0;

						hardpoint.info.type = jointtypename_to_jointtype( dir_find.cFileName );

						switch( hardpoint.info.type ) {

						case JT_REVOLUTE:
						case JT_PRISMATIC:
							read_type( fs, "Axis", &hardpoint.info.axis );
							read_type( fs, "Min", &hardpoint.info.min0 );
							read_type( fs, "Max", &hardpoint.info.max0 );

							// note the LACK of a break;

						case JT_FIXED:
							read_type( fs, "Position", &hardpoint.info.point );
							read_type( fs, "Orientation", &hardpoint.info.orientation );
							break;

						default:
							GENERAL_WARNING( "Hardpoint: unknown hardpoint type, probably random garbage" );
						}

						archetype.push_back( hardpoint );

						fs->SetCurrentDirectory( ".." );
					}
				} 
				while( fs->FindNextFile( hHP, &hp_find ) ) ;

				fs->CloseHandle( hHP );
			}

			fs->SetCurrentDirectory( ".." );
		}
		else {

			// old style
			
			GENERAL_TRACE_1( TEMPSTR( "Hardpoint: create_archetype: '%s' has old hardpoint layout. If you see this message, mail pbleisch\n", dir_find.cFileName ) );

			DAFILEDESC desc( dir_find.cFileName );
			HANDLE hFile;
			U32 sizeof_file, hp_count, hp, br, l, sizeof_type;
			PersistHPRevolute in;
			JointType type;

			if( (hFile = fs->OpenChild( &desc )) != INVALID_HANDLE_VALUE ) {

				sizeof_file= fs->GetFileSize( hFile, NULL );
	//				cur_pos = fs->SetFilePointer( hFile, 0, 0, FILE_CURRENT );	//??
				type = jointtypename_to_jointtype( dir_find.cFileName );

				switch( type ) {
				case JT_FIXED:		sizeof_type = sizeof(PersistHPFixed);		break;
				case JT_REVOLUTE:	sizeof_type = sizeof(PersistHPRevolute);	break;
				case JT_PRISMATIC:	sizeof_type = sizeof(PersistHPRevolute);	break;
				default:			sizeof_type = 0;
				}

				hp_count = sizeof_file / sizeof_type;

				for( hp = 0; hp < hp_count; hp++ ) {
						
					if( (fs->ReadFile( hFile, &in, sizeof_type, &br )) && br==sizeof_type ) {
							
						Hardpoint hardpoint;

						l = Tmin( PERSIST_MAX_HARDPOINT_NAME, MAX_HARDPOINT_NAME );
						strncpy( hardpoint.label, in.spot.name, l );
						hardpoint.label[l-1] = 0;
							
						hardpoint.info.type = type;

						switch( hardpoint.info.type ) {

						case JT_REVOLUTE:
						case JT_PRISMATIC:
							hardpoint.info.axis = in.axis;
							hardpoint.info.min0 = in.min;
							hardpoint.info.max0 = in.max;
							// note the LACK of a break;

						case JT_FIXED:
							hardpoint.info.point = in.spot.point;
							hardpoint.info.orientation = in.spot.orientation;
							break;

						default:
							GENERAL_WARNING( "Hardpoint: unknown hardpoint type, probably random garbage" );
						}

						archetype.push_back( hardpoint );
					}
				}

				fs->CloseHandle( hFile );
			}
		}
	}
	while( fs->FindNextFile( hDir, &dir_find) );

	fs->FindClose( hDir );
	fs->SetCurrentDirectory ("..");

	if( archetype.size() < 1 ) {
		return FALSE;
	}

	archetypes.insert( arch_index, archetype );

	return TRUE;
}

//

bool HardPointComponent::retrieve_hardpoint_info( ARCHETYPE_INDEX arch_index, const char *name, HardpointInfo& out ) const
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( name != NULL );

	HardpointArchetypeMap::const_iterator arch;
	
	if( (arch = archetypes.find( arch_index )) == archetypes.end() ) {
		return false;
	}

	HardpointArchetype::const_iterator beg = arch->second.begin();
	HardpointArchetype::const_iterator end = arch->second.end();
	HardpointArchetype::const_iterator hpt;

	for( hpt=beg; hpt!=end; hpt++ ) {
		if( (*hpt).matches( name ) ) {
			out = (*hpt).info;
			return true;
		}
	}

	return false;
}

//

bool HardPointComponent::set_hardpoint_info( ARCHETYPE_INDEX arch_index, const char *name, const HardpointInfo& hp )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( name );
	ASSERT( hp.type != JT_NONE );

	HardpointArchetypeMap::iterator arch;
	
	if( (arch = archetypes.find( arch_index )) == archetypes.end() ) {
		// insert new archetype
		if( (arch = archetypes.insert( arch_index, HardpointArchetype ())) == archetypes.end() ) {
			return false;
		}
	}

	HardpointArchetype::iterator beg = arch->second.begin();
	HardpointArchetype::iterator end = arch->second.end();
	HardpointArchetype::iterator hpt;

	for( hpt=beg; hpt!=end; hpt++ ) {
		if( (*hpt).matches( name ) ) {
			(*hpt).info = hp;
			return true;
		}
	}

	Hardpoint hardpoint;

	strncpy( hardpoint.label, name, MAX_HARDPOINT_NAME );
	hardpoint.label[MAX_HARDPOINT_NAME-1] = 0;

	hardpoint.info = hp;
	arch->second.push_back( hardpoint );

	return true;
}

//

void HardPointComponent::enumerate_hardpoints( HARDPOINT_ENUM_CALLBACK cbfn, ARCHETYPE_INDEX arch_index, void *misc ) const
{
	ASSERT( cbfn != NULL ) ;
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	HardpointArchetypeMap::const_iterator arch;
	
	if( (arch = archetypes.find( arch_index )) == archetypes.end() ) {
		return ;
	}

	HardpointArchetype::const_iterator beg = arch->second.begin();
	HardpointArchetype::const_iterator end = arch->second.end();
	HardpointArchetype::const_iterator hpt;

	for( hpt=beg; hpt!=end; hpt++ ) {
		cbfn( (*hpt).label, misc );
	}

	return ;
}

//

int HardPointComponent::connect( INSTANCE_INDEX parent_inst_index, const char *parent_hp_name, INSTANCE_INDEX child_inst_index, const char *child_hp_name )
{
	ASSERT( engine );

	int ret_code = 0;
	ARCHETYPE_INDEX parent_arch_index = INVALID_ARCHETYPE_INDEX;
	ARCHETYPE_INDEX child_arch_index = INVALID_ARCHETYPE_INDEX;
	HardpointInfo parent_hp_info, child_hp_info;
	JointInfo ji;

	if( ((parent_arch_index = engine->get_instance_archetype( parent_inst_index )) == INVALID_ARCHETYPE_INDEX) ||
		!retrieve_hardpoint_info( parent_arch_index, parent_hp_name, parent_hp_info ) ) {
		ret_code = 1;
	}
	else if( ((child_arch_index = engine->get_instance_archetype( child_inst_index )) == INVALID_ARCHETYPE_INDEX) ||
		!retrieve_hardpoint_info( child_arch_index, child_hp_name, child_hp_info ) ) {
		ret_code = 2;
	}
	else {
		ret_code = 3;

		switch( ji.type = child_hp_info.type ) {

		case JT_FIXED:
			{
				// BBALDWIN - The desired behavior in connecting 2 hardpoints to form a FIXED joint
				// is to make the hardpoint positions & orientations coincide. This causes connections 
				// to behave identically regardless OF which object is the parent and which is the child.
				//
				//      R_child * R_child_hp = R_parent * R_parent_hp
				//
				//  ==> R_child = R_parent * R_parent_hp * R_child_hp.get_transpose()
				//
				//  ==> R_child = R_parent * R_rel
				//
				//  where R_rel = R_parent_hp * R_child_hp.get_transpose().
				//
				Matrix R_rel( parent_hp_info.orientation * child_hp_info.orientation.get_transpose());
				ji.rel_position = parent_hp_info.point - R_rel * child_hp_info.point;
				ji.rel_orientation = R_rel;
			}
			break;

		case JT_REVOLUTE:
		case JT_PRISMATIC:
			{
				ji.parent_point = parent_hp_info.point;
				ji.child_point = child_hp_info.point;
				ji.rel_orientation = parent_hp_info.orientation * child_hp_info.orientation;
				ji.axis = ji.rel_orientation * child_hp_info.axis;
				ji.min0 = child_hp_info.min0;
				ji.max0 = child_hp_info.max0;
			}
			break;
		
		} // end switch( joint.type )

		if( engine->create_joint( parent_inst_index, child_inst_index, &ji ) ) {
			ret_code = 0;
		}
	}

	engine->release_archetype( child_arch_index );
	engine->release_archetype( parent_arch_index );
	
	return ret_code;
}

//

void HardPointComponent::destroy_archetype( ARCHETYPE_INDEX arch_index )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	HardpointArchetypeMap::iterator arch;
	
	if( (arch = archetypes.find( arch_index )) == archetypes.end() ) {
		return ;
	}

	archetypes.erase( arch_index );
}

//

void COMAPI HardPointComponent::duplicate_archetype( ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index )
{
	ASSERT( new_arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( old_arch_index != INVALID_ARCHETYPE_INDEX );

	HardpointArchetypeMap::const_iterator old_arch;
	
	if( (old_arch = archetypes.find( old_arch_index )) == archetypes.end() ) {
		return ;
	}

	archetypes.insert( new_arch_index, old_arch->second );
}

//

GENRESULT COMAPI HardPointComponent::query_archetype_interface( ARCHETYPE_INDEX, const char *, IDAComponent ** ) 
{
	return GR_GENERIC;
}

//

BOOL32 HardPointComponent::create_instance( INSTANCE_INDEX, ARCHETYPE_INDEX )
{
	return FALSE;
}

//

void HardPointComponent::destroy_instance( INSTANCE_INDEX )
{
	return ;
}

//

void HardPointComponent::update_instance( INSTANCE_INDEX , SINGLE )
{
	return ;
}

//

enum vis_state COMAPI HardPointComponent::render_instance( struct ICamera *, INSTANCE_INDEX, float, U32, const Transform * ) 
{
	return VS_UNKNOWN;
}

//

GENRESULT COMAPI HardPointComponent::query_instance_interface( INSTANCE_INDEX, const char *, IDAComponent ** ) 
{
	return GR_GENERIC;
}

//

void HardPointComponent::update( SINGLE )
{
	return ;
}

//

JointType HardPointComponent::jointtypename_to_jointtype( const char *jointtypename )
{
	if( strcmp( jointtypename, HP_FIXED_NAME ) == 0 ) {
		return JT_FIXED;
	}
	else if( strcmp( jointtypename, HP_REVOLUTE_NAME ) == 0 ) {
		return JT_REVOLUTE;
	}
	else if( strcmp( jointtypename, HP_PRISMATIC_NAME ) == 0 ) {
		return JT_PRISMATIC;
	}
	
	GENERAL_WARNING( "HardPoint: unknown joint type, returning JT_FIXED" );
	return JT_FIXED;
}

//

BOOL COMAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:

			DA_HEAP_ACQUIRE_HEAP( HEAP );
			DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

			IComponentFactory* server = 
				new DAComponentFactory2<DAComponentAggregate<HardPointComponent>, SYSCONSUMERDESC> ("IHardpoint");

			if (server)
			{
				DACOM_Acquire ()->RegisterComponent(server, "IHardpoint", DACOM_NORMAL_PRIORITY);
			}

			server->Release();	// DACOM has added a reference to the server, 
								// we call Release() since we don't save the pointer
			break;
	}

	return TRUE;
}


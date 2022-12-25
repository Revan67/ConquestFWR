// EngineCameras.cpp
//
//
//
//

#pragma warning (disable : 4786 4530)

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <limits.h>
#include <float.h>

#include <limits>
#include <map>

#include "dacom.h"
#include "TComponent.h"
#include "TSmartPointer.h"
#include "3DMath.h"
#include "da_heap_utility.h"
#include "FDump.h"
#include "TempStr.h"
#include "SysConsumerDesc.h"
#include "AllocLite.h"
#include "FileSys_Utility.h"
#include "IProfileParser_Utility.h"
#include "ICamera.h"
#include "Engine.h"
#include "EngComp.h"

#include "Tfuncs.h"

#include "EngineCameraArchetype.h"
#include "EngineCameraInstance.h"

//

const char *CLSID_EngineCameras = "EngineCameras";

//

typedef std::map<ARCHETYPE_INDEX, EngineCameraArchetype*> ArchetypeMap;
typedef std::map<INSTANCE_INDEX, EngineCameraInstance*> InstanceMap;

//

struct EngineCameras : public IEngineComponent
{
public:	// Data

	BEGIN_DACOM_MAP_INBOUND(EngineCameras)
	DACOM_INTERFACE_ENTRY(IEngineComponent)
	DACOM_INTERFACE_ENTRY (IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IEngineComponent,IEngineComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

protected: // Data

	ArchetypeMap archetypes;
	InstanceMap instances;

	IEngine *engine;
	IDAComponent *system;

protected: // Interface


public: // Interface

	EngineCameras( void );
	~EngineCameras( void );
	GENRESULT init( SYSCONSUMERDESC *info );

	DA_HEAP_DEFINE_NEW_OPERATOR(EngineCameras);

	//IAggregateComponent
	GENRESULT COMAPI Initialize( void );

	//IEngineComponent
	void			COMAPI update( SINGLE dt ) ;
	BOOL32			COMAPI create_archetype( ARCHETYPE_INDEX arch_index, struct IFileSystem *filesys ) ;
	void			COMAPI duplicate_archetype( ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index ) ;
	void			COMAPI destroy_archetype( ARCHETYPE_INDEX arch_index ) ;
	GENRESULT		COMAPI query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif );
	BOOL32			COMAPI create_instance( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index ) ;
	void			COMAPI destroy_instance( INSTANCE_INDEX inst_index ) ;
	void			COMAPI update_instance( INSTANCE_INDEX inst_index, SINGLE dt ) ;
	enum vis_state	COMAPI render_instance( struct ICamera *camera, INSTANCE_INDEX inst_index, float lod_fraction, U32 flags, const Transform *modifier_transform ) ;
	GENRESULT		COMAPI query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif ) ;
};

//

EngineCameras::EngineCameras( void )
{
	system = NULL;
	engine = NULL;
}

//

EngineCameras::~EngineCameras( void )
{
	if( system ) {
		system->Release();
		system = NULL;
	}

	engine = NULL;
}

//

GENRESULT EngineCameras::init( SYSCONSUMERDESC *info )
{
	if( info == NULL || info->system == NULL ) {
		return GR_INVALID_PARMS;
	}

	system = info->system;
	system->AddRef();

	return GR_OK;
}

//

GENRESULT EngineCameras::Initialize( void )
{
	if( FAILED( static_cast<IEngineComponent*>( this )->QueryInterface( IID_IEngine, (void**) &engine ) ) ){
		GENERAL_WARNING( "EngineCameras: Initialize: unable to acquire IID_IEngine\n" );
		return GR_GENERIC;
	}
	engine->Release();

	return GR_OK;
}

//

void COMAPI EngineCameras::update (SINGLE dt)
{
}

//

BOOL32 EngineCameras::create_archetype( ARCHETYPE_INDEX arch_index, struct IFileSystem *filesys )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( filesys != NULL );

	EngineCameraArchetype *ela;

	if( (ela = new EngineCameraArchetype()) == NULL ) {
		return FALSE;
	}

	if( !ela->load_from_filesystem( filesys ) ) {
		delete ela;
		return FALSE;
	}

	if( archetypes.insert( ArchetypeMap::value_type( arch_index, ela ) ).second == false ) {
		delete ela;
		return FALSE;
	}

	return TRUE;
}

//

void COMAPI	EngineCameras::duplicate_archetype( ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index )
{
	ASSERT( new_arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( old_arch_index != INVALID_ARCHETYPE_INDEX );

	ArchetypeMap::const_iterator old_arch;
	EngineCameraArchetype *ela;
	
	if( (old_arch = archetypes.find( old_arch_index )) == archetypes.end() ) {
		return;
	}

	if( (ela = new EngineCameraArchetype()) == NULL ) {
		GENERAL_TRACE_1( "EngineCameras: duplicate_archetype: unable to create duplicate archetype\n" );
		return ;
	}

	*ela = *(*old_arch).second;

	if( archetypes.insert( ArchetypeMap::value_type( new_arch_index, ela ) ).second == false ) {
		GENERAL_TRACE_1( "EngineCameras: duplicate_archetype: unable to insert duplicate archetype\n" );
		delete ela;
		return ;
	}

	return ;


}

//

void COMAPI EngineCameras::destroy_archetype( ARCHETYPE_INDEX arch_index )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	ArchetypeMap::iterator ai;

	if( (ai = archetypes.find( arch_index )) != archetypes.end() ) {
		delete (*ai).second;
		archetypes.erase( ai );
	}
}

//

GENRESULT COMAPI EngineCameras::query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	ArchetypeMap::iterator ai;

	if( (ai = archetypes.find( arch_index )) != archetypes.end() ) {
		return (*ai).second->QueryInterface( iid, (void**)out_iif );
	}

	return GR_GENERIC;
}

//

BOOL32 COMAPI EngineCameras::create_instance( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	EngineCameraInstance *eli;
	InstanceMap::iterator ii;
	ArchetypeMap::iterator ai;
	
	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return FALSE;
	}
	
	if( (eli = new EngineCameraInstance()) == NULL ) {
		return FALSE;
	}

	if( !eli->initialize_from_archetype( (*ai).second, system, engine, inst_index ) ) {
		delete eli;
		return FALSE;
	}

	if( (ii = instances.insert( InstanceMap::value_type( inst_index, eli ) ).first) == instances.end() ) {
		delete eli;
		return FALSE;
	}

	return TRUE;
}

//

void COMAPI EngineCameras::destroy_instance( INSTANCE_INDEX inst_index ) 
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	InstanceMap::iterator ii;
	
	if( (ii = instances.find( inst_index )) == instances.end() ) {
		return ;
	}

	delete (*ii).second;
	instances.erase( ii );
}

//

void COMAPI EngineCameras::update_instance( INSTANCE_INDEX inst_index, SINGLE dt )
{
}

//

vis_state COMAPI EngineCameras::render_instance( ICamera *camera, INSTANCE_INDEX inst_index, float lod_fraction, U32 flags, const Transform *tr )
{
	ASSERT( camera != NULL );
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( lod_fraction <= 1.1f );	
	
	return VS_UNKNOWN;
}

//

GENRESULT COMAPI EngineCameras::query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	InstanceMap::iterator ii;

	if( (ii = instances.find( inst_index )) != instances.end() ) {
		return (*ii).second->QueryInterface( iid, (void**)out_iif );
	}

	return GR_GENERIC;
}

//


// ......................................................................
//
// DllMain
//
//
BOOL COMAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	IComponentFactory *server;

	if( DLL_PROCESS_ATTACH == fdwReason ) {

		DA_HEAP_ACQUIRE_HEAP(HEAP);
		DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);
		
		if( (server = new DAComponentFactory2<DAComponentAggregate<EngineCameras>, SYSCONSUMERDESC>( CLSID_EngineCameras )) != NULL ) {
			DACOM_Acquire()->RegisterComponent( server, CLSID_EngineCameras, DACOM_NORMAL_PRIORITY );
			server->Release();
		}
	}

	return TRUE;
}

// EOF














// EngineLights.cpp
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
#include "LightMan.h"

#include "Tfuncs.h"

#include "EngineLightArchetype.h"
#include "EngineLightInstance.h"

//

const char *CLSID_EngineLights = "EngineLights";

//

#define EL_F_REGISTER_ON_LOAD	(1<<0)

//

typedef std::map<ARCHETYPE_INDEX, EngineLightArchetype*> ArchetypeMap;
typedef std::map<INSTANCE_INDEX, EngineLightInstance*> InstanceMap;

//

struct EngineLights : public IEngineComponent
{
public:	// Data

	BEGIN_DACOM_MAP_INBOUND(EngineLights)
	DACOM_INTERFACE_ENTRY(IEngineComponent)
	DACOM_INTERFACE_ENTRY (IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IEngineComponent,IEngineComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

protected: // Data

	ArchetypeMap archetypes;
	InstanceMap instances;

	U32 el_f_flags;

	IEngine *engine;
	IDAComponent *system;
	ILightManager *lightmanager;

protected: // Interface


public: // Interface

	EngineLights( void );
	~EngineLights( void );
	GENRESULT init( SYSCONSUMERDESC *info );

	DA_HEAP_DEFINE_NEW_OPERATOR(EngineLights);

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

EngineLights::EngineLights( void )
{
	system = NULL;
	engine = NULL;
	lightmanager = NULL;

	el_f_flags = 0;
}

//

EngineLights::~EngineLights( void )
{
	if( lightmanager ) {
		lightmanager->Release();
		lightmanager = NULL;
	}

	if( system ) {
		system->Release();
		system = NULL;
	}

	engine = NULL;
}

//

GENRESULT EngineLights::init( SYSCONSUMERDESC *info )
{
	if( info == NULL || info->system == NULL ) {
		return GR_INVALID_PARMS;
	}

	system = info->system;
	system->AddRef();

	el_f_flags = 0;

	return GR_OK;
}

//

GENRESULT EngineLights::Initialize( void )
{
	U32 value;

	if( FAILED( static_cast<IEngineComponent*>( this )->QueryInterface( IID_IEngine, (void**) &engine ) ) ){
		GENERAL_WARNING( "EngineLights: Initialize: unable to acquire IID_IEngine\n" );
		return GR_GENERIC;
	}
	engine->Release();

	if( FAILED( system->QueryInterface( IID_ILightManager, (void**) &lightmanager ) ) ) {
		GENERAL_NOTICE( "EngineLights: Initialize: unable to acquire IID_ILightManager, not registering lights\n" );
	}

	opt_get_u32( DACOM_Acquire(), NULL, CLSID_EngineLights, "RegisterOnLoad", 0, &value );
	if( value ) {
		el_f_flags |= EL_F_REGISTER_ON_LOAD;
	}
	else {
		el_f_flags &= ~(EL_F_REGISTER_ON_LOAD);
	}

	return GR_OK;
}

//

void COMAPI EngineLights::update (SINGLE dt)
{
}

//

BOOL32 EngineLights::create_archetype( ARCHETYPE_INDEX arch_index, struct IFileSystem *filesys )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( filesys != NULL );

	EngineLightArchetype *ela;

	if( (ela = new EngineLightArchetype()) == NULL ) {
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

void COMAPI	EngineLights::duplicate_archetype( ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index )
{
	ASSERT( new_arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( old_arch_index != INVALID_ARCHETYPE_INDEX );

	ArchetypeMap::const_iterator old_arch;
	EngineLightArchetype *ela;
	
	if( (old_arch = archetypes.find( old_arch_index )) == archetypes.end() ) {
		return;
	}

	if( (ela = new EngineLightArchetype()) == NULL ) {
		GENERAL_TRACE_1( "EngineLights: duplicate_archetype: unable to create duplicate archetype\n" );
		return ;
	}

	*ela = *(*old_arch).second;

	if( archetypes.insert( ArchetypeMap::value_type( new_arch_index, ela ) ).second == false ) {
		GENERAL_TRACE_1( "EngineLights: duplicate_archetype: unable to insert duplicate archetype\n" );
		delete ela;
		return ;
	}

	return ;


}

//

void COMAPI EngineLights::destroy_archetype( ARCHETYPE_INDEX arch_index )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	ArchetypeMap::iterator ai;

	if( (ai = archetypes.find( arch_index )) != archetypes.end() ) {
		delete (*ai).second;
		archetypes.erase( ai );
	}
}

//

GENRESULT COMAPI EngineLights::query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif )
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	ArchetypeMap::iterator ai;

	if( (ai = archetypes.find( arch_index )) != archetypes.end() ) {
		return (*ai).second->QueryInterface( iid, (void**)out_iif );
	}

	return GR_GENERIC;
}

//

BOOL32 COMAPI EngineLights::create_instance( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	EngineLightInstance *eli;
	InstanceMap::iterator ii;
	ArchetypeMap::iterator ai;
	
	if( (ai = archetypes.find( arch_index )) == archetypes.end() ) {
		return FALSE;
	}
	
	if( (eli = new EngineLightInstance()) == NULL ) {
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

	if( lightmanager && (el_f_flags & EL_F_REGISTER_ON_LOAD) ) {
		lightmanager->register_light( static_cast<ILight*>(eli) );
	}

	return TRUE;
}

//

void COMAPI EngineLights::destroy_instance( INSTANCE_INDEX inst_index ) 
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	InstanceMap::iterator ii;
	
	if( (ii = instances.find( inst_index )) == instances.end() ) {
		return ;
	}

	if( lightmanager && (el_f_flags & EL_F_REGISTER_ON_LOAD) ) {
		lightmanager->unregister_light( static_cast<ILight*>((*ii).second) );
	}

	delete (*ii).second;
	instances.erase( ii );
}

//

void COMAPI EngineLights::update_instance( INSTANCE_INDEX inst_index, SINGLE dt )
{
}

//

vis_state COMAPI EngineLights::render_instance( ICamera *camera, INSTANCE_INDEX inst_index, float lod_fraction, U32 flags, const Transform *tr )
{
	ASSERT( camera != NULL );
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( lod_fraction <= 1.1f );	
	
	return VS_UNKNOWN;
}

//

GENRESULT COMAPI EngineLights::query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif )
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
		
		if( (server = new DAComponentFactory2<DAComponentAggregate<EngineLights>, SYSCONSUMERDESC>( CLSID_EngineLights )) != NULL ) {
			DACOM_Acquire()->RegisterComponent( server, CLSID_EngineLights, DACOM_NORMAL_PRIORITY );
			server->Release();
		}
	}

	return TRUE;
}

// EOF














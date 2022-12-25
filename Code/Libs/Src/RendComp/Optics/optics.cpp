//$Header: /Libs/dev/Src/RendComp/Optics/optics.cpp 35    2/23/00 1:50p Pbleisch $
//Copyright (c) 1997 Digital Anvil, Inc.
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "FDump.h"
#include "TempStr.h"
#include "dacom.h"
#include "TComponent.h"
#include "TSmartPointer.h"
#include "View2D.h"
#include "3dmath.h"
#include "packed_argb.h"
#include "da_heap_utility.h"

#include "ICamera.h"
#include "rendpipeline.h"
#include "IRenderPrimitive.h"
#include "IRenderComponent.h"
#include "ITextureLibrary.h"
#include "Engine.h"
#include "FileSys.h"
#include "IParticleSystem.h"

//

#include "handlemap.h"

//

#include "ParticleSystemArchetype.h"
#include "ParticleSystemInstance.h"

//

const char *CLSID_Optics = "Optics";

//

typedef inst_handlemap< ParticleSystemInstance* >	inst_map;
typedef rarch_handlemap< ParticleSystemArchetype*>	rarch_map;

// .........................................................................
//
// Optics
//
// Manages and renders particle effects
//
struct Optics : public IRenderComponent
{
	BEGIN_DACOM_MAP_INBOUND(Optics)
	DACOM_INTERFACE_ENTRY2(IID_IRenderComponent,IRenderComponent)
	END_DACOM_MAP()

public: // Interface

	DA_HEAP_DEFINE_NEW_OPERATOR(Optics)

	Optics( void );
	~Optics( void );
	GENRESULT init( RendCompDesc *info );

	// IRenderComponent
	GENRESULT COMAPI set_render_property( const RenderProp name, DACOM_VARIANT value ) ;
	GENRESULT COMAPI get_render_property( const RenderProp name, DACOM_VARIANT out_value ) ;
	void COMAPI update( float dt ) ;
	bool COMAPI create_archetype( RENDER_ARCHETYPE render_arch_index, IFileSystem *filesys ) ;
	bool COMAPI duplicate_archetype( RENDER_ARCHETYPE new_render_arch_index, RENDER_ARCHETYPE old_render_arch_index ) ;
	void COMAPI destroy_archetype( RENDER_ARCHETYPE render_arch_index ) ;
	bool COMAPI split_archetype( RENDER_ARCHETYPE render_arch_index, const Vector& normal, float d, RENDER_ARCHETYPE r0, RENDER_ARCHETYPE r1, U32 sa_flags, INSTANCE_INDEX inst_index ) ;
	bool COMAPI get_archetype_statistics( RENDER_ARCHETYPE render_arch_index, float lod_fraction, enum StatType statistic, DACOM_VARIANT out_value ) ;
	bool COMAPI get_archetype_bounding_box( RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] ) ;
	bool COMAPI get_archetype_centroid( RENDER_ARCHETYPE render_arch_index, float lod_fraction, Vector& out_centroid ) ;
	bool COMAPI query_archetype_interface( RENDER_ARCHETYPE render_arch_index, const char *iid, IDAComponent **out_iid ) ;
	bool COMAPI create_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) ;
	void COMAPI destroy_instance( INSTANCE_INDEX inst_index ) ;
	void COMAPI update_instance( INSTANCE_INDEX inst_index, float dt ) ;
	vis_state COMAPI render_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, struct ICamera *camera, float lod_fraction, U32 flags, const Transform *tr ) ;
	bool COMAPI get_instance_bounding_box( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] ) ;		
	bool COMAPI query_instance_interface( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, const char *iid, IDAComponent **out_iid ) ;
	struct Mesh * COMAPI get_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) ;
	struct Mesh * COMAPI get_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) ;
	GENRESULT COMAPI release_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) ;
	struct Mesh * COMAPI get_archetype_mesh( RENDER_ARCHETYPE render_arch_index ) ;

protected: // Data

	IDAComponent *system;
	IRenderPipeline *renderpipeline;
	IRenderPrimitive *renderprimitive;
	IEngine *engine;
	ITextureLibrary *texturelibrary;

//	mutable DynamicArray< TPointer< ParticleSystemArchetype > > archetypes;
//	mutable DynamicArray< TPointer< ParticleSystemInstance > >  instances;
	mutable rarch_map archetypes;
	mutable inst_map instances;
};


//

Optics::Optics( void )  
{
	system = NULL;
	renderpipeline = NULL;
	renderprimitive = NULL;
	engine = NULL;
	texturelibrary = NULL;
}

//

Optics::~Optics()
{
	inst_map::iterator ibeg = instances.begin();
	inst_map::iterator iend = instances.end();
	inst_map::iterator inst;

	for( inst=ibeg; inst!=iend; inst++ ) {
		GENERAL_NOTICE( TEMPSTR( "Optics: dtor: Instance %d has dangling particle system\n", inst->first ) );
	}

	rarch_map::iterator abeg = archetypes.begin();
	rarch_map::iterator aend = archetypes.end();
	rarch_map::iterator arch;

	for( arch=abeg; arch!=aend; arch++ ) {
		GENERAL_NOTICE( TEMPSTR( "Optics: dtor: Particle System archetype %d is dangling \n", arch->first ) );
	}

	DACOM_RELEASE( texturelibrary );
	DACOM_RELEASE( renderpipeline );
	DACOM_RELEASE( renderprimitive );

	extern Vector *pe_vector_buffer;
	extern RPVertex *pe_vertex_buffer;

	delete[] pe_vector_buffer;
	delete[] pe_vertex_buffer;
}

//

GENRESULT Optics::init( RendCompDesc *info )
{
	ASSERT( info );

	if( info->system_services == NULL || NULL == info->engine_services ) {
		return GR_INVALID_PARMS;		// must have a system container
	}

	if( FAILED( info->system_services->QueryInterface( IID_IRenderPrimitive, (void**) &renderprimitive ) ) ) {
		GENERAL_ERROR( "Optics: init: Failed to get IRenderPrimitive\n" );
		return GR_GENERIC;
	}
	
	if( FAILED( info->system_services->QueryInterface( IID_IRenderPipeline, (void**) &renderpipeline ) ) ) {	
		GENERAL_ERROR( "Optics: init: Failed to get IRenderPipeline" );
		return GR_GENERIC;
	}

	if( FAILED( info->system_services->QueryInterface ( IID_ITextureLibrary, (void**)&texturelibrary ) ) ) {
		GENERAL_ERROR( "Optics: init: Failed to get ITextureLibrary" );
		return GR_GENERIC;
	}

	if( FAILED( info->engine_services->QueryInterface( IID_IEngine, (void **) &engine ) ) ) {
		GENERAL_ERROR( "Optics: init: Unable to get IEngine" );
		return GR_GENERIC;
	}
	engine->Release();

	return GR_OK;
}

//

GENRESULT COMAPI Optics::set_render_property( const RenderProp name, DACOM_VARIANT value ) 
{
	return GR_GENERIC;
}

//

GENRESULT COMAPI Optics::get_render_property( const RenderProp name, DACOM_VARIANT out_value ) 
{
	return GR_GENERIC;
}

//

//updates with zero dt are valid
void Optics::update( SINGLE dt )
{
	inst_map::iterator ibeg = instances.begin();
	inst_map::iterator iend = instances.end();
	inst_map::iterator inst;

	for( inst=ibeg; inst!=iend; inst++ ) {
		if( (*inst).second->is_active() ) {
			(*inst).second->update_system( dt, engine );
		}
	}
}

//

bool Optics::create_archetype( RENDER_ARCHETYPE render_arch_index, IFileSystem *filesys )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( filesys != NULL );

	ParticleSystemArchetype *new_archetype;

	if( (new_archetype = new ParticleSystemArchetype) == NULL ) {
		return false;
	}

	if( new_archetype->load_from_filesystem( filesys, texturelibrary ) == false ) {
		delete new_archetype;
		return false;
	}

	archetypes.insert( render_arch_index, new_archetype );

	return true;
}

//

bool COMAPI Optics::duplicate_archetype( RENDER_ARCHETYPE new_render_arch_index, RENDER_ARCHETYPE old_render_arch_index )
{
	ASSERT( new_render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( old_render_arch_index != INVALID_RENDER_ARCHETYPE );

	rarch_map::iterator rarch;
	ParticleSystemArchetype *old_archetype, *new_archetype;

	if( (rarch = archetypes.find( old_render_arch_index )) == archetypes.end() ) {
		return false;
	}
	
	old_archetype = rarch->second;

	if( (new_archetype = new ParticleSystemArchetype()) == NULL ) {
		return false;
	}

	*new_archetype = *old_archetype;

	archetypes.insert( new_render_arch_index, new_archetype );

	return true;
}

//

void Optics::destroy_archetype( RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );

	rarch_map::iterator rarch;
	
	if( (rarch = archetypes.find( render_arch_index )) != archetypes.end() ) {
		delete rarch->second;
		archetypes.erase( render_arch_index );
	}
}

//

bool Optics::split_archetype( RENDER_ARCHETYPE render_arch_index, const Vector& normal, float d, RENDER_ARCHETYPE r0, RENDER_ARCHETYPE r1, U32 sa_flags, INSTANCE_INDEX inst_index )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );

	return false;
}

//

bool COMAPI Optics::get_archetype_statistics( RENDER_ARCHETYPE render_arch_index, float lod_fraction, enum StatType statistic, DACOM_VARIANT out_value ) 
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	
	return false;
}

//

bool COMAPI Optics::get_archetype_bounding_box( RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE *out_box ) 
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	
	return false;
}

//

bool COMAPI Optics::get_archetype_centroid( RENDER_ARCHETYPE render_arch_index, float lod_fraction, Vector& out_centroid ) 
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	
	out_centroid.zero();
	
	return true;
}

//

bool COMAPI Optics::query_archetype_interface( RENDER_ARCHETYPE render_arch_index, const char *iid, IDAComponent **out_iif ) 
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );

	rarch_map::iterator rarch;
	
	if( (rarch = archetypes.find( render_arch_index )) != archetypes.end() ) {
		if( SUCCEEDED( rarch->second->QueryInterface( IID_IParticleSystem, (void**)out_iif ) ) ) {
			return true;
		}
	}

	return false;
}

//
                               
bool Optics::create_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );

	ParticleSystemInstance *new_instance;
	
	rarch_map::iterator rarch;
	
	if( (rarch = archetypes.find( render_arch_index )) == archetypes.end() ) {
		return false;
	}

	if( (new_instance = new ParticleSystemInstance()) == NULL ) {
		return false;
	}

	if( new_instance->initialize_from_archetype( rarch->second, texturelibrary, inst_index ) ) {
		instances.insert( inst_index, new_instance );
		engine->set_instance_bounding_sphere( inst_index, EN_DONT_RECURSE, new_instance->parameters.bounding_sphere_radius, Vector(0,0,0) ) ;
		return true;
	}

	return false;
}

//

void Optics::destroy_instance( INSTANCE_INDEX inst_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	inst_map::iterator inst;
	
	if( (inst = instances.find( inst_index )) == instances.end() ) {
		return ;
	}

	inst->second->cleanup( );
	delete inst->second;
	instances.erase( inst_index );
}

//

//updates with zero dt are valid
void Optics::update_instance( INSTANCE_INDEX inst_index, float dt )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	inst_map::iterator inst;
	
	if( (inst = instances.find( inst_index )) != instances.end() ) {
		if( inst->second->is_active() ) {
			inst->second->update_system( dt, engine );
		}
	}
}

//

vis_state Optics::render_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, struct ICamera *camera, float lod_fraction, U32 flags, const Transform *tr )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	inst_map::iterator inst;
	
	if( (inst = instances.find( inst_index )) != instances.end() ) {
		if( inst->second->is_active() ) {
			return inst->second->render_system( renderprimitive, engine, camera, lod_fraction );
		}
	}

	return VS_UNKNOWN;
}

//

bool COMAPI Optics::get_instance_bounding_box( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] ) 
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	return false;
}

//

bool COMAPI Optics::query_instance_interface( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE, const char *iid, IDAComponent **out_iif ) 
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	inst_map::iterator inst;

	if( (inst = instances.find( inst_index )) != instances.end() ) {
		if( SUCCEEDED( inst->second->QueryInterface( iid, (void**)out_iif ) ) ) {
			return true;
		}
	}

	return false;
}


//

struct Mesh * COMAPI Optics::get_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) 
{
	return NULL;
}

//

struct Mesh * COMAPI Optics::get_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) 
{
	return NULL;
}

//

GENRESULT COMAPI Optics::release_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index ) 
{
	return GR_GENERIC;
}

//

struct Mesh * COMAPI Optics::get_archetype_mesh( RENDER_ARCHETYPE render_arch_index ) 
{
	return NULL;
}

//

BOOL COMAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	IComponentFactory *server;

	if( fdwReason == DLL_PROCESS_ATTACH ) {
		
	    DA_HEAP_ACQUIRE_HEAP( HEAP );
		DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

		if( (server = new DAComponentFactory< DAComponent< Optics >, RendCompDesc >( CLSID_Optics )) == NULL ) {
			return TRUE;
		}
		
		DACOM_Acquire()->RegisterComponent( server, CLSID_Optics, DACOM_NORMAL_PRIORITY );

		server->Release();
	}

	return TRUE;
}

// TriMesh.cpp
//
//
// TODO:
// a) Figure out how discrete LOD affects all of this code.
// b) Finish split() code.
//

#pragma warning( disable : 4786 )

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <limits.h>

#include <map>

//

#include "dacom.h"
#include "TComponent.h"
#include "FDUMP.h"
#include "TempStr.h"

#include "da_heap_utility.h"
#include "3dmath.h"
#include "SysConsumerDesc.h"
#include "engine.h"
#include "filesys.h"
#include "renderer.h"
#include "ICamera.h"
#include "IRenderComponent.h"
#include "RendPipeline.h"
#include "IRenderPrimitive.h"
#include "IVertexBufferManager.h"

//

#include "Tfuncs.h"
#include "handlemap.h"

//

#include "TriMeshArchetype.h"
#include "TriMeshInstance.h"

//

const char *CLSID_TriMesh = "TriMesh";

//

#pragma warning( disable : 4786 )

//

// ........................................................................
//
// TriMesh
//
// Renders triangle mesh objects with optional continuous lod.
//
struct DACOM_NO_VTABLE TriMesh : public IRenderComponent
{
public:	// Data
	BEGIN_DACOM_MAP_INBOUND(TriMesh)
	DACOM_INTERFACE_ENTRY2(IID_IRenderComponent,IRenderComponent)
	DACOM_INTERFACE_ENTRY(IRenderComponent)
	END_DACOM_MAP()

public:	// Interface

	DA_HEAP_DEFINE_NEW_OPERATOR(TriMesh)

	TriMesh( void );
	~TriMesh( void );
		
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

protected: // Interface

protected:	// Data

	IEngine					*engine;
	IRenderPipeline			*renderpipeline;
	IRenderPrimitive		*renderprimitive;
	IMaterialLibrary		*materiallibrary;
	IVertexBufferManager	*vbuffermanager;

	typedef rarch_handlemap< TriMeshArchetype* >	rarch_map;
	typedef inst_handlemap< TriMeshInstance* >		inst_map;
	
	rarch_map			render_archetypes;
	inst_map			instances;
};

//

TriMesh::TriMesh( void ) 
{
	engine = NULL;
	renderpipeline = NULL;
	renderprimitive = NULL;
	materiallibrary = NULL;
}

//

TriMesh::~TriMesh()
{
	//
	engine = NULL;
	
	DACOM_RELEASE( materiallibrary );
	DACOM_RELEASE( renderpipeline );
	DACOM_RELEASE( renderprimitive );
	DACOM_RELEASE( vbuffermanager );
}

//

GENRESULT TriMesh::init (RendCompDesc* info)
{
	if( info->system_services == NULL || NULL == info->engine_services ) {
		return GR_INVALID_PARMS;		// must have a system container
	}

	if( FAILED( info->system_services->QueryInterface( IID_IRenderPrimitive, (void**) &renderprimitive ) ) ) {
		GENERAL_ERROR( "TriMesh: Failed to get IID_IRenderPrimitive\n" );
		return GR_GENERIC;
	}
	
	if( FAILED( info->system_services->QueryInterface( IID_IRenderPipeline, (void**) &renderpipeline ) ) ) {	
		GENERAL_ERROR( "TriMesh: Failed to get IID_IRenderPipeline" );
		return GR_GENERIC;
	}

	if( FAILED( info->system_services->QueryInterface( IID_IMaterialLibrary, (void**) &materiallibrary ) ) ) {	
		GENERAL_ERROR( "TriMesh: Failed to get IID_IMaterialLibrary" );
		return GR_GENERIC;
	}

	if( FAILED( info->system_services->QueryInterface( IID_IVertexBufferManager, (void**) &vbuffermanager ) ) ) {	
		GENERAL_ERROR( "TriMesh: Failed to get IID_IVertexBufferManager" );
		return GR_GENERIC;
	}

	//
	// Get engine component resources
	//  
	if( FAILED( info->engine_services->QueryInterface( IID_IEngine, (void **) &engine ) ) ) {
		GENERAL_ERROR( "TriMesh: Unable to get IEngine" );
		return GR_GENERIC;
	}
	engine->Release();
														
	return GR_OK;
}

//

GENRESULT COMAPI TriMesh::set_render_property( const RenderProp name, DACOM_VARIANT value )
{
	return GR_GENERIC;
}

//

GENRESULT COMAPI TriMesh::get_render_property( const RenderProp name, DACOM_VARIANT value )
{
	return GR_GENERIC;
}

//

void TriMesh::update( float dt )
{
	inst_map::iterator ibeg = instances.begin();
	inst_map::iterator iend = instances.end();
	inst_map::iterator inst;

	for( inst=ibeg; inst!=iend; inst++ ) {
		if( inst->second ) {
			inst->second->update( dt );
		}
	}
}

//

bool TriMesh::create_archetype( RENDER_ARCHETYPE render_arch_index, IFileSystem *filesys )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( filesys != NULL );

	TriMeshArchetype *mesh_archetype;

	if( (mesh_archetype = new TriMeshArchetype()) == NULL ) {
		return false;
	}

	if( FAILED( mesh_archetype->initialize( ) ) ) {
		delete mesh_archetype;
		return false;
	}

	if( FAILED( mesh_archetype->load( filesys, renderpipeline, vbuffermanager, materiallibrary ) ) ) {
		delete mesh_archetype;
		return false;
	}

	render_archetypes.insert( render_arch_index, mesh_archetype );

	return true;
}

//

bool COMAPI TriMesh::duplicate_archetype( RENDER_ARCHETYPE new_render_arch_index, RENDER_ARCHETYPE old_render_arch_index )
{
	ASSERT( new_render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( old_render_arch_index != INVALID_RENDER_ARCHETYPE );

	TriMeshArchetype *new_mesh_archetype = NULL;
	rarch_map::iterator rarch;
	
	if( (rarch = render_archetypes.find( old_render_arch_index )) == render_archetypes.end() ) {
		return false;
	}

	if( rarch->second->clone( &new_mesh_archetype ) == false ) {
		return false;
	}

	render_archetypes.insert( new_render_arch_index, new_mesh_archetype );

	return true;
}

//

void TriMesh::destroy_archetype( RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	
	rarch_map::iterator rarch;

	if( (rarch = render_archetypes.find( render_arch_index )) != render_archetypes.end() ) {
		delete rarch->second;
		render_archetypes.erase( render_arch_index );
	}
}

//

bool TriMesh::split_archetype( RENDER_ARCHETYPE render_arch_index, const Vector& normal, float d, RENDER_ARCHETYPE r0, RENDER_ARCHETYPE r1, U32 sa_flags, INSTANCE_INDEX inst_index )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( r0 != INVALID_RENDER_ARCHETYPE );
	ASSERT( r1 != INVALID_RENDER_ARCHETYPE );

	ASSERT( inst_index == INVALID_INSTANCE_INDEX );

	TriMeshArchetype *mesh_archetype;
	TriMeshArchetype *o0, *o1;
	rarch_map::iterator rarch;
	inst_map::iterator inst;

	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return false;
	}

	mesh_archetype = rarch->second;

	ASSERT( mesh_archetype );

	if( FAILED(mesh_archetype->split( normal, d, sa_flags, &o0, &o1 )) ) {
		return false;
	}

	ASSERT( o0 );
	ASSERT( o1 );

	render_archetypes.insert( r0, o0 );
	render_archetypes.insert( r1, o1 );
	
	return true;
}

//

bool COMAPI TriMesh::get_archetype_statistics( RENDER_ARCHETYPE render_arch_index, float lod_fraction, enum StatType statistic, DACOM_VARIANT out_value ) 
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( lod_fraction >= 0.0f );
	ASSERT( lod_fraction  < 1.1f );

	rarch_map::iterator rarch;

	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return false;
	}

	switch( statistic ) {

	case ST_NUM_PRIMITIVES:
	case ST_NUM_FACES:
		if( ((U32*)out_value) != NULL ) {
			*((U32*)out_value) = rarch->second->get_num_mesh_faces();
			return true;
		}
		break;

	case ST_NUM_VERTICES:
		if( ((U32*)out_value) != NULL ) {
			*((U32*)out_value) = rarch->second->get_num_mesh_vertices();
			return true;
		}
		break;

	}

	return false;
}

//

bool COMAPI TriMesh::get_archetype_bounding_box( RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] ) 
{
	ASSERT( render_arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( lod_fraction >= 0.0f );
	ASSERT( lod_fraction  < 1.1f );
	ASSERT( out_box );

	rarch_map::iterator rarch;

	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return false;
	}

	rarch->second->get_bounding_box( out_box );
	
	return true;
}


//

bool COMAPI TriMesh::create_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );

	TriMeshInstance *mesh_instance;
	inst_map::iterator inst;
	rarch_map::iterator rarch;
	float radius;
	Vector center;
	
	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return false;
	}

	if( FAILED( rarch->second->create_instance( inst_index, &mesh_instance ) ) ) {
		return false;
	}

	rarch->second->get_bounding_sphere( &center, &radius );
	engine->set_instance_bounding_sphere( inst_index, EN_DONT_RECURSE, radius, center );

	if( (inst = instances.insert( inst_index, mesh_instance )) == instances.end() ) {
		mesh_instance->cleanup();
		return false;
	}

	return true;
}
                               
//

void COMAPI TriMesh::destroy_instance( INSTANCE_INDEX inst_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	inst_map::iterator inst;

	if( (inst = instances.find( inst_index )) != instances.end() ) {
		inst->second->cleanup();
		delete inst->second;
		instances.erase( inst_index );
	}
}

//

vis_state TriMesh::render_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, struct ICamera *camera, float lod_fraction, U32 flags, const Transform *tr )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( camera != NULL );

	rarch_map::iterator rarch;
	inst_map::iterator inst;
	vis_state vs = VS_UNKNOWN;
	
	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return vs;
	}

	if( (inst = instances.find( inst_index )) == instances.end() ) {
		return vs;
	}

	if( FAILED( inst->second->render( renderpipeline, vbuffermanager, renderprimitive, engine, camera, lod_fraction, flags, tr, &vs ) ) ) {
		return vs;
	}

	return vs;
}

//

void TriMesh::update_instance( INSTANCE_INDEX inst_index, float dt )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	inst_map::iterator inst;
	
	if( (inst = instances.find( inst_index )) != instances.end() ) {
		inst->second->update( dt );
	}
}

//

bool TriMesh::get_archetype_centroid( RENDER_ARCHETYPE render_arch_index, float lod_fraction, Vector& out_centroid )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( lod_fraction >= 0.0f );
	ASSERT( lod_fraction <  1.1f );

	rarch_map::iterator rarch;
	
	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return VS_UNKNOWN;
	}

	rarch->second->get_centroid( &out_centroid );

	return true;
}

//

bool COMAPI TriMesh::get_instance_bounding_box( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] ) 
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( lod_fraction >= 0.0f );
	ASSERT( lod_fraction <  1.1f );

	rarch_map::iterator rarch;
	
	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return VS_UNKNOWN;
	}

	rarch->second->get_bounding_box( out_box );

	return true;
}

//

bool COMAPI TriMesh::query_archetype_interface( RENDER_ARCHETYPE arch_index, const char *IID, IDAComponent **out_iif ) 
{
	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	rarch_map::iterator arch;

	if( (arch = render_archetypes.find( arch_index )) == render_archetypes.end() ) {
		return false;
	}

	if( FAILED( arch->second->QueryInterface( IID, (void**)out_iif ) ) ) {
		return false;
	}

	return true;
}

//

bool COMAPI TriMesh::query_instance_interface( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE, const char *IID, IDAComponent **out_iif ) 
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	inst_map::iterator inst;

	if( (inst = instances.find( inst_index )) == instances.end() ) {
		return false;
	}

	if( FAILED( inst->second->QueryInterface( IID, (void**)out_iif ) ) ) {
		return false;
	}

	return true;
}

//

Mesh * TriMesh::get_archetype_mesh( RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( render_arch_index != INVALID_ARCHETYPE_INDEX ) ;

	return NULL;
}

//

Mesh * TriMesh::get_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_ARCHETYPE_INDEX ) ;

	return NULL;
}

//

Mesh * TriMesh::get_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_ARCHETYPE_INDEX ) ;

	return NULL;
}

//

GENRESULT COMAPI TriMesh::release_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_ARCHETYPE_INDEX ) ;

	return GR_GENERIC;
}

//








//

BOOL COMAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
   IComponentFactory *server;
   ICOManager *DACOM ;

   if( fdwReason == DLL_PROCESS_ATTACH ) {

	    DA_HEAP_ACQUIRE_HEAP( HEAP );
		DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

		if( (server = new DAComponentFactory< DAComponent< TriMesh >, RendCompDesc >( CLSID_TriMesh )) == NULL ) {
			return TRUE;
		}

		if( (DACOM = DACOM_Acquire()) != NULL ) {
			DACOM->RegisterComponent( server,  CLSID_TriMesh, DACOM_NORMAL_PRIORITY );
		}

		server->Release();
	}

   return TRUE;
}



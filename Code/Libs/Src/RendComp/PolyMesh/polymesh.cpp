//****************************************************************************
//*                                                                          *
//*  POLYMESH.CPP: DA COM object-list renderer component for openFLAME 3D    *
//*               environment mesh files                                     *
//*                                                                          *
//*  Source compatible with 32-bit 80386 C/C++                               *
//*                                                                          *
//*  V1.00 of 23-Apr-97: Initial                                             *
//*                                                                          *
//*  $Header: /Libs/dev/Src/RendComp/PolyMesh/polymesh.cpp 75    3/21/00 4:30p Pbleisch $
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1997 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#pragma warning( disable : 4786 )

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <limits.h>
#include <map>

#include "dacom.h"
#include "TComponent.h"
#include "TSmartPointer.h"
#include "FDUMP.h"
#include "TempStr.h"

#include "3dmath.h"
#include "typedefs.h"
#include "view2d.h"
#include "SysConsumerDesc.h"
#include "FaceProp.h"
#include "RPUL/MTPrimitiveBuilder.h"
#include "da_heap_utility.h"
#include "packed_argb.h"
#include "TextureCoord.h"

#include "engine.h"
#include "filesys.h"
#include "renderer.h"
#include "ICamera.h"
#include "ILight.h"
#include "IProfileParser_Utility.h"
#include "IRenderComponent.h"
#include "LightMan.h"
#include "RendPipeline.h"
#include "IRenderPrimitive.h"
#include "IMaterialManager.h"

#include "Tfuncs.h"
#include "handlemap.h"

//

#include "MeshInstance.h"
#include "XMesh.h"

//

const char *CLSID_PolyMesh = "PolyMesh";

//

#pragma warning( disable : 4786 )


// ........................................................................
//
// POLYMESH
//
// Renders triangle mesh objects with optional continuous lod.
//
struct DACOM_NO_VTABLE POLYMESH : public IRenderComponent
{
public:	// Data
	BEGIN_DACOM_MAP_INBOUND(POLYMESH)
	DACOM_INTERFACE_ENTRY2(IID_IRenderComponent,IRenderComponent)
	DACOM_INTERFACE_ENTRY(IRenderComponent)
	END_DACOM_MAP()

public:	// Interface

	DA_HEAP_DEFINE_NEW_OPERATOR(POLYMESH)

	POLYMESH( void );
	~POLYMESH( void );
		
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

	void reset_vertex_cache(void);
	void get_pipe_info( void );

	void verify_pools( int size );
	void delete_pools( void );
	void verify_lists( int faces, int verts );
	void delete_lists( void );

	void render_facegroup_with_material( U32 rwm_flags, MeshInstance *object, const XMesh *mesh, FaceGroup *fgroup, Material *mat, U16 *index_list, U32 index_cnt, U16 *spec_index_list, U32 spec_index_cnt, U32 dip_flags, int local_light_map, Matrix4 &texxform );
	vis_state render( struct ICamera * camera, INSTANCE_INDEX object, MeshInstance *mesh_instance, const XMesh * mesh, const float lod_fraction, U32 flags, const Transform *tr);

protected:	// Data

	IEngine			 *engine;
	ITextureLibrary  *texturelibrary;
	ILightManager    *lightmanager;
	IRenderPipeline  *renderpipeline;
	IRenderPrimitive *renderprimitive;
	IMaterialManager  *materialmanager;

	typedef rarch_handlemap< XMesh* >		rarch_map;
	typedef inst_handlemap< MeshInstance* >	inst_map;
	
	rarch_map			render_archetypes;
	inst_map			instances;

	bool				got_pipe_info;

	int					pool_element_count;
	int					vertex_list_len;

	U16 *				vertex_slot;		// marks which facegroup vertices have been processed
	LightRGB *			shared_light_pool;
	LightRGB *			back_shared_light_pool;
	
	U32					default_material_flags;
	U32					device_supports_uvchannel1;
	U32					device_num_tss;
	
	U32					specular_mode;						// 0, 1, or 2
	ITL_TEXTURE_ID		specular_texture_id;
	U32					specular_irp_texture_id;
	float				specular_light_scale;
	char				specular_texture_name[64];

	U32					diffuse2_fallback_blend[2];		// src,dst framebuffer blend modes for Diffuse1*Diffuse2
	U32					emissive_fallback_blend[2];		// src,dst framebuffer blend modes for + Emissive
	U32					specular_fallback_blend[2];		// src,dst framebuffer blend modes for + Specular

	MTVERTEX *			mt_list;						// used instead of pbMain
	int					mt_list_len;
	int					mt_list_index;
	PrimitiveBuilder	pbSpecGood;
	unsigned short *	index_list;			
	unsigned short *	spec_good_index_list;			
	U16 *				spec_good_vertex_slot;		// marks which facegroup vertices have been processed for specular

	float				min_poly_size;			// smallest polygon size in pixels.  anythig smaller will not be rendered

	U32					cont_lod_mode;				// 0 no lod, 1 per instance lod(default), 2 per archetype lod
};

//

POLYMESH::POLYMESH( void ) 
{
	engine = NULL;
	lightmanager = NULL;
	renderpipeline = NULL;
	renderprimitive = NULL;
	texturelibrary = NULL;
	materialmanager = NULL;

	pool_element_count = 0;
	vertex_list_len = 0;

	vertex_slot = NULL;			// marks which facegroup vertices have been processed

	index_list = NULL;			
	spec_good_vertex_slot = NULL;			// marks which facegroup vertices have been processed
	spec_good_index_list = NULL;			

	mt_list = NULL;				// used instead of primitive builder
	mt_list_len = 0;

	specular_irp_texture_id = 0;
	specular_texture_id = ITL_INVALID_ID;
	specular_light_scale = 1.0f;
	specular_texture_name[0] = 0;
	specular_mode = 0;

	device_supports_uvchannel1 = 0;

	got_pipe_info = false;

	min_poly_size = .5f;

	
}

//

POLYMESH::~POLYMESH()
{
	if( texturelibrary && (specular_texture_id != ITL_INVALID_ID) ) {
		texturelibrary->release_texture_id( specular_texture_id );
		specular_texture_id = ITL_INVALID_ID;
	}

	//
	// Free archetypes & instances.
	//
	inst_map::iterator ibeg = instances.begin();
	inst_map::iterator iend = instances.end();
	inst_map::iterator inst;
	for( inst=ibeg; inst!=iend; inst++ ) {
		GENERAL_NOTICE( TEMPSTR( "PolyMesh: dtor: instance %d has dangling mesh\n", inst->first ) );
	}

	rarch_map::iterator abeg = render_archetypes.begin();
	rarch_map::iterator aend = render_archetypes.end();
	rarch_map::iterator arch;
	for( arch=abeg; arch!=aend; arch++ ) {
		GENERAL_NOTICE( TEMPSTR( "PolyMesh: dtor: render archetype %d has dangling mesh\n", arch->first ) );
	}

	engine = NULL;
	
	DACOM_RELEASE( lightmanager );
	DACOM_RELEASE( renderpipeline );
	DACOM_RELEASE( renderprimitive );
	DACOM_RELEASE( texturelibrary );
	DACOM_RELEASE( materialmanager );
	

	delete [] mt_list;
	mt_list = NULL;
	delete_pools();
	delete_lists();
}

//

GENRESULT POLYMESH::init (RendCompDesc* info)
{
	if( info->system_services == NULL || NULL == info->engine_services ) {
		return GR_INVALID_PARMS;		// must have a system container
	}

	if( FAILED( info->system_services->QueryInterface( IID_IRenderPrimitive, (void**) &renderprimitive ) ) ) {
		GENERAL_ERROR( "POLYMESH: Failed to get IRenderPrimitive\n" );
		return GR_GENERIC;
	}
	
	if( FAILED( info->system_services->QueryInterface( IID_IRenderPipeline, (void**) &renderpipeline ) ) ) {	
		GENERAL_ERROR( "POLYMESH: Failed to get IRenderPipeline" );
		return GR_GENERIC;
	}

	if( FAILED( info->system_services->QueryInterface ( IID_ITextureLibrary, (void**)&texturelibrary ) ) ) {
		GENERAL_ERROR( "POLYMESH: Failed to get ITextureLibrary" );
		return GR_GENERIC;
	}

	if( FAILED( info->system_services->QueryInterface ( IID_IMaterialManager, (void**)&materialmanager ) ) ) {
		GENERAL_ERROR( "POLYMESH: Failed to get IMaterialManager" );
		return GR_GENERIC;
	}

	if( FAILED( info->system_services->QueryInterface( IID_ILightManager, (void **) &lightmanager ) ) ) {
		GENERAL_NOTICE( "POLYMESH: Failed to get ILightManager" );
	}

	//
	// Get engine component resources
	// 
	if( FAILED( info->engine_services->QueryInterface(IID_IEngine, (void **) &engine ) ) ) {
		GENERAL_ERROR( "POLYMESH: Unable to get IEngine" );
		return GR_GENERIC;
	}
	engine->Release();

																	
	ICOManager *DACOM = DACOM_Acquire();
	opt_get_u32( DACOM, NULL, CLSID_PolyMesh, "SpecularMode", 0, &specular_mode );
	opt_get_string( DACOM, NULL, CLSID_PolyMesh, "SpecularTextureName",	"SpecularHighlight", specular_texture_name, 64 );

	opt_get_u32( DACOM, NULL, CLSID_PolyMesh, "ContLodMode", 1, &cont_lod_mode );

	return GR_OK;
}

//

GENRESULT COMAPI POLYMESH::set_render_property(const RenderProp name, DACOM_VARIANT value)
{
	GENRESULT result;

	switch( name ) 
	{
		case SUB_PIXEL_THRESHOLD:
			min_poly_size = value;
			result = GR_OK;
			break;
		default:
			result = GR_GENERIC;
	}

	return result;
}

//

GENRESULT COMAPI POLYMESH::get_render_property(const RenderProp name, DACOM_VARIANT value)
{
	GENRESULT result;

	switch(name)
	{
		case SUB_PIXEL_THRESHOLD:
			value = min_poly_size;
			result = GR_OK;
			break;
		default:
			result = GR_GENERIC;
	}

	return result;
}

//

void POLYMESH::update( float dt )
{
	inst_map::iterator ibeg = instances.begin();
	inst_map::iterator iend = instances.end();
	inst_map::iterator inst;

	for( inst=ibeg; inst!=iend; inst++ ) {
		if( inst->second->mesh ) {
			inst->second->update_uv( dt );
			inst->second->update_axm( dt, texturelibrary );
		}
	}
}

//

bool POLYMESH::create_archetype( RENDER_ARCHETYPE render_arch_index, IFileSystem *filesys )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( filesys != NULL );

	COMPTR<IFileSystem> meshDir;
	DAFILEDESC fdesc = "openFLAME 3D N-mesh";
	XMesh *mesh_archetype;

	if( FAILED( filesys->CreateInstance( &fdesc, meshDir ) ) ) {
		return false;
	}

	if( !got_pipe_info) {
		get_pipe_info();
	}

	if( (mesh_archetype = new XMesh) == NULL ) {
		return false;
	}

	if( !mesh_archetype->read( meshDir, texturelibrary,materialmanager, cont_lod_mode ) ) {
		delete mesh_archetype;
		return false;
	}

	if (mesh_archetype->material_cnt > 0)
	{
		for( int fg=0; fg<mesh_archetype->face_group_cnt; fg++ ) {
			mesh_archetype->material_list[ mesh_archetype->face_groups[ fg ].material ].flags |= default_material_flags;
		}
	}

	render_archetypes.insert( render_arch_index, mesh_archetype );

	return true;
}

//

bool COMAPI POLYMESH::duplicate_archetype( RENDER_ARCHETYPE new_render_arch_index, RENDER_ARCHETYPE old_render_arch_index )
{
	ASSERT( new_render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( old_render_arch_index != INVALID_RENDER_ARCHETYPE );

	XMesh *new_mesh_archetype = NULL;
	rarch_map::iterator rarch;
	
	if( (rarch = render_archetypes.find( old_render_arch_index )) == render_archetypes.end() ) {
		return false;
	}

	if( rarch->second->duplicate_mesh( &new_mesh_archetype, texturelibrary,materialmanager ) == false ) {
		return false;
	}

	render_archetypes.insert( new_render_arch_index, new_mesh_archetype );

	return true;
}

//

void POLYMESH::destroy_archetype( RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	
	rarch_map::iterator rarch;

	if( (rarch = render_archetypes.find( render_arch_index )) != render_archetypes.end() ) {
		delete rarch->second;
		render_archetypes.erase( render_arch_index );
	}
}

//

bool POLYMESH::split_archetype( RENDER_ARCHETYPE render_arch_index, const Vector& normal, float d, RENDER_ARCHETYPE r0, RENDER_ARCHETYPE r1, U32 sa_flags, INSTANCE_INDEX inst_index )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( r0 != INVALID_RENDER_ARCHETYPE );
	ASSERT( r1 != INVALID_RENDER_ARCHETYPE );

	const XMesh *mesh_archetype;
	XMesh *o0, *o1;
	rarch_map::iterator rarch;
	inst_map::iterator inst;

	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return false;
	}

	mesh_archetype = rarch->second;

	if( inst_index != INVALID_INSTANCE_INDEX ) {
		if( (inst = instances.find( inst_index )) != instances.end() ) {
			if( inst->second->local_mesh != NULL ) {
				mesh_archetype = inst->second->local_mesh;
			}
		}
	}

	ASSERT( mesh_archetype );

	if( !mesh_archetype->split_mesh( normal, d, o0, o1, sa_flags ) ) {
		return false;
	}

	ASSERT( o0 );
	ASSERT( o1 );

	render_archetypes.insert( r0, o0 );
	render_archetypes.insert( r1, o1 );
	
	return true;
}

//

bool COMAPI POLYMESH::get_archetype_statistics( RENDER_ARCHETYPE render_arch_index, float lod_fraction, enum StatType statistic, DACOM_VARIANT out_value ) 
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( lod_fraction >= 0.0f );
	ASSERT( lod_fraction  < 1.1f );

	const XMesh *mesh_archetype;
	
	rarch_map::iterator rarch;

	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return false;
	}

	mesh_archetype = rarch->second;

	switch( statistic ) {

	case ST_NUM_PRIMITIVES:
	case ST_NUM_FACES:
		if( ((unsigned long*)out_value) != NULL ) {
			*((unsigned long*)out_value) = (unsigned long)mesh_archetype->face_cnt;
			return true;
		}
		break;

	case ST_NUM_VERTICES:
		if( ((unsigned long*)out_value) != NULL ) {
			*((unsigned long*)out_value) = (unsigned long)mesh_archetype->object_vertex_cnt;
			return true;
		}
		break;
	}

	return true;
}

//

bool COMAPI POLYMESH::get_archetype_bounding_box( RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] ) 
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

bool COMAPI POLYMESH::create_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );

	MeshInstance *mesh_instance;
	inst_map::iterator inst;
	rarch_map::iterator rarch;
	
	if( !got_pipe_info) {
		get_pipe_info();
	}


	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return false;
	}

	if( (mesh_instance = new MeshInstance()) == NULL ) {
		return false;
	}

	mesh_instance->set_mesh( rarch->second, cont_lod_mode );
	
	if( mesh_instance->mesh ) {
		engine->set_instance_bounding_sphere( inst_index, EN_DONT_RECURSE, mesh_instance->mesh->radius, mesh_instance->mesh->sphere_center );
//		engine->set_centered_radius( inst_index, mesh_instance->mesh->radius, mesh_instance->mesh->sphere_center );
	}

	if( (inst = instances.insert( inst_index, mesh_instance )) == instances.end() ) {
		return false;
	}


	return true;
}
                               
//

void COMAPI POLYMESH::destroy_instance( INSTANCE_INDEX inst_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	inst_map::iterator inst;

	if( (inst = instances.find( inst_index )) != instances.end() ) {
		inst->second->free ();
		delete inst->second;
		instances.erase( inst_index );
	}
}

//

vis_state POLYMESH::render_instance( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, struct ICamera *camera, float lod_fraction, U32 flags, const Transform *tr )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( camera != NULL );

	const XMesh *mesh_archetype ;
	MeshInstance *mesh_instance;
	rarch_map::iterator rarch;
	inst_map::iterator inst;
	
	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return VS_UNKNOWN;
	}

	if( (inst = instances.find( inst_index )) == instances.end() ) {
		return VS_UNKNOWN;
	}
		
	mesh_instance  = inst->second;
	mesh_archetype = (mesh_instance->local_mesh != NULL) ? mesh_instance->local_mesh : mesh_instance->mesh;

	mesh_instance->set_mesh( rarch->second, cont_lod_mode );

	return render( camera, inst_index, mesh_instance, mesh_archetype, lod_fraction, flags, tr ) ;
}

//

void POLYMESH::update_instance( INSTANCE_INDEX inst_index, float dt )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	inst_map::iterator inst;
	
	if( (inst = instances.find( inst_index )) != instances.end() ) {
		if( inst->second->mesh ) {
			inst->second->update_uv( dt );
			inst->second->update_axm( dt, texturelibrary );
		}
	}
}

//

bool POLYMESH::get_archetype_centroid( RENDER_ARCHETYPE render_arch_index, float lod_fraction, Vector& out_centroid )
{
	ASSERT( render_arch_index != INVALID_RENDER_ARCHETYPE );
	ASSERT( lod_fraction >= 0.0f );
	ASSERT( lod_fraction <  1.1f );

	rarch_map::iterator rarch;
	
	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return VS_UNKNOWN;
	}

	out_centroid = rarch->second->centroid;

	return true;
}

//

bool COMAPI POLYMESH::query_archetype_interface( RENDER_ARCHETYPE, const char *, IDAComponent ** ) 
{
	return false;
}

//

bool COMAPI POLYMESH::get_instance_bounding_box( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index, float lod_fraction, SINGLE out_box[6] ) 
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

bool COMAPI POLYMESH::query_instance_interface( INSTANCE_INDEX, RENDER_ARCHETYPE, const char *, IDAComponent ** ) 
{
	return false;
}

//

void POLYMESH::get_pipe_info( void )
{
	U32 value, modes[4];
	RPDEVICEINFO rpdi;
	
	ICOManager *DACOM = DACOM_Acquire();

	renderpipeline->query_device_ability( RP_A_DEVICE_SOFTWARE, &value );
	
	if( value == 0xFFFFFFFF ) {
		GENERAL_WARNING( "POLYMESH: Trying to create archetypes/istances before calling IRenderPipeline::startup, YMMV." );
	}
	
	if( value ) {
		value = MF_NO_DIFFUSE2_PASS | MF_NO_EMITTER_PASS | MF_NO_SPECULAR_PASS;
	}
	else {
		value = 0;
	}

	renderpipeline->get_device_info( &rpdi );

	switch( rpdi.device_chipset_id ) {

	case RP_D_VOODOO_1:
	case RP_D_VOODOO_2:
	case RP_D_VOODOO_3:
	case RP_D_VOODOO_RUSH:
	case RP_D_VOODOO_BANSHEE:
		value = MF_NO_DIFFUSE2_PASS | MF_NO_EMITTER_PASS | MF_NO_SPECULAR_PASS;
		break;
	}

	opt_get_u32( DACOM, NULL, CLSID_PolyMesh, "DefaultMaterialFlags", value, &default_material_flags );


	renderpipeline->query_device_ability( RP_A_TEXTURE_STAGES, &value );
	opt_get_u32( DACOM, NULL, CLSID_PolyMesh, "NumTSS", value, &device_num_tss );

	renderpipeline->query_device_ability( RP_A_TEXTURE_COORDINATES, &value );
	value = (value==1)? FALSE : TRUE;
	opt_get_u32( DACOM, NULL, CLSID_PolyMesh, "Diffuse2AllowTC2", value, &device_supports_uvchannel1 );

	renderpipeline->query_device_ability( RP_A_BLEND_MUL_SRC, &value );
	opt_get_u32( DACOM, NULL, CLSID_PolyMesh, "Diffuse2SrcBlend", value, &diffuse2_fallback_blend[0] );
	
	renderpipeline->query_device_ability( RP_A_BLEND_MUL_DST, &value );
	opt_get_u32( DACOM, NULL, CLSID_PolyMesh, "Diffuse2DstBlend", value, &diffuse2_fallback_blend[1] );

	renderpipeline->query_device_ability( RP_A_BLEND_ADD_SRC, &value );
	opt_get_u32( DACOM, NULL, CLSID_PolyMesh, "EmissiveSrcBlend", value, &emissive_fallback_blend[0] );
	
	renderpipeline->query_device_ability( RP_A_BLEND_ADD_DST, &value );
	opt_get_u32( DACOM, NULL, CLSID_PolyMesh, "EmissiveDstBlend", value, &emissive_fallback_blend[1] );

	renderpipeline->query_device_ability( RP_A_BLEND_MATRIX, modes );

	opt_get_u32( DACOM, NULL, CLSID_PolyMesh, "SpecularSrcBlend", D3DBLEND_ONE, &specular_fallback_blend[0] );
	opt_get_u32( DACOM, NULL, CLSID_PolyMesh, "SpecularDstBlend", D3DBLEND_ONE, &specular_fallback_blend[1] );

	if( !rp_a_is_blend_supported( (D3DBLEND)specular_fallback_blend[0], (D3DBLEND)specular_fallback_blend[1], modes ) ) {
		specular_fallback_blend[0] = D3DBLEND_ONE;
		specular_fallback_blend[1] = D3DBLEND_ONE;
	}

	got_pipe_info = true;
}

//

bool pm_fl_map_gen( TC_UVGENERATORCONTEXT *ctx, void *user_context )
{
	float N_dot_L = dot_product( ctx->N, ctx->L );
	
	if( N_dot_L >= 0.0 ) {

		if( ctx->SpecularMode == 2 ) {
			Vector n = ctx->Tov->rotate(ctx->N);
			if( n.z > 0 ) {
				// intersect r with view xy plane.
				//
				Vector p = ctx->Tov->rotate_translate(ctx->P);
				float t = -p.z / n.z;
				Vector pp = p + t * n; 

				float depth_scale = 1 / 100.0F * n.z * (ctx->M->shininess_width);

				ctx->u = 0.5 + pp.x * depth_scale;
				ctx->v  = 0.5 + pp.y * depth_scale;
		
				return true;
			}
		}
	}

	return false;
}

//

bool pm_reflection_map_gen( TC_UVGENERATORCONTEXT *ctx, void *user_context )
{
	float N_dot_L = dot_product( ctx->N, ctx->L );
	
	if( N_dot_L >= 0.0 ) {

		float depth_scale;

		Vector R = 2.0 * (dot_product( ctx->N, ctx->L ) * (ctx->N)) - ctx->L;

		if( ctx->SpecularMode == 1 ) {
			float V_dot_R = dot_product( ctx->V, R );
			if( V_dot_R >= 0 ) {
				float n = ctx->M->shininess_width * 32.0;
				V_dot_R = pow( V_dot_R, n );
				int c = ((int)(V_dot_R * ctx->M->shininess));
				ctx->r = ctx->g = ctx->b = (c>0)?c:(c<255)?c:255;
			}
		}
		else if( ctx->SpecularMode == 2 ) {
			Vector r = ctx->Tov->rotate(R);

			if( r.z > 0 ) {

				// intersect r with view xy plane.
				//
				Vector p = ctx->Tov->rotate_translate(ctx->P);
				float t = -p.z / r.z;
				Vector pp = p + t * r; 

				// calc s,t coords for highlight
				//
				depth_scale = 1 / 100.0F * r.z * (ctx->M->shininess_width);

				ctx->u = 0.5 + pp.x * depth_scale;
				ctx->v  = 0.5 + pp.y * depth_scale;
		
				return true;
			}
		}
	}

	return false;
}

//

bool pm_simple_map_gen( TC_UVGENERATORCONTEXT *ctx, void *user_context )
{
	float N_dot_L = dot_product( ctx->N, ctx->L );
	
//	if( N_dot_L >= 0.0 ) {

		if( ctx->SpecularMode == 2 ) {
			Vector n = ctx->Tov->rotate(ctx->N);
			if( n.z > 0 ) {
				// intersect r with view xy plane.
				//
				ctx->u = 0.5 + n.x * 0.5 * 2.0f;
				ctx->v  = 0.5 + n.y * 0.5 * 2.0f;
		
				return true;
			}
		}
//	}

	return false;
}

//

bool pm_simple2_map_gen( TC_UVGENERATORCONTEXT *ctx, void *user_context )
{
	float N_dot_L = dot_product( ctx->N, ctx->L );
	
	if( N_dot_L >= 0.0 ) {

		if( ctx->SpecularMode == 2 ) {
			Vector n = ctx->Tov->rotate(ctx->N);
			if( n.z > 0 ) {
				// intersect r with view xy plane.
				//
				ctx->u = 0.5 + n.x * 0.5;
				ctx->v  = 0.5 + n.y * 0.5;
		
				return true;
			}
		}
	}

	return false;
}


//

bool pm_happy_map_gen( TC_UVGENERATORCONTEXT *ctx, void *user_context )
{
	//float N_dot_L = dot_product( ctx->N, ctx->L );

	Vector u_vec,v_vec;
	Vector Bob;
	if ((ctx->L-ctx->V).magnitude() == 0)
	{
		Bob.set(0,0,1);
		v_vec.set(0,1,0);
	}
	else
	{
		Bob = (ctx->L+ctx->V);
		SINGLE bobMag = Bob.magnitude();
		if (bobMag < 1e-3)
			return false;

		Bob /= bobMag;

		v_vec = cross_product(ctx->V,ctx->L);
		v_vec.normalize();
	}

	u_vec = cross_product(Bob,v_vec);
	
	float Nf_dot_Bob = dot_product(Bob,ctx->Nf);

		if( Nf_dot_Bob >= -0.3 ) {

		if( ctx->SpecularMode == 2 ) {
			Vector n = ctx->N;//ctx->Tow->rotate(ctx->N);
			if (dot_product(n,ctx->Nf) < 0.35)
				n = ctx->Nf;
			SINGLE facing = dot_product(n,Bob);
			if( facing > 0 ) {
				// intersect r with view xy plane.
				//
				ctx->u = 0.5 + dot_product(n,u_vec) * 0.5;
				ctx->v = 0.5 + dot_product(n,v_vec) * 0.5;
		
				return true;
			}
			else //if (facing > -0.7)
			{
				SINGLE x,y;
				x = dot_product(n,u_vec);
				y = dot_product(n,v_vec);
				if (x > 0)
					ctx->u = 1.5 - x * 0.5;
				else
					ctx->u = -0.5-x*0.5;
				if (y > 0)
					ctx->v = 1.5 - y * 0.5;
				else
					ctx->v = -0.5-y*0.5;
		
				return true;
			}
		}
	}

	return false;
}

//


void POLYMESH::delete_pools( void )
{
	delete [] vertex_slot;
	vertex_slot = NULL;
	
	delete [] spec_good_vertex_slot;
	spec_good_vertex_slot = NULL;

	delete [] shared_light_pool;
	shared_light_pool = NULL;

	delete [] back_shared_light_pool;
	back_shared_light_pool = NULL;

	pool_element_count = 0;
}

//

void POLYMESH::verify_pools(int size)
{
	// Check that caches and pools are large enough.
	if (pool_element_count < size)
	{
		delete_pools();

		pool_element_count = size;

		vertex_slot			= new U16[pool_element_count];
		
		spec_good_vertex_slot	= new U16[pool_element_count];

		shared_light_pool = new LightRGB[pool_element_count];
	
		back_shared_light_pool = new LightRGB[pool_element_count];
	}
}


//

void POLYMESH::delete_lists( void )
{
	delete [] index_list;			
	index_list = NULL;

	delete [] spec_good_index_list;			
	spec_good_index_list = NULL;

	vertex_list_len = 0;
}

//

void POLYMESH::verify_lists( int faces, int verts )
{
	if( vertex_list_len < verts ) {
		
		delete_lists();

		index_list = new unsigned short[verts];
		spec_good_index_list = new unsigned short[verts];

		vertex_list_len = verts;
	}
}


//

#define RWM_DONT_TEXTURE	(1<<0)

void POLYMESH::render_facegroup_with_material( U32 rwm_flags, MeshInstance *object, const XMesh *mesh, FaceGroup *fgroup, Material *mat, U16 *index_list, U32 index_cnt, U16 *spec_index_list, U32 spec_index_cnt, U32 dip_flags, int local_light_map, Matrix4 &texxform )
{
	// avoid setting unnecassary state
	if( !index_cnt )
		return;

	// NOTES
	//
	// o Try to always put D3DTA_TEXTURE in ARG1 as some hardware
	//   cannot handle _TEXTURE in ARG2 for *some* operations.  (TNT)
	// 
	// o Try to put _CURRENT in ARG2 for similar reasons (ATI?)
	//

//	U32 mat_uid = (((U32)mat)<<16) | (mat->unique<<8);
	U32 mat_uid = (U32)mat+mat->unique;


	// Set common state for all passes
	//
	renderprimitive->set_render_state( D3DRS_LIGHTING, FALSE );

	renderprimitive->set_texture_stage_state( 0, D3DTSS_ALPHAOP,		D3DTOP_SELECTARG2 );
	renderprimitive->set_texture_stage_state( 0, D3DTSS_ALPHAARG1,		D3DTA_TEXTURE );
	renderprimitive->set_texture_stage_state( 0, D3DTSS_ALPHAARG2,		D3DTA_DIFFUSE );
	renderprimitive->set_texture_stage_state( 0, D3DTSS_COLOROP,		D3DTOP_SELECTARG2 );
	renderprimitive->set_texture_stage_state( 0, D3DTSS_COLORARG1,		D3DTA_TEXTURE );
	renderprimitive->set_texture_stage_state( 0, D3DTSS_COLORARG2,		D3DTA_DIFFUSE );
	renderprimitive->set_sampler_state( 0, D3DSAMP_MINFILTER,		 D3DTEXF_LINEAR );
	renderprimitive->set_sampler_state( 0, D3DSAMP_MAGFILTER,		 D3DTEXF_LINEAR );
	renderprimitive->set_sampler_state( 0, D3DSAMP_MIPFILTER,		 D3DTEXF_POINT );

	if( GET_TC_WRAP_MODE(mat->texture_flags) == TC_WRAP_UV_1 && mesh->texture_batch_list2 )
	{
		renderprimitive->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	1 );
	}
	else
	{
		renderprimitive->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	0 );
	}

	renderprimitive->set_render_state( D3DRS_ZENABLE,			TRUE );
	renderprimitive->set_render_state( D3DRS_ZWRITEENABLE,	TRUE );


	// Transparency
	//
	U32 blend_needed = 0;
	U32 blend_enabled = 0;
	U32 stage_cnt = 0;
	U32 stage_zero_flags = 0;


	if( mat->transparency < 255 ) {
		blend_needed++;
	}

#if 1

	// Diffuse1
	//
	if( !(mat->flags & MF_NO_DIFFUSE1_PASS) )
	{

		if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
			renderprimitive->set_texture_stage_texture( stage_cnt, mat->texture_id );
		
			if( mat->texture_id ) {
			
				renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_COLOROP,	D3DTOP_MODULATE );
				renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAOP,	D3DTOP_MODULATE );
				renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_ADDRESSU,	MAT_GET_ADDR_MODE(mat->texture_flags,0) );
				renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_ADDRESSV,	MAT_GET_ADDR_MODE(mat->texture_flags,1) );
					
				if( mat->texture_id && (mat->texture_flags & TF_F_HAS_ALPHA) ) {
					blend_needed++;
				}

				if( !stage_cnt ) {
					stage_zero_flags = MF_NO_DIFFUSE1_PASS;
				}
				stage_cnt++;
			}
		}
		else {
			renderprimitive->set_texture_stage_texture( stage_cnt, 0 );
		}
	}

	// Diffuse2
	//
	// Don't setup multitexture if we already know or are told to do
	// multpass.
	//
	// Don't setup multitexture if we are not to do any diffuse2 things.
	//
	if( !(stage_cnt && (mat->num_passes > 1)) && !(mat->flags & MF_NO_DIFFUSE2_PASS) ) {

		if( mat->second_diffuse_texture_id || (!mat->texture_id && !mat->emissive_texture_id) ) {
		
			if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
				renderprimitive->set_texture_stage_texture( stage_cnt, mat->second_diffuse_texture_id );
			
				if( GET_TC_WRAP_MODE(mat->second_diffuse_texture_flags) == TC_WRAP_UV_1 && mesh->texture_batch_list2 )
				{
					renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX,	1 );
				}
				else
				{
					renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX,	0 );
				}

				renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_COLOROP,	D3DTOP_MODULATE );
				renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAOP,	D3DTOP_MODULATE );
				renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_ADDRESSU,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,0) );
				renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_ADDRESSV,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,1) );

				if( stage_cnt ) {
					renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_COLORARG1,	D3DTA_TEXTURE );
					renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_COLORARG2,	D3DTA_CURRENT );
					renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE );
					renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG2,	D3DTA_CURRENT );
					renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_MINFILTER,	D3DTEXF_LINEAR );
					renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_MAGFILTER,	D3DTEXF_LINEAR );
					renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_MIPFILTER,	D3DTEXF_POINT );
				}

				if( mat->second_diffuse_texture_id && (mat->second_diffuse_texture_flags & TF_F_HAS_ALPHA) ) {
					blend_needed++;
				}
				
				if( !stage_cnt ) {
					stage_zero_flags = MF_NO_DIFFUSE2_PASS;
				}
				stage_cnt++;
			}
			else {
				renderprimitive->set_texture_stage_texture( stage_cnt, 0 );
			}
		}
	}

	// Emissive
	//
	if( !(stage_cnt && (mat->num_passes > 1)) && !(mat->flags & MF_NO_EMITTER_PASS) && mat->emissive_texture_id ) {
		
		// emissive_blend

		if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
			renderprimitive->set_texture_stage_texture( stage_cnt, mat->emissive_texture_id );
		
			if( GET_TC_WRAP_MODE(mat->emissive_texture_flags) == TC_WRAP_UV_1 && mesh->texture_batch_list2 )
			{
				renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX,	1 );
			}
			else
			{
				renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX,	0 );
			}

			renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_COLOROP,	D3DTOP_ADD );
			renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_ADDRESSU,	MAT_GET_ADDR_MODE(mat->emissive_texture_flags,0) );
			renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_ADDRESSV,	MAT_GET_ADDR_MODE(mat->emissive_texture_flags,1) );
			
			if( stage_cnt ) {
				renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_COLORARG1,	D3DTA_TEXTURE );
				renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_COLORARG2,	D3DTA_CURRENT );
				renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE );
				renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG2,	D3DTA_CURRENT );
				renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_MINFILTER,	D3DTEXF_LINEAR );
				renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_MAGFILTER,	D3DTEXF_LINEAR );
				renderprimitive->set_sampler_state( stage_cnt, D3DSAMP_MIPFILTER,	D3DTEXF_POINT );
			}
			
			if( mat->emissive_texture_id && (mat->emissive_texture_flags & TF_F_HAS_ALPHA) ) {
				blend_needed++;
			}
			
			if( !stage_cnt ) {
				stage_zero_flags = MF_NO_EMITTER_PASS;
			}
			stage_cnt++;
		}
		else {
			renderprimitive->set_texture_stage_texture( stage_cnt, 0 );
		}
	}

	// Set up blending
	//
	if( blend_needed == 0 || (mat->flags & MF_ENABLE_ALPHA_BLEND_NEVER) ) {
		renderprimitive->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );
	}
	else {
		renderprimitive->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
		renderprimitive->set_render_state( D3DRS_SRCBLEND, mat->src_blend );
		renderprimitive->set_render_state( D3DRS_DESTBLEND, mat->dst_blend );
	}

	if ( mat->flags & MF_ENABLE_DEPTH_WRITES_ALWAYS )
		renderprimitive->set_render_state( D3DRS_ZWRITEENABLE, TRUE );

	renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_COLOROP, D3DTOP_DISABLE );
	renderprimitive->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

	if( mat->num_passes <= 0 ) {
		if( SUCCEEDED( renderprimitive->verify_state() ) ) {
			mat->num_passes = 1;
		}
		else {
			mat->num_passes = Tmax<U32>(1, stage_cnt);
		}
	}

#endif

	U32 enable_specular = mat->shininess && (mat->shininess_width > 0.0) && !(mat->flags & MF_NO_SPECULAR_PASS);

	if( enable_specular && specular_mode == 1 ) {
		renderprimitive->set_render_state( D3DRS_SPECULARENABLE, TRUE );
	}
	else {
		renderprimitive->set_render_state( D3DRS_SPECULARENABLE, FALSE );
	}
	
	renderprimitive->set_state( RPR_STATE_ID, mat_uid++ );

	if( mat->num_passes > 1 ) {
	
		U32 pass_cnt = 0;

		// This texture count/configuration can't be done in one pass
		// do multiple passes.
		
		// Stage0 will already be setup above
		renderprimitive->draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, mt_list, mt_list_index, index_list, index_cnt, dip_flags );
		
		if( enable_specular && specular_mode == 1 ) {
			renderprimitive->set_render_state( D3DRS_SPECULARENABLE, FALSE );
		}

		// set up framebuffer blending for next N passes
		//
		renderprimitive->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
		renderprimitive->set_render_state( D3DRS_ZENABLE, TRUE );
		renderprimitive->set_render_state( D3DRS_ZWRITEENABLE, FALSE );
		renderprimitive->set_render_state( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );


		// Diffuse2
		//
		if( (stage_zero_flags != MF_NO_DIFFUSE2_PASS) && !(mat->flags & MF_NO_DIFFUSE2_PASS) && mat->second_diffuse_texture_id ) {

			renderprimitive->set_render_state( D3DRS_SRCBLEND, diffuse2_fallback_blend[0] );
			renderprimitive->set_render_state( D3DRS_DESTBLEND, diffuse2_fallback_blend[1] );

			if( GET_TC_WRAP_MODE(mat->second_diffuse_texture_flags) == TC_WRAP_UV_1 && mesh->texture_batch_list2 )
			{
				renderprimitive->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	1 );
			}
			else
			{
				renderprimitive->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	0 );
			}

			if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
				renderprimitive->set_texture_stage_texture( 0, mat->second_diffuse_texture_id );
			}
			else {
				renderprimitive->set_texture_stage_texture( 0, 0 );
			}
			
			renderprimitive->set_texture_stage_state( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
			renderprimitive->set_texture_stage_state( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

			renderprimitive->set_sampler_state( 0, D3DSAMP_ADDRESSU,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,0) );
			renderprimitive->set_sampler_state( 0, D3DSAMP_ADDRESSV,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,1) );
			
			renderprimitive->set_state( RPR_STATE_ID, mat_uid++ );
		
			renderprimitive->draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, mt_list, mt_list_index, index_list, index_cnt, dip_flags ) ;
		}

		// Emissive
		//
		if( (stage_zero_flags != MF_NO_EMITTER_PASS) && !(mat->flags & MF_NO_EMITTER_PASS) && mat->emissive_texture_id ) {
			
			renderprimitive->set_render_state( D3DRS_SRCBLEND, emissive_fallback_blend[0] );
			renderprimitive->set_render_state( D3DRS_DESTBLEND, emissive_fallback_blend[1] );
			
			if( GET_TC_WRAP_MODE(mat->emissive_texture_flags) == TC_WRAP_UV_1 && mesh->texture_batch_list2 )
			{
				renderprimitive->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	1 );
			}
			else
			{
				renderprimitive->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	0 );
			}

			if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
				renderprimitive->set_texture_stage_texture( 0, mat->emissive_texture_id );
			}
			else {
				renderprimitive->set_texture_stage_texture( 0, 0 );
			}
			
			renderprimitive->set_texture_stage_state( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
			renderprimitive->set_texture_stage_state( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

			renderprimitive->set_sampler_state( 0, D3DSAMP_ADDRESSU, MAT_GET_ADDR_MODE(mat->emissive_texture_flags,0) );
			renderprimitive->set_sampler_state( 0, D3DSAMP_ADDRESSV, MAT_GET_ADDR_MODE(mat->emissive_texture_flags,1) );
			
			renderprimitive->set_state( RPR_STATE_ID, mat_uid++ );

			renderprimitive->draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, mt_list, mt_list_index, index_list, index_cnt, dip_flags ) ;
		
			renderprimitive->set_render_state( D3DRS_ZFUNC, D3DCMP_LESS ) ;
		}
	}
	else if( mat->num_passes >= 0 ) {

		renderprimitive->draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, mt_list, mt_list_index, index_list, index_cnt, dip_flags ) ;
	}

	if( enable_specular && specular_mode == 2 ) {

		if( local_light_map != 0 ) {

			// set up framebuffer blending for specular pass
			//
			renderprimitive->set_render_state( D3DRS_ZENABLE, TRUE ) ;
			renderprimitive->set_render_state( D3DRS_ZWRITEENABLE, FALSE ) ;
			renderprimitive->set_render_state( D3DRS_ZFUNC, D3DCMP_LESSEQUAL ) ;

			renderprimitive->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE ) ;
			renderprimitive->set_render_state( D3DRS_SRCBLEND, specular_fallback_blend[0] );
			renderprimitive->set_render_state( D3DRS_DESTBLEND, specular_fallback_blend[1] );
			
			if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
				renderprimitive->set_texture_stage_texture( 0, local_light_map );
			}
			else {
				renderprimitive->set_texture_stage_texture( 0, 0 );
			}
			
			renderprimitive->set_texture_stage_state( 0, D3DTSS_COLOROP,	D3DTOP_MODULATE );
			renderprimitive->set_texture_stage_state( 0, D3DTSS_COLORARG1,	D3DTA_TEXTURE );
			renderprimitive->set_texture_stage_state( 0, D3DTSS_COLORARG2,	D3DTA_CURRENT );
			renderprimitive->set_texture_stage_state( 0, D3DTSS_ALPHAOP,	D3DTOP_MODULATE );
			renderprimitive->set_texture_stage_state( 0, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE );
			renderprimitive->set_texture_stage_state( 0, D3DTSS_ALPHAARG2,	D3DTA_CURRENT );
			renderprimitive->set_sampler_state( 0, D3DSAMP_ADDRESSU,	D3DTADDRESS_CLAMP );
			renderprimitive->set_sampler_state( 0, D3DSAMP_ADDRESSV,	D3DTADDRESS_CLAMP );

//PTEX					renderprimitive->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION );
//PTEX					renderprimitive->set_texture_stage_state( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3 | D3DTTFF_PROJECTED );
//PTEX					renderpipeline->set_texture_stage_transform( 0, texxform );

			renderprimitive->set_texture_stage_texture( 1, 0 );
			renderprimitive->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
			renderprimitive->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

			renderprimitive->set_state( RPR_STATE_ID, mat_uid++ );

			U32 btm;

			renderprimitive->get_state( RPR_BATCH_TRANSLUCENT_MODE, &btm );
			renderprimitive->set_state( RPR_BATCH_TRANSLUCENT_MODE, RPR_TRANSLUCENT_UNSORTED );

			renderprimitive->draw_indexed_primitive( D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX, pbSpecGood.vertex_buffer, pbSpecGood.current_vertex, spec_index_list, spec_index_cnt, dip_flags ) ;
//PTEX		renderprimitive->draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, mt_list, mt_list_index, index_list, index_cnt, dip_flags ) ;
			
			renderprimitive->set_render_state( D3DRS_ZFUNC, D3DCMP_LESS ) ;

			renderprimitive->set_state( RPR_BATCH_TRANSLUCENT_MODE, btm );

			Matrix4 I;

			I.set_identity();

//PTEX					renderpipeline->set_texture_stage_transform( 0, I );
		}
	}

	renderprimitive->set_state( RPR_STATE_ID, 0 );
}

//

vis_state POLYMESH::render( ICamera * camera, INSTANCE_INDEX object_index, MeshInstance *mesh_instance, 
							const XMesh * mesh, const float lod_fraction, U32 flags, const Transform *tr)
{
	vis_state result = VS_UNKNOWN;

	if( mesh->face_cnt == 0 && lod_fraction == 0.0f ) {
		return VS_NOT_VISIBLE;
	}

	MeshInstance * object = mesh_instance;

	Transform world_to_view ( camera->get_inverse_transform() );
	
	Transform object_to_world(false);
	if(tr != NULL)
	{
		object_to_world = *tr * engine->get_transform(object_index);
	}
	else
	{
		object_to_world = engine->get_transform(object_index);
	}
 
	// NOTE:
	// zero_D is a hack to keep things simple and fast
	// if things get smaller face visibility checks don't work w/o recalculating D
	// but to do that correctly we would also have to recompute the plane equation/normal
	// for each face.  setting D to 0 gets around this as 0 is scale independent.  note, the normal
	// direction can still be wrong so lighting can be off
	bool zero_D;
	Vector svec;
	if(tr)
	{	
		svec = Vector(tr->get_i().magnitude(), tr->get_j().magnitude(), tr->get_k().magnitude());
		zero_D = (svec.x < 1.0 || svec.y < 1.0 || svec.z < 1.0);
	}
	else
	{
		svec = Vector(1, 1, 1);
		zero_D = false;
	}
	
	{
		if( !(flags & RF_SAME_LOD) )
		{
			object->set_lod( lod_fraction );
		}

		Transform object_to_view ( world_to_view * object_to_world ); // world_to_view.multiply(object_to_world);

		verify_lists(mesh->face_cnt, 3 * mesh->face_cnt);

		Vector cam_pos = camera->get_position();
		Vector cam_pos_in_object_space = object_to_world.inverse_rotate_translate(cam_pos);

		float cx = cam_pos_in_object_space.x;
		float cy = cam_pos_in_object_space.y;
		float cz = cam_pos_in_object_space.z;
	
	//
	// Make sure our data pools are large enough for this object.
	//
		verify_pools( Tmax( mesh->object_vertex_cnt, Tmax( 4*mesh->face_cnt, mesh->vertex_batch_cnt ) ) );


	//
	// Would be nice if we could avoid this matrix inversion.
	//
		Transform light_world_to_object(false);
		// take out any scale that might have been in tr for lighting purposes
		// otherwise normals get scaled and so do dot products
		if(tr)
		{
			light_world_to_object = object_to_world;
			light_world_to_object.set_i( light_world_to_object.get_i().normalize() );
			light_world_to_object.set_j( light_world_to_object.get_j().normalize() );
			light_world_to_object.set_k( light_world_to_object.get_k().normalize() );
			light_world_to_object = light_world_to_object.get_inverse();
		}
		else
		{
			light_world_to_object = object_to_world.get_inverse();
		}

	//
	// Compute incident light at vertices.
	//
	// THIS LIGHTING CODE COMBINES ALL LIGHT CONTRIBUTIONS BEFORE TAKING
	// ANY MATERIAL PROPERTIES INTO ACCOUNT. NON-OpenGL lighting model.
	//
	// IF THERE ARE NO SMOOTH-SHADED FACES, Skip this step.
	//
		
		// the first time RF_DONT_RELIGHT is used we allocate a light pool for the instance
		if( (flags & RF_DONT_RELIGHT) && (object->smooth_light_pool == NULL) && mesh->have_smooth_shaded_faces )
		{
			object->create_light_pools();
			flags &= ~RF_DONT_RELIGHT; // have to relight the first time
		}

		LightRGB *current_light_pool = (object->smooth_light_pool)
			? object->smooth_light_pool
			: shared_light_pool;
		LightRGB *back_current_light_pool = (object->back_smooth_light_pool)
			? object->back_smooth_light_pool
			: back_shared_light_pool;

		const ClampFlags light_clamp = (flags & RF_CLAMP_COLOR) ? CF_COLOR : CF_INTENSITY;
		
		if ( lightmanager )
		{
			if( !(flags & RF_DONT_RELIGHT) && mesh->have_smooth_shaded_faces )
			{
				if( mesh->have_double_sided_faces > 0 )
				{
					lightmanager->light_vertices_strided( current_light_pool, sizeof(LightRGB),
													  mesh->object_vertex_list, sizeof(Vector),
													  mesh->normal_ABC, sizeof(Vector),
													  (U32*)mesh->vertex_normal, sizeof(U32),
													  mesh->object_vertex_cnt, &light_world_to_object, light_clamp,
													  back_current_light_pool);								  
				}
				else
				{
					lightmanager->light_vertices_strided( current_light_pool, sizeof(LightRGB),
													  mesh->object_vertex_list, sizeof(Vector),
													  mesh->normal_ABC, sizeof(Vector),
													  (U32*)mesh->vertex_normal, sizeof(U32),
													  mesh->object_vertex_cnt, &light_world_to_object, light_clamp);
				}
			}
		}
		else
		{
			if( mesh->have_smooth_shaded_faces )
			{
				for(int i = 0; i < mesh->object_vertex_cnt; i++)
				{
					current_light_pool[i].r = 
					current_light_pool[i].g = 
					current_light_pool[i].b = 255;

					back_current_light_pool[i].r =
					back_current_light_pool[i].g =
					back_current_light_pool[i].b = 255;
				}
			}
		}

		U32 rwm_flags;

		renderpipeline->get_pipeline_state( RP_TEXTURE, &rwm_flags );
		if( !rwm_flags ) {
			rwm_flags = RWM_DONT_TEXTURE;
		}
		else {
			rwm_flags = 0;
		}

		ILight *lights[8];
		int num_lights = 0;
		int spec_light_index = 0;
		bool use_specular_materials = false;
		U32 light_map = 0;
		Matrix4 texxform, proj;
		Transform lightmv;
		Vector light_in_world;


		TC_UVGENERATORCONTEXT ctx;

		ctx.SpecularMode = specular_mode;
		ctx.Tov = &object_to_view;
		ctx.Tow = &object_to_world;
		ctx.Vo = cam_pos_in_object_space;

		num_lights = lightmanager->get_active_lights( lights );

		if( specular_mode != 0 && lightmanager && num_lights != 0 && mesh->tc_user_generator ) {

			int l;

			for( l=0; l<num_lights; l++ ) {

				light_map = lights[l]->GetMap();

				if( light_map != 0 && (light_map != 0xFFFFFFFF) ) {

					lights[l]->GetPosition( light_in_world );
					
					ctx.Lo = object_to_world.inverse_rotate_translate( light_in_world );

					spec_light_index = l;

					use_specular_materials = true;

					break;
				}
			}

			if( l == num_lights ) {

				if( specular_irp_texture_id == 0 ) {

					ITL_TEXTUREFRAME_IRP frame;
				
					if( SUCCEEDED( texturelibrary->has_texture_id( specular_texture_name ) ) ) {
					
						if( SUCCEEDED( texturelibrary->get_texture_id( specular_texture_name, &specular_texture_id ) ) ) {

							texturelibrary->get_texture_frame( specular_texture_id, 0, &frame ); 
							
							specular_irp_texture_id = frame.rp_texture_id;
						}

					}
					else {
						specular_irp_texture_id = 0;
					}

				}

				if( (light_map = specular_irp_texture_id) != 0 ) {
						
					use_specular_materials = true;

					lights[0]->GetPosition( light_in_world );

					ctx.Lo = object_to_world.inverse_rotate_translate( light_in_world );

					spec_light_index = 0;
				}
			}

//PTEX					renderpipeline->get_projection( proj );
//PTEX					renderpipeline->set_lookat( ctx.Lo.x, ctx.Lo.y, ctx.Lo.z, 
//PTEX												object_to_world.translation.x, object_to_world.translation.y, object_to_world.translation.z, 
//PTEX												0, 1, 0 );
//PTEX					renderpipeline->get_modelview( lightmv );

//PTEX					texxform = proj * lightmv;
		}

		renderprimitive->set_modelview( object_to_view ) ;

		// done by polymesh itself
		renderprimitive->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE ) ;


		Vector centroid_in_camera ( object_to_view * mesh->centroid );
		// objects that have morphed due to LOD can't use this optimization
		// because their face areas are invalid
		const float half_pixel_area_at_object = ( mesh->lod_step_list && (lod_fraction != 1.0f) ) ? 0.0f :
			min_poly_size * (centroid_in_camera.z * centroid_in_camera.z) /
			( camera->get_znear() * camera->get_znear() * camera->get_hpc() * -camera->get_vpc() );

	//
	// Build master list of vertex/texcoord pairs ONCE, then fill in colors for
	// each face group.
	//
		if(mt_list_len < 3 * mesh->face_cnt)
		{
			delete [] mt_list;
			mt_list_len = 3 * mesh->face_cnt;
			mt_list = new MTVERTEX[mt_list_len];
		}
		
		
		FaceGroup * group = mesh->face_groups;
		for (int i = 0; i < mesh->face_group_cnt; i++, group++)
		{
			
			// VISIBILITY PER FACE GROUP.
			if( group->face_cnt == 0 ) {
				continue;
			}

			int * normal_index = group->face_normal;
			float * D = group->face_D_coefficient;

			memset(vertex_slot, 0xff, sizeof(short) * mesh->vertex_batch_cnt);

			Material *mat = ctx.M = mesh->material_list + group->material;

			bool specular_material = false;

#if 1 // PTEX
			if( use_specular_materials && mat->shininess != 0 && mat->shininess_width != 0 ) {
			
				specular_material = true;

				if( specular_mode == 2 ) {
					
					LightRGB color;

					lights[spec_light_index]->GetColor(color);

					memset( spec_good_vertex_slot, 0xff, sizeof(short) * mesh->vertex_batch_cnt);
					
					pbSpecGood.Begin( D3DPT_TRIANGLELIST );
					
					int r = Tclamp( 0, 255, (color.r * mat->shininess)>>8  );
					int g = Tclamp( 0, 255, (color.g * mat->shininess)>>8 );
					int b = Tclamp( 0, 255, (color.b * mat->shininess)>>8 );
					
					pbSpecGood.Color4ub( r, g, b, mat->shininess );
				}
			}
#endif

		//
		// AMBIENT is already accounted for in the light_pool values.
		// We're basically rolling ambient and diffuse together here.
		//
			unsigned int base_r = 0, base_g = 0, base_b = 0;

			if (mat->flags & MF_EMITTER)
			{
				base_r += mat->emission.r;
				base_g += mat->emission.g;
				base_b += mat->emission.b;
			}

			mt_list_index = 0;

			U16 *idx_ptr = index_list;
			U16 *spec_good_idx_ptr = spec_good_index_list;
			LightRGB flat[3];
			unsigned int r, g, b;
			int max_c;

			for (int f = 0; f < group->face_cnt; f++)
			{
				FACE_PROPERTY * fp = group->face_properties + f;

				if ( !(*fp & HIDDEN) )
				{
					if(group->face_area[f] < half_pixel_area_at_object )
					{
						if(mesh->sorted_faces == 0)
						{
							continue;	// lod meshes can't have all faces sorted
						}
						else
						{
							break;
						}
					}

					U32 is_face_dbl = *fp & TWO_SIDED;
					U32 is_face_flat = *fp & FLAT_SHADED;
					U32 face_normal_flag = 0x00000000;	// dont flip the normal	
					U32 face_normal_idx = group->face_normal[f];
					
					ctx.Nf = mesh->normal_ABC[face_normal_idx];
					
					float face_D = ((zero_D) ? 0.0 : -(group->face_D_coefficient[f]));
					U32 dot_check = (ctx.Nf.x*cx + ctx.Nf.y*cy + ctx.Nf.z*cz) <= face_D;

					if( is_face_dbl && dot_check ) {						
						face_normal_flag = 0x80000000;	// normal is on the wrong side of a double sided poly
					}
					else if( dot_check && !(flags & RF_DONT_CULL_FACES) ) {
						continue;						// cull backfacing single sided poly
					}

				
					if (*fp & FLAT_SHADED)
					{
					//
					// Compute lighting based on face normal for flat-shaded polys.
					//
					// we copy vertices so that LIGHTMAN does not have to be called 3 times -ms

						Vector light_normal[3];

						// use flipped normal of double sided face if necessary -ms
						if (face_normal_flag) {
							light_normal[0] = light_normal[1] = light_normal[2] = -ctx.Nf;
						}
						else {
							light_normal[0] = light_normal[1] = light_normal[2] = ctx.Nf;
						}

						const int * first = group->face_vertex_chain + f * 3;

						const Vector light_vertex[3] = {
							*(mesh->object_vertex_list + mesh->vertex_batch_list[*(first+0)]),
							*(mesh->object_vertex_list + mesh->vertex_batch_list[*(first+1)]),
							*(mesh->object_vertex_list + mesh->vertex_batch_list[*(first+2)]) };

						if( lightmanager ) 
						{
							lightmanager->light_vertices(flat, light_vertex, light_normal, 3, &light_world_to_object, light_clamp);
						}
						else
						{
							flat[0].r = flat[0].g = flat[0].b =
							flat[1].r = flat[1].g = flat[1].b =
							flat[2].r = flat[2].g = flat[2].b = 255;
						}

						if( mesh->vertex_color_list )
						{
							int bid;

							bid = 3 * mesh->vertex_batch_list[*(first+0)];
							flat[0].r = (flat[0].r * mesh->vertex_color_list[bid  ]) >> 8;
							flat[0].g = (flat[0].g * mesh->vertex_color_list[bid+1]) >> 8;
							flat[0].b = (flat[0].b * mesh->vertex_color_list[bid+2]) >> 8;

							bid = 3 * mesh->vertex_batch_list[*(first+1)];
							flat[1].r = (flat[1].r * mesh->vertex_color_list[bid  ]) >> 8;
							flat[1].g = (flat[1].g * mesh->vertex_color_list[bid+1]) >> 8;
							flat[1].b = (flat[1].b * mesh->vertex_color_list[bid+2]) >> 8;

							bid = 3 * mesh->vertex_batch_list[*(first+2)];
							flat[2].r = (flat[2].r * mesh->vertex_color_list[bid  ]) >> 8;
							flat[2].g = (flat[2].g * mesh->vertex_color_list[bid+1]) >> 8;
							flat[2].b = (flat[2].b * mesh->vertex_color_list[bid+2]) >> 8;
						}
					}

					int * chain = group->face_vertex_chain + f * 3;
					int flat_slot = 0;
					int spec_flat_slot = 0;
					U32 spec_valid[3] = { 0,0,0 };

				// TOO DAMN MANY FLAT V. SMOOTH COMPARES. DO IT ONCE AND EXECUTE ENTIRELY
				// DIFFERENT BLOCKS OF CODE.
					for (int v = 0; v < 3; v++, chain++)
					{
					//
					// For flat-shaded polys, we can't in general share vertices
					// because they'll have different light values.
					//
						if (is_face_dbl || *fp & FLAT_SHADED || (vertex_slot[*chain] == 0xFFFF))
						{
						// Must copy vertex to list.

							MTVERTEX & current_mt = mt_list[mt_list_index];

							int vchain = mesh->vertex_batch_list[*chain];
							int tchain = mesh->texture_batch_list[*chain];
							int tchain2 = (mesh->texture_batch_list2) ? mesh->texture_batch_list2[*chain] : -1;

							r = base_r;
							g = base_g;
							b = base_b;
							ctx.r = ctx.g = ctx.b = 0;
			
							ctx.P = mesh->object_vertex_list[vchain];

							unsigned char *Cv = NULL;
							if( mesh->vertex_color_list ) {
								Cv = &mesh->vertex_color_list[vchain*3];
							}

							if (*fp & FLAT_SHADED)
							{
								r += (mat->diffuse.r * flat[v].r) >> 8;
								g += (mat->diffuse.g * flat[v].g) >> 8;
								b += (mat->diffuse.b * flat[v].b) >> 8;

								flat_slot = mt_list_index;
								
								if( specular_material ) { 
									ctx.N = mesh->normal_ABC[ mesh->vertex_normal[ vchain ] ];
									ctx.L = (ctx.Lo - ctx.P).normalize();
									ctx.V = (ctx.Vo - ctx.P).normalize();
							
									if( mesh->tc_user_generator( &ctx, mesh->tc_user_generator_context ) && (specular_mode == 2) ) {
										spec_valid[v] = 1;
										spec_good_vertex_slot[*chain] = pbSpecGood.current_vertex;
										spec_flat_slot = pbSpecGood.current_vertex;
										pbSpecGood.TexCoord2f( ctx.u, ctx.v );
										pbSpecGood.Vertex3f( ctx.P.x, ctx.P.y, ctx.P.z );
									}
								}
							}
							else
							{
							//
							// Smooth-shaded. Use vertex light values computed above.
							//
								LightRGB * l = face_normal_flag 
									? back_current_light_pool + vchain
									: current_light_pool + vchain;
								
								if (mat->flags & MF_DIFFUSE)
								{
									r += (mat->diffuse.r * l->r) >> 8;
									g += (mat->diffuse.g * l->g) >> 8;
									b += (mat->diffuse.b * l->b) >> 8;
								}

								if( Cv ) {
									r = (r * *Cv) >> 8;	Cv++;
									g = (g * *Cv) >> 8;	Cv++;
									b = (b * *Cv) >> 8;	Cv++;
								}

								if( specular_material ) { 
									ctx.N = mesh->normal_ABC[ mesh->vertex_normal[ vchain ] ];
									ctx.L = (ctx.Lo - ctx.P).normalize();
									ctx.V = (ctx.Vo - ctx.P).normalize();
									
									if( mesh->tc_user_generator( &ctx, mesh->tc_user_generator_context ) && specular_mode == 2 ) {
										spec_valid[v] = 1;
										spec_good_vertex_slot[*chain] = pbSpecGood.current_vertex;
										pbSpecGood.TexCoord2f( ctx.u, ctx.v );
										pbSpecGood.Vertex3f( ctx.P.x, ctx.P.y, ctx.P.z );
									}
								}

								vertex_slot[*chain] = mt_list_index;
							}

							if( flags & RF_CLAMP_COLOR )
							{
								if( r > 255 ) r = 255;
								if( g > 255 ) g = 255;
								if( b > 255 ) b = 255;
							}
							else
							{
								if(255 < (max_c = Tmax(Tmax(r, g), b)))
								{
									r = (r * 255) / max_c;
									g = (g * 255) / max_c;
									b = (b * 255) / max_c;
								}
							}

							current_mt.r = (U8)r;
							current_mt.g = (U8)g;
							current_mt.b = (U8)b;
							current_mt.a = mat->transparency;

							if (object->animated_txt_vertex_list)
							{
								current_mt.u = object->animated_txt_vertex_list[*chain].u;
								current_mt.v = object->animated_txt_vertex_list[*chain].v;
							}
							else
							{
								current_mt.u = mesh->texture_vertex_list[tchain].u;
								current_mt.v = mesh->texture_vertex_list[tchain].v;

							}
							
							if(tchain2 == -1)
							{
								current_mt.u2 = current_mt.u;
								current_mt.v2 = current_mt.v;
							}
							else
							{
								current_mt.u2 = mesh->texture_vertex_list[tchain2].u;
								current_mt.v2 = mesh->texture_vertex_list[tchain2].v;
							}

							current_mt.scolor = ARGB_MAKE( ctx.r, ctx.g, ctx.b, 0 );
							
							current_mt.pos.x = ctx.P.x;
							current_mt.pos.y = ctx.P.y;
							current_mt.pos.z = ctx.P.z;
							mt_list_index++;

							ASSERT(mt_list_index <= mt_list_len);
						}
						else if( spec_good_vertex_slot[*chain] != 0xFFFF ) {
							spec_valid[v] = 1;
						}

						// Add index to list:

						if (*fp & FLAT_SHADED)
						{
							*idx_ptr++ = flat_slot;
							*spec_good_idx_ptr++ = spec_flat_slot;
						}
						else
						{
							*idx_ptr++ = vertex_slot[*chain];
							*spec_good_idx_ptr++ = spec_good_vertex_slot[*chain];
						}

						ASSERT_FATAL(idx_ptr <= (index_list + vertex_list_len) || !idx_ptr);
					}

					if( specular_material && (specular_mode == 2) && (spec_valid[0] + spec_valid[1] + spec_valid[2]) < 3 ) {
						// cleanup geometry because this face isn't a valid specular triangle.
						
						if( spec_valid[0] ) {
							pbSpecGood.vertex_buffer[ spec_good_idx_ptr[-3] ].color &= 0x00FFFFFF;
						}
						if( spec_valid[1] ) {
							pbSpecGood.vertex_buffer[ spec_good_idx_ptr[-2] ].color &= 0x00FFFFFF;
						}
						if( spec_valid[2] ) {
							pbSpecGood.vertex_buffer[ spec_good_idx_ptr[-1] ].color &= 0x00FFFFFF;
						}
						// remove this triangle from the specular pass
						spec_good_idx_ptr -= 3;
					}
				}
			}

			render_facegroup_with_material( rwm_flags, object, mesh, group, mat, index_list, idx_ptr-index_list, spec_good_index_list, spec_good_idx_ptr-spec_good_index_list, 0, light_map, texxform );
		}
	}

	return result;
}

//

Mesh * POLYMESH::get_archetype_mesh( RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( render_arch_index != INVALID_ARCHETYPE_INDEX ) ;

	rarch_map::iterator rarch;
	
	if( (rarch = render_archetypes.find( render_arch_index )) == render_archetypes.end() ) {
		return NULL;
	}

	return static_cast<Mesh*>( const_cast<XMesh*>( rarch->second ) );
}

//

Mesh * POLYMESH::get_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_ARCHETYPE_INDEX ) ;

	inst_map::iterator inst;
	
	if( (inst = instances.find( inst_index )) == instances.end() ) {
		return NULL;
	}
		
	return static_cast<Mesh*>( const_cast<XMesh*>( inst->second->get_mesh() ) );
}

//

Mesh * POLYMESH::get_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_ARCHETYPE_INDEX ) ;

	inst_map::iterator inst;
	
	if( (inst = instances.find( inst_index )) == instances.end() ) {
		return NULL;
	}
		
	return static_cast<Mesh*>( const_cast<XMesh*>( inst->second->get_unique_mesh() ) );
}

//

GENRESULT COMAPI POLYMESH::release_unique_instance_mesh( INSTANCE_INDEX inst_index, RENDER_ARCHETYPE render_arch_index )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( render_arch_index != INVALID_ARCHETYPE_INDEX ) ;

	inst_map::iterator inst;
	
	if( (inst = instances.find( inst_index )) == instances.end() ) {
		return GR_GENERIC;
	}
		
	return inst->second->release_unique_mesh() ;
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

		if( (server = new DAComponentFactory< DAComponent< POLYMESH >, RendCompDesc >( CLSID_PolyMesh )) == NULL ) {
			return TRUE;
		}

		if( (DACOM = DACOM_Acquire()) != NULL ) {
			DACOM->RegisterComponent( server,  CLSID_PolyMesh, DACOM_NORMAL_PRIORITY );
		}

		server->Release();
	}

   return TRUE;
}



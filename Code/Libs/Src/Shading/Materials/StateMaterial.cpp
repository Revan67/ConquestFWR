// StateMaterial.cpp
//
// This material simply collects common IRP state into a nice handy block
//
// TODO: make for() loops use advancing pointers
// TODO: make state arrays organized like: state = array[s], value = array[num_states+s]	??


#include "Materials.h"


//

const char *CLSID_StateMaterial= "StateMaterial";

//

// StateMaterial
//
// 
//
struct StateMaterial : public IMaterial,
					   public IStateMaterial
{
	BEGIN_DACOM_MAP_INBOUND(StateMaterial)
	DACOM_INTERFACE_ENTRY(IMaterial)
	DACOM_INTERFACE_ENTRY2(IID_IMaterial,IMaterial)
	DACOM_INTERFACE_ENTRY(IStateMaterial)
	DACOM_INTERFACE_ENTRY2(IID_IStateMaterial,IStateMaterial)
	END_DACOM_MAP()

public:		// public interface
    
	// IMaterial
	GENRESULT COMAPI initialize( IDAComponent *system_container ) ;
	GENRESULT COMAPI load_from_filesystem( IFileSystem *IFS ) ;
	GENRESULT COMAPI verify( U32 max_num_passes, float max_detail_level ) ;
	GENRESULT COMAPI update( float dt ) ;
	GENRESULT COMAPI apply( void ) ;
	GENRESULT COMAPI clone( IMaterial **out_Material ) ;
	GENRESULT COMAPI render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) ;
	GENRESULT COMAPI set_name( const char *new_name ) ;
	GENRESULT COMAPI get_name( char *out_name, U32 max_name_len ) ;
	GENRESULT COMAPI get_type( char *out_type, U32 max_type_len ) ;
	GENRESULT COMAPI get_num_passes( U32 *out_num_passes ) ;

	// IStateMaterial
	GENRESULT COMAPI set_render_state( D3DRENDERSTATETYPE state, U32 value ) ;
	GENRESULT COMAPI get_render_state( D3DRENDERSTATETYPE state, U32 *out_value ) ;
	GENRESULT COMAPI set_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 value ) ;
	GENRESULT COMAPI get_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 *out_value ) ;
	GENRESULT COMAPI set_texture_stage_texture( U32 stage, U32 irp_texture_id ) ;
	GENRESULT COMAPI get_texture_stage_texture( U32 stage, U32 *out_irp_texture_id ) ;


	//
	StateMaterial( void );
	~StateMaterial( void );

	GENRESULT init( DACOMDESC *desc );

protected:	// protected data

	GENRESULT cleanup( void );

	IDAComponent			*system_services;
	IRenderPrimitive		*render_primitive;
	IRenderPipeline			*render_pipeline;
	IVertexBufferManager	*vbuffer_manager;

	char					material_name[IM_MAX_NAME_LEN];

	U32						*render_states;				// [2*i + 0] == state enum, [2*i + 1] == value
	U32						num_render_states;

	U32						*texture_stage_states;		// [3*i + 0] == stage, [3*i + 1] == state, [3*i + 2] == value
	U32						num_texture_stage_states;

	U32						*texture_stage_textures;	// [2*i + 0] == stage, [2*i + 1] == texture
	U32						num_texture_stage_textures;
};

//

DECLARE_MATERIAL( StateMaterial, IS_SIMPLE );

//


StateMaterial::StateMaterial( void )
{
	init( NULL );
}

//

StateMaterial::~StateMaterial( void )
{
	cleanup();
}

//

GENRESULT StateMaterial::init( DACOMDESC *desc )
{
	system_services = NULL;
	render_primitive = NULL;
	render_pipeline = NULL;
	vbuffer_manager = NULL;

	material_name[0] = 0;

	render_states = NULL;
	num_render_states = 0;

	texture_stage_states = NULL;
	num_texture_stage_states = 0;

	texture_stage_textures = NULL;
	num_texture_stage_textures = 0;

	return GR_OK;
}

//

GENRESULT StateMaterial::cleanup( void )
{
	system_services = NULL;
	render_primitive = NULL;
	render_pipeline = NULL;
	vbuffer_manager = NULL;

	material_name[0] = 0;

	delete[] render_states;
	render_states = NULL;
	num_render_states = 0;

	delete[] texture_stage_states;
	texture_stage_states = NULL;
	num_texture_stage_states = 0;

	delete[] texture_stage_textures;
	texture_stage_textures = NULL;
	num_texture_stage_textures = 0;

	return GR_OK;
}

//

GENRESULT StateMaterial::initialize( IDAComponent *system_container ) 
{
	cleanup();

	if( system_container == NULL ) {
		return GR_GENERIC;
	}

	material_name[0] = 0;

	system_services = system_container;

	if( FAILED( system_container->QueryInterface( IID_IRenderPrimitive, (void**)&render_primitive ) ) ) {
		GENERAL_TRACE_1( "StateMaterial: intitialize: this material requires IID_IRenderPrimitive\n" );
		return GR_GENERIC;
	}
	render_primitive->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IRenderPipeline, (void**)&render_pipeline ) ) ) {
		GENERAL_TRACE_1( "StateMaterial: intitialize: this material requires IID_IRenderPipeline\n" );
		return GR_GENERIC;
	}
	render_pipeline->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IVertexBufferManager, (void**)&vbuffer_manager ) ) ) {
		GENERAL_TRACE_1( "StateMaterial: intitialize: this material requires IID_IVertexBufferManager\n" );
		return GR_GENERIC;
	}
	vbuffer_manager->Release();	// maintain artificial reference

	//

	return GR_OK;
}

//

GENRESULT StateMaterial::clone( IMaterial **out_material ) 
{
	IMaterial *material;
	StateMaterial *new_material;
	DACOMDESC desc;

	desc.interface_name = CLSID_StateMaterial;

	if( FAILED( DACOM_Acquire()->CreateInstance( &desc, (void**)&material ) ) ) {
		return GR_GENERIC;
	}

	// this is safe assuming no one registers a new component
	// under CLSID_StateMaterial
	//
	new_material = (StateMaterial*)material;

	if( FAILED( new_material->initialize( system_services ) ) ) {
		return GR_GENERIC;
	}

	ASSERT( 0 );

	return GR_OK;
}

//

GENRESULT StateMaterial::load_from_filesystem( IFileSystem *IFS ) 
{
	// Do something here

	return GR_OK;
}

//

GENRESULT StateMaterial::verify( U32 max_num_passes, float max_detail_level ) 
{
	apply();
	return render_primitive->verify_state();
}

//

GENRESULT StateMaterial::update( float dt ) 
{
	return GR_OK;
}

//

GENRESULT StateMaterial::apply( void ) 
{

	if( render_states ) {
		for( U32 s=0; s<num_render_states; s++ ) {
			render_primitive->set_render_state( (D3DRENDERSTATETYPE)render_states[s*2+0], render_states[s*2+1] );
		}
	}

	if( texture_stage_states ) {
		for( U32 s=0; s<num_texture_stage_states; s++ ) {
			render_primitive->set_texture_stage_state( texture_stage_states[s*3+0], (D3DTEXTURESTAGESTATETYPE)texture_stage_states[s*3+1], texture_stage_states[s*3+2] );
		}
	}
	
	if( texture_stage_textures ) {
		for( U32 s=0; s<num_texture_stage_textures; s++ ) {
			render_primitive->set_texture_stage_texture( texture_stage_textures[s*2+0], texture_stage_textures[s*2+1] );
		}
	}

	return GR_OK;
}

//

GENRESULT StateMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
{
	IRP_VERTEXBUFFERHANDLE vb;
	void *vbmem;
	U32 vertex_format;
	U32 num_verts;

	if( context->vertex_buffer != IRP_INVALID_VB_HANDLE ) {
		return render_primitive->draw_indexed_primitive_vb( PrimitiveType, context->vertex_buffer, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );
	}

	if( SUCCEEDED( vbuffer_manager->acquire_vertex_buffer( Vertices->vertex_format, 
														   Vertices->num_vertices, 
														   0, 
														   DDLOCK_DISCARDCONTENTS|DDLOCK_WRITEONLY,
														   0,
														   &vb,
														   &vbmem,
														   &vertex_format,
														   &num_verts ) ) ) {
		
		vbuffer_manager->copy_vertex_data( vbmem, vertex_format, Vertices );
		
		render_pipeline->unlock_vertex_buffer( vb );

		render_primitive->draw_indexed_primitive_vb( PrimitiveType, vb, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );
		
		vbuffer_manager->release_vertex_buffer( vb );
	}

	return GR_OK;
}

//

GENRESULT StateMaterial::set_name( const char *new_name ) 
{
	strcpy( material_name, new_name );
	return GR_OK;
}

//

GENRESULT StateMaterial::get_name( char *out_name, U32 max_name_len ) 
{
	strcpy( out_name, material_name );
	return GR_OK;
}

//

GENRESULT StateMaterial::get_num_passes( U32 *out_num_passes ) 
{
	*out_num_passes = 1;
	return GR_OK;
}

//

GENRESULT StateMaterial::get_type( char *out_type, U32 max_type_len )
{
	strncpy( out_type, CLSID_StateMaterial, max_type_len );
	out_type[max_type_len-1] = 0;
	return GR_OK;
}

//

GENRESULT StateMaterial::set_render_state( D3DRENDERSTATETYPE state, U32 value ) 
{
	for( U32 s=0; s<num_render_states; s++ ) {
		if( render_states[s*2+0] == (U32)state ) {
			render_states[s*2+1] = value;
			return GR_OK;
		}
	}

	render_states = (U32*)realloc( render_states, (num_render_states+1)*2*sizeof(U32) );
	ASSERT( render_states );

	render_states[num_render_states*2 + 0] = (U32)state;
	render_states[num_render_states*2 + 1] = (U32)value;

	num_render_states++;

	return GR_OK;
}

//

GENRESULT StateMaterial::get_render_state( D3DRENDERSTATETYPE state, U32 *out_value ) 
{
	for( U32 s=0; s<num_render_states; s++ ) {
		if( render_states[s*2+0] == (U32)state ) {
			*out_value = render_states[s*2+1];
			return GR_OK;
		}
	}

	return GR_GENERIC;
}

//

GENRESULT StateMaterial::set_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 value ) 
{
	for( U32 s=0; s<num_texture_stage_states; s++ ) {
		if( texture_stage_states[s*3+0] == stage && texture_stage_states[s*3+1] == (U32)state ) {
			texture_stage_states[s*3+2] = value;
			return GR_OK;
		}
	}

	texture_stage_states = (U32*)realloc( texture_stage_states, (num_texture_stage_states+1)*3*sizeof(U32) );
	ASSERT( texture_stage_states );

	texture_stage_states[num_texture_stage_states*3 + 0] = (U32)stage;
	texture_stage_states[num_texture_stage_states*3 + 1] = (U32)state;
	texture_stage_states[num_texture_stage_states*3 + 2] = (U32)value;

	num_texture_stage_states++;

	return GR_OK;
}

//

GENRESULT StateMaterial::get_texture_stage_state( U32 stage, D3DTEXTURESTAGESTATETYPE state, U32 *out_value ) 
{
	for( U32 s=0; s<num_texture_stage_states; s++ ) {
		if( texture_stage_states[s*3+0] == stage && texture_stage_states[s*3+1] == (U32)state ) {
			*out_value = texture_stage_states[s*3+2];
			return GR_OK;
		}
	}

	return GR_GENERIC;
}

//

GENRESULT StateMaterial::set_texture_stage_texture( U32 stage, U32 irp_texture_id ) 
{
	for( U32 s=0; s<num_texture_stage_textures; s++ ) {
		if( texture_stage_textures[s*2+0] == (U32)stage ) {
			texture_stage_textures[s*2+1] = irp_texture_id;
			return GR_OK;
		}
	}

	texture_stage_textures = (U32*)realloc( texture_stage_textures, (num_texture_stage_textures+1)*2*sizeof(U32) );
	ASSERT( texture_stage_textures );

	texture_stage_textures[num_texture_stage_textures*2 + 0] = (U32)stage;
	texture_stage_textures[num_texture_stage_textures*2 + 1] = (U32)irp_texture_id;

	num_texture_stage_textures++;

	return GR_OK;
}

//

GENRESULT StateMaterial::get_texture_stage_texture( U32 stage, U32 *out_irp_texture_id ) 
{
	for( U32 s=0; s<num_texture_stage_textures; s++ ) {
		if( texture_stage_textures[s*2+0] == (U32)stage ) {
			*out_irp_texture_id = texture_stage_textures[s*2+1];
			return GR_OK;
		}
	}

	return GR_GENERIC;
}

//


// EOF

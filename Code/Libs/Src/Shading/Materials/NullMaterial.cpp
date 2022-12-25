// NullMaterial.cpp
//
// This material that does nothing except handle render calls.
//

#include "Materials.h"


//

const char *CLSID_NullMaterial= "NullMaterial";

//

// NullMaterial
//
// 
//
struct NullMaterial : public IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(NullMaterial)
	DACOM_INTERFACE_ENTRY(IMaterial)
	DACOM_INTERFACE_ENTRY2(IID_IMaterial,IMaterial)
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

	//
	NullMaterial( void );
	~NullMaterial( void );

	GENRESULT init( DACOMDESC *desc );

protected:	// protected data

	GENRESULT cleanup( void );

	IDAComponent			*system_services;
	IRenderPrimitive		*render_primitive;
	IRenderPipeline			*render_pipeline;
	IVertexBufferManager	*vbuffer_manager;

	char					material_name[IM_MAX_NAME_LEN];
};

//

DECLARE_MATERIAL( NullMaterial, IS_SIMPLE );

//


NullMaterial::NullMaterial( void )
{
	init( NULL );
}

//

NullMaterial::~NullMaterial( void )
{
	cleanup();
}

//

GENRESULT NullMaterial::init( DACOMDESC *desc )
{
	system_services = NULL;
	render_primitive = NULL;
	render_pipeline = NULL;
	vbuffer_manager = NULL;

	material_name[0] = 0;

	return GR_OK;
}

//

GENRESULT NullMaterial::cleanup( void )
{
	system_services = NULL;
	render_primitive = NULL;
	render_pipeline = NULL;
	vbuffer_manager = NULL;

	material_name[0] = 0;

	return GR_OK;
}

//

GENRESULT NullMaterial::initialize( IDAComponent *system_container ) 
{
	cleanup();

	if( system_container == NULL ) {
		return GR_GENERIC;
	}

	material_name[0] = 0;

	system_services = system_container;

	if( FAILED( system_container->QueryInterface( IID_IRenderPrimitive, (void**)&render_primitive ) ) ) {
		GENERAL_TRACE_1( "NullMaterial: intitialize: this material requires IID_IRenderPrimitive\n" );
		return GR_GENERIC;
	}
	render_primitive->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IRenderPipeline, (void**)&render_pipeline ) ) ) {
		GENERAL_TRACE_1( "NullMaterial: intitialize: this material requires IID_IRenderPipeline\n" );
		return GR_GENERIC;
	}
	render_pipeline->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IVertexBufferManager, (void**)&vbuffer_manager ) ) ) {
		GENERAL_TRACE_1( "NullMaterial: intitialize: this material requires IID_IVertexBufferManager\n" );
		return GR_GENERIC;
	}
	vbuffer_manager->Release();	// maintain artificial reference

	//

	return GR_OK;
}

//

GENRESULT NullMaterial::clone( IMaterial **out_material ) 
{
	IMaterial *material;
	DACOMDESC desc;

	desc.interface_name = CLSID_NullMaterial;

	if( FAILED( DACOM_Acquire()->CreateInstance( &desc, (void**)&material ) ) ) {
		return GR_GENERIC;
	}

	if( FAILED( material->initialize( system_services ) ) ) {
		return GR_GENERIC;
	}

	return GR_OK;
}

//

GENRESULT NullMaterial::load_from_filesystem( IFileSystem *IFS ) 
{
	// Do something here

	return GR_OK;
}

//

GENRESULT NullMaterial::verify( U32 max_num_passes, float max_detail_level ) 
{
	apply();
	return render_primitive->verify_state();
}

//

GENRESULT NullMaterial::update( float dt ) 
{
	return GR_OK;
}

//

GENRESULT NullMaterial::apply( void ) 
{
	return GR_OK;
}

//

GENRESULT NullMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
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

GENRESULT NullMaterial::set_name( const char *new_name ) 
{
	strcpy( material_name, new_name );
	return GR_OK;
}

//

GENRESULT NullMaterial::get_name( char *out_name, U32 max_name_len ) 
{
	strcpy( out_name, material_name );
	return GR_OK;
}

//

GENRESULT NullMaterial::get_num_passes( U32 *out_num_passes ) 
{
	*out_num_passes = 1;
	return GR_OK;
}

//

GENRESULT NullMaterial::get_type( char *out_type, U32 max_type_len )
{
	strncpy( out_type, CLSID_NullMaterial, max_type_len );
	out_type[max_type_len-1] = 0;
	return GR_OK;
}

// EOF

// DebugMaterial.cpp
//
// A brain dead debugging material.
//
// TODO: define and **use** IMaterial protocol (initialize, verify, apply, etc...)
// TODO: add verify() to material manager to verify all loaded materials.
// TODO: remove save_to_filesystem()

#include "Materials.h"
#include "RPUL.h"
//

#define CLSID_DebugMaterial "DebugMaterial"

//

// Define the normals for the cube
D3DVECTOR n0( 0.0f, 0.0f,-1.0f ); // Front face
D3DVECTOR n1( 0.0f, 0.0f, 1.0f ); // Back face
D3DVECTOR n2( 0.0f,-1.0f, 0.0f ); // Top face
D3DVECTOR n3( 0.0f, 1.0f, 0.0f ); // Bottom face
D3DVECTOR n4(-1.0f, 0.0f, 0.0f ); // Right face
D3DVECTOR n5( 1.0f, 0.0f, 0.0f ); // Left face

//

// DebugMaterial
//
// 
//
struct DebugMaterial :	IMaterial
{
	BEGIN_DACOM_MAP_INBOUND(DebugMaterial)
	DACOM_INTERFACE_ENTRY(IMaterial)
	DACOM_INTERFACE_ENTRY2(IID_IMaterial,IMaterial)
	END_DACOM_MAP()

public:		// public interface
    

	// IMaterial
	GENRESULT COMAPI initialize( IDAComponent *system_container ) ;
	GENRESULT COMAPI cleanup( void ) ;
	GENRESULT COMAPI set_name( const char *new_name ) ;
	GENRESULT COMAPI get_name( char *out_name, U32 max_name_len ) ;
	GENRESULT COMAPI load_from_filesystem( IFileSystem *IFS ) ;
	GENRESULT COMAPI clone( IMaterial **out_Material ) ;
	GENRESULT COMAPI verify( U32 max_num_passes, float max_detail_level ) ;
	GENRESULT COMAPI update( float dt ) ;
	GENRESULT COMAPI apply( void ) ;
	GENRESULT COMAPI render( MaterialContext *, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) ;
	GENRESULT COMAPI render_vertex_buffer( MaterialContext *, D3DPRIMITIVETYPE PrimitiveType, IRP_VERTEXBUFFERHANDLE VertexBuffer, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) ;
	GENRESULT COMAPI get_num_passes( U32 *out_num_passes ) ;
	GENRESULT COMAPI get_type( char *out_type, U32 max_type_len );
	
	//
 
	DebugMaterial(void);
	~DebugMaterial(void);
	DA_HEAP_DEFINE_NEW_OPERATOR(DebugMaterial);
	GENRESULT init( DACOMDESC *desc );

protected:	// protected interface

protected:	// protected data
	IDAComponent			*system_services;
	ITextureLibrary			*texture_library;
	IRenderPrimitive		*render_primitive;
	IVertexBufferManager	*vbuffer_manager;
	IRenderPipeline			*render_pipeline;

	char				 material_name[MAX_PATH];

	char				 texture_name[MAX_PATH];
	ITL_TEXTURE_REF_ID	 texture_ref_id;
	ITL_TEXTUREFRAME_IRP texture_frame;
	U32					 texture_flags;
	D3DMATERIAL7		 d3d_material;

	//

#define NUM_CUBE_VERTICES (4*6)
#define NUM_CUBE_INDICES  (6*6)

	//
	
	D3DVERTEX	m_pCubeVertices[NUM_CUBE_VERTICES]; // Vertices for the cube
	U32			m_pCubeVertexIndices[NUM_CUBE_INDICES];   // Indices for the vertices of the cube
	WORD		m_pCubeIndices[NUM_CUBE_INDICES];   // Indices for the cube
};

// 

DECLARE_MATERIAL( DebugMaterial, IS_SIMPLE );

//

DebugMaterial::DebugMaterial(void)
{
	init( NULL );
}

//

DebugMaterial::~DebugMaterial(void)
{
	cleanup();
}

//

GENRESULT DebugMaterial::init( DACOMDESC *desc )
{
	render_primitive = NULL;
	system_services = NULL;
	texture_library = NULL;
	vbuffer_manager = NULL;


	return GR_OK;
}

//

GENRESULT DebugMaterial::initialize( IDAComponent *system_container ) 
{
	if( system_container == NULL ) {
		return GR_GENERIC;
	}

	system_services = system_container;

	if( FAILED( system_container->QueryInterface( IID_ITextureLibrary, (void**)&texture_library ) ) ) {
		GENERAL_TRACE_1( "DebugMaterial: intitialize: this material requires IID_ITextureLibrary\n" );
		return GR_GENERIC;
	}
	texture_library->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IRenderPrimitive, (void**)&render_primitive ) ) ) {
		GENERAL_TRACE_1( "DebugMaterial: intitialize: this material requires IID_IRenderPrimitive\n" );
		return GR_GENERIC;
	}
	render_primitive->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IRenderPipeline, (void**)&render_pipeline ) ) ) {
		GENERAL_TRACE_1( "DebugMaterial: intitialize: this material requires IID_IRenderPipeline\n" );
		return GR_GENERIC;
	}
	render_pipeline->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IVertexBufferManager, (void**)&vbuffer_manager ) ) ) {
		GENERAL_TRACE_1( "DebugMaterial: intitialize: this material requires IID_IVertexBufferManager\n" );
		return GR_GENERIC;
	}
	vbuffer_manager->Release();	// maintain artificial reference

	material_name[0] = 0;

	texture_name[0] = 0;
	texture_ref_id = ITL_INVALID_REF_ID;
	texture_flags = 0;

	d3d_material.ambient.r = 1.0f;
	d3d_material.ambient.g = 1.0f;
	d3d_material.ambient.b = 1.0f;
	d3d_material.ambient.a = 1.0f;

	d3d_material.diffuse.r = 1.0f;
	d3d_material.diffuse.g = 1.0f;
	d3d_material.diffuse.b = 1.0f;
	d3d_material.diffuse.a = 1.0f;

	d3d_material.emissive.r = 0.0f;
	d3d_material.emissive.g = 0.0f;
	d3d_material.emissive.b = 0.0f;
	d3d_material.emissive.a = 0.0f;

	d3d_material.specular.r = 0.0f;
	d3d_material.specular.g = 0.0f;
	d3d_material.specular.b = 0.0f;
	d3d_material.specular.a = 0.0f;
	
	d3d_material.power = 0.0f;

	// Create a debugging cube
	//

	WORD* pIndices = m_pCubeIndices;

	// Set up the indices for the cube
	*pIndices++ =  0+0;   *pIndices++ =  0+1;   *pIndices++ =  0+2;
	*pIndices++ =  0+2;   *pIndices++ =  0+3;   *pIndices++ =  0+0;
	*pIndices++ =  4+0;   *pIndices++ =  4+1;   *pIndices++ =  4+2;
	*pIndices++ =  4+2;   *pIndices++ =  4+3;   *pIndices++ =  4+0;
	*pIndices++ =  8+0;   *pIndices++ =  8+1;   *pIndices++ =  8+2;
	*pIndices++ =  8+2;   *pIndices++ =  8+3;   *pIndices++ =  8+0;
	*pIndices++ = 12+0;   *pIndices++ = 12+1;   *pIndices++ = 12+2;
	*pIndices++ = 12+2;   *pIndices++ = 12+3;   *pIndices++ = 12+0;
	*pIndices++ = 16+0;   *pIndices++ = 16+1;   *pIndices++ = 16+2;
	*pIndices++ = 16+2;   *pIndices++ = 16+3;   *pIndices++ = 16+0;
	*pIndices++ = 20+0;   *pIndices++ = 20+1;   *pIndices++ = 20+2;
	*pIndices++ = 20+2;   *pIndices++ = 20+3;   *pIndices++ = 20+0;

	//

	D3DVERTEX *pVertices = m_pCubeVertices;

	// Front face
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f, 1.0f,-1.0f), n0, 0.01f, 0.99f );
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f, 1.0f,-1.0f), n0, 0.99f, 0.99f );
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f,-1.0f,-1.0f), n0, 0.99f, 0.01f );
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f,-1.0f,-1.0f), n0, 0.01f, 0.01f );

	// Back face
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f, 1.0f, 1.0f), n1, 0.99f, 0.99f );
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f,-1.0f, 1.0f), n1, 0.99f, 0.01f );
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f,-1.0f, 1.0f), n1, 0.01f, 0.01f );
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f, 1.0f, 1.0f), n1, 0.01f, 0.99f );

	// Top face
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f, 1.0f, 1.0f), n2, 0.01f, 0.99f );
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f, 1.0f, 1.0f), n2, 0.99f, 0.99f );
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f, 1.0f,-1.0f), n2, 0.99f, 0.01f );
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f, 1.0f,-1.0f), n2, 0.01f, 0.01f );

	// Bottom face
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f,-1.0f, 1.0f), n3, 0.01f, 0.99f );
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f,-1.0f,-1.0f), n3, 0.01f, 0.01f );
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f,-1.0f,-1.0f), n3, 0.99f, 0.01f );
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f,-1.0f, 1.0f), n3, 0.99f, 0.99f );

	// Right face
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f, 1.0f,-1.0f), n4, 0.01f, 0.99f );
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f, 1.0f, 1.0f), n4, 0.99f, 0.99f );
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f,-1.0f, 1.0f), n4, 0.99f, 0.01f );
	*pVertices++ = D3DVERTEX( D3DVECTOR( 1.0f,-1.0f,-1.0f), n4, 0.01f, 0.01f );

	// Left face
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f, 1.0f,-1.0f), n5, 0.99f, 0.99f );
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f,-1.0f,-1.0f), n5, 0.99f, 0.01f );
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f,-1.0f, 1.0f), n5, 0.01f, 0.01f );
	*pVertices++ = D3DVERTEX( D3DVECTOR(-1.0f, 1.0f, 1.0f), n5, 0.01f, 0.99f );

	for( U32 i=0; i<NUM_CUBE_VERTICES; i++ ) {
		m_pCubeVertexIndices[i] = i;
	}

	return GR_OK;
}

//

GENRESULT DebugMaterial::cleanup( void ) 
{
	if( texture_ref_id != ITL_INVALID_REF_ID ) {
		texture_library->release_texture_ref( texture_ref_id );
		texture_ref_id = ITL_INVALID_REF_ID;
	}

	return GR_OK;
}

//

GENRESULT DebugMaterial::set_name( const char *new_name ) 
{
	strcpy( material_name, new_name );
	return GR_OK;
}

//

GENRESULT DebugMaterial::get_name( char *out_name, U32 max_name_len ) 
{
	strcpy( out_name, material_name );
	return GR_OK;
}

//

GENRESULT DebugMaterial::load_from_filesystem( IFileSystem *IFS ) 
{
	if( IFS == NULL ) {
		return GR_GENERIC;
	}

	Vector color;

	if( SUCCEEDED( read_type<Vector>( IFS, "C0", &color ) ) ) {
		d3d_material.diffuse.r = color.x;
		d3d_material.diffuse.g = color.y;
		d3d_material.diffuse.b = color.z;

		d3d_material.ambient.r = color.x;
		d3d_material.ambient.g = color.y;
		d3d_material.ambient.b = color.z;
	}

	if( SUCCEEDED( read_type<float>( IFS, "C1", &color.x ) ) ) {
		d3d_material.diffuse.a = color.x;
	}

	if( FAILED( read_string( IFS, "T0_name", MAX_PATH, texture_name ) ) ) {
		texture_name[0] = 0;
	}

	if( FAILED( read_type<U32>( IFS, "T0_flags", &texture_flags ) ) ) {
		texture_flags = 0;
	}

	return GR_OK;
}

//

GENRESULT DebugMaterial::clone( IMaterial **out_material ) 
{
	IMaterial *material;
	DebugMaterial *new_material;
	DACOMDESC desc;

	desc.interface_name = CLSID_DebugMaterial;

	if( FAILED( DACOM_Acquire()->CreateInstance( &desc, (void**)&material ) ) ) {
		return GR_GENERIC;
	}

	// this is safe assuming no one registers a new component
	// under CLSID_DebugMaterial
	//
	new_material = (DebugMaterial*)material;

	if( FAILED( new_material->initialize( system_services ) ) ) {
		return GR_GENERIC;
	}

	// copy our state
	// DO NOT copy the texture ref id.  the clone will pick up
	// a new reference when it needs one.
	//
	strcpy( new_material->texture_name, texture_name );
	new_material->texture_ref_id = ITL_INVALID_REF_ID;
	
	memcpy( &new_material->d3d_material, &d3d_material, sizeof(d3d_material) );

	return GR_OK;
}

//

GENRESULT DebugMaterial::verify( U32 max_num_passes, float max_detail_level ) 
{
	ITL_TEXTURE_ID tid;
	ITL_TEXTURE_REF_ID trid;

	trid = ITL_INVALID_REF_ID;

	if( texture_name[0] != 0 ) {
		if( SUCCEEDED( texture_library->get_texture_id( texture_name, &tid ) ) ) {
			texture_library->add_ref_texture_id( tid, &trid );
			texture_library->release_texture_id( tid );
		}
	}

	if( texture_ref_id == ITL_INVALID_REF_ID ) {
		texture_library->release_texture_ref( texture_ref_id );
		texture_ref_id = ITL_INVALID_REF_ID;
	}

	texture_ref_id = trid;

//	apply();

	render_primitive->verify_state();

	return GR_OK;
}

//

GENRESULT DebugMaterial::update( float dt ) 
{
	texture_library->get_texture_ref_frame( texture_ref_id, ITL_FRAME_CURRENT, &texture_frame );
	return GR_OK;
}

//

GENRESULT DebugMaterial::apply( void ) 
{
//	render_primitive->set_render_state( D3DRS_ZENABLE, TRUE );
//	render_primitive->set_render_state( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
//	render_primitive->set_render_state( D3DRS_CULLMODE, D3DCULL_CW );
	render_primitive->set_render_state( D3DRS_LIGHTING, TRUE );

	if( d3d_material.diffuse.a < 0.999f ) {
		render_primitive->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
		render_primitive->set_render_state( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		render_primitive->set_render_state( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	}

	render_primitive->set_material( &d3d_material );

	render_primitive->set_texture_stage_texture( 0, texture_frame.rp_texture_id );
	
	render_primitive->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	render_primitive->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	render_primitive->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_TEXTURE );

	render_primitive->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
	render_primitive->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
	render_primitive->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE );

	render_primitive->set_texture_stage_texture( 1, 0 );
	render_primitive->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
	render_primitive->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

	return GR_OK;
}

//

GENRESULT DebugMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
{
	IRP_VERTEXBUFFERHANDLE vb;
	void *vbmem;
	U32 vertex_format;
	U32 num_verts;

#if 0
	// render a little axis
	PrimitiveBuilder pb( render_primitive );
	pb.Begin( PB_LINES );
	pb.Color3f( 1,0,0 );	pb.Vertex3f( 0,0,0 );	pb.Vertex3f( 10,0,0 );
	pb.Color3f( 0,1,0 );	pb.Vertex3f( 0,0,0 );	pb.Vertex3f( 0,10,0 );
	pb.Color3f( 0,0,1 );	pb.Vertex3f( 0,0,0 );	pb.Vertex3f( 0,0,10 );
	pb.End();
#endif

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

GENRESULT DebugMaterial::get_num_passes( U32 *out_num_passes ) 
{
	*out_num_passes = 1;

	return GR_OK;
}

//

GENRESULT DebugMaterial::get_type( char *out_type, U32 max_type_len )
{
	strcpy( out_type, "DebugMaterial" );
	return GR_OK;
}

// EOF







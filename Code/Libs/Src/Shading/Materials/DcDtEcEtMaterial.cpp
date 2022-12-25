#include "Materials.h"


//

const char *CLSID_DcDtEcEtMaterial	= "DcDtEcEt";

//

struct	DcDtEcEtMaterial : public IMaterial,
						public IMaterialProperties
{
	BEGIN_DACOM_MAP_INBOUND(DcDtEcEtMaterial)
	DACOM_INTERFACE_ENTRY(IMaterial)
	DACOM_INTERFACE_ENTRY2(IID_IMaterial,IMaterial)
	DACOM_INTERFACE_ENTRY(IMaterialProperties)
	DACOM_INTERFACE_ENTRY2(IID_IMaterialProperties,IMaterialProperties)
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

	// IMaterialProperties
	GENRESULT COMAPI get_num_textures( U32 *out_num_textures ) ;
	GENRESULT COMAPI set_texture( U32 texture_num, ITL_TEXTURE_REF_ID trid ) ;
	GENRESULT COMAPI get_texture( U32 texture_num, ITL_TEXTURE_REF_ID *out_trid ) ;
	GENRESULT COMAPI set_texture_addr_mode( U32 texture_num, TC_COORD which_uv, TC_ADDRMODE mode ) ;
	GENRESULT COMAPI get_texture_addr_mode( U32 texture_num, TC_COORD which_uv, TC_ADDRMODE *out_mode ) ;
	GENRESULT COMAPI set_texture_wrap_mode( U32 texture_num, TC_WRAPMODE mode ) ;
	GENRESULT COMAPI get_texture_wrap_mode( U32 texture_num, TC_WRAPMODE *out_mode ) ;
	GENRESULT COMAPI get_num_constants( U32 *out_num_constants ) ;
	GENRESULT COMAPI set_constant( U32 constant_num, U32 num_values, float *values ) ;
	GENRESULT COMAPI get_constant( U32 constant_num, U32 max_num_values, float *out_values ) ;
	GENRESULT COMAPI get_constant_length( U32 constant_num, U32 *out_num_values ) ;

	DcDtEcEtMaterial();
	~DcDtEcEtMaterial();

	GENRESULT init( DACOMDESC *desc );

protected:	// protected interface

	GENRESULT cleanup( void );

protected:	// protected data

	IDAComponent			*system_services;
	ITextureLibrary			*texture_library;
	IRenderPipeline			*render_pipeline;
	IVertexBufferManager	*vbuffer_manager;


	char					material_name[IM_MAX_NAME_LEN];

	char					texture_name[2][MAX_PATH];
	ITL_TEXTURE_REF_ID		texture_ref_id[2];
	ITL_TEXTUREFRAME_IRP	texture_frame[2];
	U32						texture_flags[2];
	D3DMATERIAL7			d3d_material;

	//
};

//

DECLARE_MATERIAL( DcDtEcEtMaterial, IS_SIMPLE );

//

DcDtEcEtMaterial::DcDtEcEtMaterial(void)
{
	init( NULL );
}

//

DcDtEcEtMaterial::~DcDtEcEtMaterial(void)
{
	cleanup();
}

//

GENRESULT DcDtEcEtMaterial::init( DACOMDESC *desc )
{
	system_services = NULL;
	render_pipeline = NULL;
	texture_library = NULL;
	vbuffer_manager = NULL;

	texture_ref_id[0] = ITL_INVALID_REF_ID;
	texture_ref_id[1] = ITL_INVALID_REF_ID;
	material_name[0] = 0;

	return GR_OK;
}

//

GENRESULT	DcDtEcEtMaterial::cleanup(void) 
{
	int	i;

	for(i=0;i < 2;i++)
	{
		if(texture_ref_id[i] != ITL_INVALID_REF_ID)
		{
			texture_library->release_texture_ref(texture_ref_id[i]);
			texture_ref_id[i]	=ITL_INVALID_REF_ID;
		}
	}

	texture_library		=NULL;
	render_pipeline		=NULL;
	vbuffer_manager		=NULL;
	system_services		=NULL;
	material_name[0]	=0;

	return	GR_OK;
}
	
//

GENRESULT DcDtEcEtMaterial::initialize( IDAComponent *system_container ) 
{
	int	i;

	cleanup();

	if( system_container == NULL ) {
		return GR_GENERIC;
	}

	system_services = system_container;

	if( FAILED( system_container->QueryInterface( IID_ITextureLibrary, (void**)&texture_library ) ) ) {
		GENERAL_TRACE_1( "DcDtEcEtMaterial: intitialize: this material requires IID_ITextureLibrary\n" );
		return GR_GENERIC;
	}
	texture_library->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IRenderPipeline, (void**)&render_pipeline ) ) ) {
		GENERAL_TRACE_1( "DcDtEcEtMaterial: intitialize: this material requires IID_IRenderPipeline\n" );
		return GR_GENERIC;
	}
	render_pipeline->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IVertexBufferManager, (void**)&vbuffer_manager ) ) ) {
		GENERAL_TRACE_1( "DcDtEcEtMaterial: intitialize: this material requires IID_IVertexBufferManager\n" );
		return GR_GENERIC;
	}
	vbuffer_manager->Release();	// maintain artificial reference

	//

	for(i=0;i < 2;i++)
	{
		texture_name[i][0]	=0;
		texture_ref_id[i]	=ITL_INVALID_REF_ID;
		texture_flags[i]	=0;
	}

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

	return GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::clone( IMaterial **out_material ) 
{
	IMaterial		*material;
	DcDtEcEtMaterial	*new_material;
	DACOMDESC		desc;
	int				i;


	desc.interface_name = CLSID_DcDtEcEtMaterial;

	if( FAILED( DACOM_Acquire()->CreateInstance( &desc, (void**)&material ) ) ) {
		return GR_GENERIC;
	}

	// this is safe assuming no one registers a new component
	// under CLSID_DcDtEcEtMaterial
	//
	new_material = (DcDtEcEtMaterial*)material;

	if( FAILED( new_material->initialize( system_services ) ) ) {
		return GR_GENERIC;
	}

	// copy our state
	// DO NOT copy the texture ref id.  the clone will pick up
	// a new reference when it needs one.
	//
	for(i=0;i < 2;i++)
	{
		strcpy(new_material->texture_name[i], texture_name[i]);
		new_material->texture_ref_id[i]	=ITL_INVALID_REF_ID;
	}
	
	memcpy( &new_material->d3d_material, &d3d_material, sizeof(d3d_material) );

	return GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::load_from_filesystem( IFileSystem *IFS ) 
{
	Vector color;

	// read constants
	//
	if( SUCCEEDED( read_type<Vector>( IFS, "C0", &color ) ) ) {
		d3d_material.diffuse.r = color.x;
		d3d_material.diffuse.g = color.y;
		d3d_material.diffuse.b = color.z;

		d3d_material.ambient.r = color.x;
		d3d_material.ambient.g = color.y;
		d3d_material.ambient.b = color.z;
	}

	if( SUCCEEDED( read_type<Vector>( IFS, "C1", &color ) ) ) {
		d3d_material.emissive.r = color.x;
		d3d_material.emissive.g = color.y;
		d3d_material.emissive.b = color.z;
	}


	if( FAILED( read_string( IFS, "T0_name", MAX_PATH, texture_name[0] ) ) ) {
		texture_name[0][0] = 0;
	}

	if( FAILED( read_type<U32>( IFS, "T0_flags", &texture_flags[0] ) ) ) {
		texture_flags[0] = 0;
	}

	if( FAILED( read_string( IFS, "T1_name", MAX_PATH, texture_name[1] ) ) ) {
		texture_name[1][0] = 0;
	}

	if( FAILED( read_type<U32>( IFS, "T1_flags", &texture_flags[1] ) ) ) {
		texture_flags[1] = 0;
	}

	return GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::verify( U32 max_num_passes, float max_detail_level ) 
{
	ITL_TEXTURE_ID		tid;
	ITL_TEXTURE_REF_ID	trid;
	int					i;


	trid	=ITL_INVALID_REF_ID;

	for(i=0;i < 2;i++)
	{
		if(texture_name[i][0])
		{
			if(SUCCEEDED(texture_library->get_texture_id(texture_name[i], &tid)))
			{
				texture_library->add_ref_texture_id(tid, &trid);
				texture_library->release_texture_id(tid);
			}
		}

		if(texture_ref_id[i] != ITL_INVALID_REF_ID)
		{
			texture_library->release_texture_ref(texture_ref_id[i]);
			texture_ref_id[i]	=ITL_INVALID_REF_ID;
		}

		texture_ref_id[i]	=trid;
	}

	return	GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::update( float dt ) 
{
	int	i;


	for(i=0;i < 2;i++)
	{
		if(texture_ref_id[i] != ITL_INVALID_REF_ID)
		{
			texture_library->get_texture_ref_frame(texture_ref_id[i], ITL_FRAME_CURRENT, &texture_frame[i]);
		}
	}
	
	return	GR_OK;
}

//

GENRESULT	DcDtEcEtMaterial::apply(void) 
{
	render_pipeline->set_material(&d3d_material);

	render_pipeline->set_render_state(D3DRS_LIGHTING, TRUE);
	render_pipeline->set_render_state(D3DRS_SPECULARENABLE, FALSE);

	render_pipeline->set_render_state(D3DRS_ALPHABLENDENABLE, TRUE);
	render_pipeline->set_render_state(D3DRS_SRCBLEND, D3DBLEND_ONE);
	render_pipeline->set_render_state(D3DRS_DESTBLEND, D3DBLEND_ONE);

	render_pipeline->set_texture_stage_texture(0, texture_frame[0].rp_texture_id);
	
	render_pipeline->set_texture_stage_texture(0, 0);
	render_pipeline->set_texture_stage_state(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
	render_pipeline->set_texture_stage_state(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	return	GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
{
	IRP_VERTEXBUFFERHANDLE vb;
	void *vbmem;
	U32 vertex_format;
	U32 num_verts;

	if( context->vertex_buffer != IRP_INVALID_VB_HANDLE ) {
		return render_pipeline->draw_indexed_primitive_vb( PrimitiveType, context->vertex_buffer, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );
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

		render_pipeline->draw_indexed_primitive_vb( PrimitiveType, vb, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );
		
		vbuffer_manager->release_vertex_buffer( vb );
	}

	return GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::set_name( const char *new_name ) 
{
	strcpy( material_name, new_name );
	return GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::get_name( char *out_name, U32 max_name_len ) 
{
	strcpy( out_name, material_name );
	return GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::get_num_passes( U32 *out_num_passes ) 
{
	*out_num_passes = 1;
	return GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::get_type( char *out_type, U32 max_type_len )
{
	strncpy( out_type, CLSID_DcDtEcEtMaterial, max_type_len );
	
	out_type[max_type_len-1] = 0;

	return GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::get_num_textures( U32 *out_num_textures ) 
{
	*out_num_textures = 2;
	return GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::get_texture( U32 texture_num, ITL_TEXTURE_REF_ID *out_trid ) 
{
	if(texture_num < 2)
	{
		*out_trid	=texture_ref_id[texture_num];
		texture_library->add_ref_texture_ref(texture_ref_id[texture_num]);
		return	GR_OK;
	}
	else
	{
		*out_trid	= ITL_INVALID_REF_ID;
		return	GR_GENERIC;
	}
}

//

GENRESULT DcDtEcEtMaterial::set_texture( U32 texture_num, ITL_TEXTURE_REF_ID trid ) 
{
	ITL_TEXTURE_ID	tid;

	if(texture_num > 1)
	{
		return	GR_GENERIC;
	}

	texture_library->release_texture_ref(texture_ref_id[texture_num]);
	
	texture_ref_id[texture_num]	=trid;
	
	if(texture_ref_id[texture_num] != ITL_INVALID_REF_ID)
	{			
		texture_library->add_ref_texture_ref(texture_ref_id[texture_num]);

		texture_library->get_texture_ref_texture_id(texture_ref_id[texture_num], &tid);
		texture_library->get_texture_name(tid, texture_name[texture_num], MAX_PATH);
	}
	
	return	GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::set_texture_addr_mode( U32 texture_num, TC_COORD which_uv, TC_ADDRMODE mode ) 
{
	if(texture_num > 1)
	{
		return	GR_GENERIC;
	}

	SET_TC_ADDRESS_MODE( texture_flags[texture_num], mode, which_uv );

	return	GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::get_texture_addr_mode( U32 texture_num, TC_COORD which_uv, TC_ADDRMODE *out_mode ) 
{
	if(texture_num > 1)
	{
		return	GR_GENERIC;
	}

	*out_mode = GET_TC_ADDRESS_MODE( texture_flags[texture_num], which_uv );

	return	GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::set_texture_wrap_mode( U32 texture_num, TC_WRAPMODE mode ) 
{
	if(texture_num > 1)
	{
		return	GR_GENERIC;
	}

	SET_TC_WRAP_MODE( texture_flags[texture_num], mode );
	
	return	GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::get_texture_wrap_mode( U32 texture_num, TC_WRAPMODE *out_mode ) 
{
	if(texture_num > 1)
	{
		return	GR_GENERIC;
	}

	*out_mode	=GET_TC_WRAP_MODE(texture_flags[texture_num]);
		
	return	GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::get_num_constants( U32 *out_num_constants ) 
{
	*out_num_constants	= 2;
	
	return	GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::set_constant( U32 constant_num, U32 num_values, float *values ) 
{
	if(constant_num)
	{
		return	GR_GENERIC;
	}

	ASSERT(num_values >= 3);
		
	d3d_material.diffuse.r	=values[0];
	d3d_material.diffuse.g	=values[1];
	d3d_material.diffuse.b	=values[2];

	return	GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::get_constant( U32 constant_num, U32 max_num_values, float *out_values ) 
{
	if(constant_num)
	{
		return	GR_GENERIC;
	}

	ASSERT(max_num_values >= 3);

	out_values[0]	=d3d_material.diffuse.r;
	out_values[1]	=d3d_material.diffuse.g;
	out_values[2]	=d3d_material.diffuse.b;

	return	GR_OK;
}

//

GENRESULT DcDtEcEtMaterial::get_constant_length( U32 constant_num, U32 *out_num_values ) 
{
	if( constant_num > 0 ) {
		return	GR_GENERIC;
	}
	else {
	
		*out_num_values = 3;
	
		return	GR_OK;
	}
}

//

//


// EOF

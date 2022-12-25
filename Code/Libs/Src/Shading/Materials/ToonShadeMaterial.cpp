/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	ToonShaderMaterial.cpp

	MAR.7 2000 Written by Yuichi Ito

Notice:
	 ToonShaderMaterial is generates Texture from parameters.
	 When you use this material. You need activate a Light.
	The Render method uses that parameter for calculation.
	Also, There is different calculations and it depends light type.
	Fastest calc is when you using Directional lighting.
	When you using spot light, It'll need more cpu time. But looks better.

=============================================================================*/
#define __TOONSHADERMATERIAL_CPP

#include "Materials.h"
#include <3dmath.h>
#include <fvf.h>

//-----------------------------------------------------------------------------
const char *CLSID_ToonShaderMaterial	= "ToonShader";

//####
static U32 gTextureHandle = 0;

/*-----------------------------------------------------------------------------
	ToonShader material
=============================================================================*/
struct ToonShaderMaterial : public IMaterial, public IMaterialProperties
{
	BEGIN_DACOM_MAP_INBOUND(ToonShaderMaterial)
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

	ToonShaderMaterial();
	~ToonShaderMaterial();

	GENRESULT init( DACOMDESC *desc );

protected:	// protected interface

	GENRESULT cleanup( void );

protected:	// protected data

	IDAComponent					*system_services;
	ITextureLibrary				*texture_library;
	IRenderPipeline				*render_pipeline;
	IVertexBufferManager	*vbuffer_manager;

	char									material_name[ IM_MAX_NAME_LEN ];
	char									texture_name[MAX_PATH];
	ITL_TEXTURE_REF_ID		texture_ref_id;
	ITL_TEXTUREFRAME_IRP	texture_frame;
	U32										texture_flags;

};

DECLARE_MATERIAL( ToonShaderMaterial, IS_SIMPLE );


/*-----------------------------------------------------------------------------
=============================================================================*/
ToonShaderMaterial::ToonShaderMaterial(void)
{
	init( NULL );
}

//-----------------------------------------------------------------------------
ToonShaderMaterial::~ToonShaderMaterial(void)
{
	cleanup();
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::init( DACOMDESC *desc )
{
	system_services = NULL;
	render_pipeline = NULL;
	texture_library = NULL;
	vbuffer_manager = NULL;

	texture_ref_id = ITL_INVALID_REF_ID;
	material_name[0] = 0;

	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::cleanup( void ) 
{
	if( texture_ref_id != ITL_INVALID_REF_ID ) texture_library->release_texture_ref( texture_ref_id );
	init( NULL );

	return GR_OK;
}
	
//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::initialize( IDAComponent *system_container ) 
{
	cleanup();

	if( system_container == NULL ) return GR_GENERIC;

	// Query to get interface pointer
	system_services = system_container;

	if( FAILED( system_container->QueryInterface( IID_ITextureLibrary, (void**)&texture_library ) ) ) {
		GENERAL_TRACE_1( "ToonShaderMaterial: intitialize: this material requires IID_ITextureLibrary\n" );
		return GR_GENERIC;
	}
	texture_library->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IRenderPipeline, (void**)&render_pipeline ) ) ) {
		GENERAL_TRACE_1( "ToonShaderMaterial: intitialize: this material requires IID_IRenderPipeline\n" );
		return GR_GENERIC;
	}
	render_pipeline->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IVertexBufferManager, (void**)&vbuffer_manager ) ) ) {
		GENERAL_TRACE_1( "ToonShaderMaterial: intitialize: this material requires IID_IVertexBufferManager\n" );
		return GR_GENERIC;
	}
	vbuffer_manager->Release();	// maintain artificial reference

	return GR_OK;
}

/*-----------------------------------------------------------------------------
	Data access methods
=============================================================================*/
GENRESULT ToonShaderMaterial::set_name( const char *new_name ) 
{
	strcpy( material_name, new_name );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::get_name( char *out_name, U32 max_name_len ) 
{
	strcpy( out_name, material_name );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::get_num_passes( U32 *out_num_passes )
{
	*out_num_passes = 1;
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::get_type( char *out_type, U32 max_type_len )
{
	strncpy( out_type, CLSID_ToonShaderMaterial, max_type_len );
	out_type[max_type_len-1] = 0;
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::get_num_textures( U32 *out_num_textures ) 
{
	*out_num_textures = 1;
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::get_texture( U32 texture_num, ITL_TEXTURE_REF_ID *out_trid ) 
{
	if( texture_num != 0 )
	{
		*out_trid = ITL_INVALID_REF_ID;
		return GR_GENERIC;
	}

	*out_trid = texture_ref_id;
	texture_library->add_ref_texture_ref( texture_ref_id );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::set_texture( U32 texture_num, ITL_TEXTURE_REF_ID trid ) 
{
	if( texture_num != 0 ) return GR_GENERIC;

	ITL_TEXTURE_ID tid;
		
	texture_library->release_texture_ref( texture_ref_id );
	texture_ref_id = trid;
		
	if( texture_ref_id != ITL_INVALID_REF_ID )
	{
		texture_library->add_ref_texture_ref( texture_ref_id );

		texture_library->get_texture_ref_texture_id( texture_ref_id, &tid );
		texture_library->get_texture_name( tid, texture_name, MAX_PATH );
	}
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::set_texture_addr_mode( U32 texture_num, TC_COORD which_uv, TC_ADDRMODE mode ) 
{
	if( texture_num != 0 ) return GR_GENERIC;
	SET_TC_ADDRESS_MODE( texture_flags, mode, which_uv );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::get_texture_addr_mode( U32 texture_num, TC_COORD which_uv, TC_ADDRMODE *out_mode ) 
{
	if( texture_num != 0 ) return GR_GENERIC;
	*out_mode = GET_TC_ADDRESS_MODE( texture_flags, which_uv );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::set_texture_wrap_mode( U32 texture_num, TC_WRAPMODE mode ) 
{
	if( texture_num != 0 ) return GR_GENERIC;
	SET_TC_WRAP_MODE( texture_flags, mode );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::get_texture_wrap_mode( U32 texture_num, TC_WRAPMODE *out_mode ) 
{
	if( texture_num != 0 ) return GR_GENERIC;
	*out_mode	= GET_TC_WRAP_MODE( texture_flags );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::get_num_constants( U32 *out_num_constants ) 
{
	*out_num_constants = 0;
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::set_constant( U32 constant_num, U32 num_values, float *values ) 
{
	return GR_GENERIC;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::get_constant( U32 constant_num, U32 max_num_values, float *out_values ) 
{
	return GR_GENERIC;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::get_constant_length( U32 constant_num, U32 *out_num_values ) 
{
	return GR_GENERIC;
}


//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::clone( IMaterial **out_material ) 
{
	IMaterial					*material;
	ToonShaderMaterial *new_material;
	DACOMDESC					desc;

	desc.interface_name = CLSID_ToonShaderMaterial;

	if( FAILED( DACOM_Acquire()->CreateInstance( &desc, (void**)&material ) ) ) return GR_GENERIC;

	// this is safe assuming no one registers a new component
	// under CLSID_ToonShaderMaterial
	new_material = static_cast<ToonShaderMaterial*>( material );

	if( FAILED( new_material->initialize( system_services ) ) )	return GR_GENERIC;



	// copy our state
	// DO NOT copy the texture ref id.  the clone will pick up
	// a new reference when it needs one.
	strcpy( new_material->texture_name, texture_name );
	new_material->texture_ref_id = ITL_INVALID_REF_ID;
	
//	memcpy( &new_material->d3d_material, &d3d_material, sizeof(d3d_material) );

	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::load_from_filesystem( IFileSystem *IFS ) 
{
	//#### should support mixed context color?
	return GR_OK;
}


/*-----------------------------------------------------------------------------
=============================================================================*/
GENRESULT ToonShaderMaterial::verify( U32 max_num_passes, float max_detail_level ) 
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

	if( texture_ref_id != ITL_INVALID_REF_ID ) {
		texture_library->release_texture_ref( texture_ref_id );
		texture_ref_id = ITL_INVALID_REF_ID;
	}

	texture_ref_id = trid;
	texture_frame.rp_texture_id = 0;


	//#### changed here
	//#### must be think about how to export toon shader texture!!
	if ( gTextureHandle == 0 )
	{
		static U16 pixels[] =
		{ 
0xb880, 0xb880, 0xb880, 0xb880, 0xb880, 0xb880, 0xb880, 0xb880, 0xb880, 0xb880, 0xb880, 0xb880, 0xd0c0, 0xd0c0, 0xd0c0, 0xd0c0, 0xd0c0, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xe900, 0xfc00, 0xe900, 0xfe08, 0xfe08, 0xfe08, 0xfe08, 0xfbde, 
		};

		PixelFormat fmt;
		fmt.init( PF_RGB5_A1 );

		// make fake texture
		int w = 64;
		int h = 64;

		if ( SUCCEEDED( render_pipeline->create_texture( w, h, fmt, 0, 0, gTextureHandle ) ) )
		{
			// make Speculare texture now
			RPLOCKDATA data;
			render_pipeline->lock_texture( gTextureHandle, 0, &data );
			char *dstPtr = (char*)data.pixels;
			for ( int y = 0; y < data.height; ++y )
			{
				memcpy( dstPtr, pixels, sizeof( pixels ) );
				dstPtr += data.pitch;
			}
			render_pipeline->unlock_texture( gTextureHandle, 0 );
		}
	}

	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::update( float dt )
{
	if( texture_ref_id != ITL_INVALID_REF_ID )
		texture_library->get_texture_ref_frame( texture_ref_id, ITL_FRAME_CURRENT, &texture_frame );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::apply( void ) 
{
	render_pipeline->set_render_state( D3DRS_LIGHTING, FALSE );
	render_pipeline->set_render_state( D3DRS_SPECULARENABLE, FALSE );

/*	if( d3d_material.diffuse.a < 0.999f )
	{
		render_pipeline->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
		render_pipeline->set_render_state( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		render_pipeline->set_render_state( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	}
	else
*/
	{
		render_pipeline->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );
	}

//	render_pipeline->set_texture_stage_texture( 0, texture_frame.rp_texture_id );
	render_pipeline->set_texture_stage_texture( 0, gTextureHandle );

	render_pipeline->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
	render_pipeline->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_TEXTURE );

	render_pipeline->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
	render_pipeline->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
	render_pipeline->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE );

	//####
	render_pipeline->set_texture_stage_state( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP  );
	render_pipeline->set_texture_stage_state( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP  );

	return GR_OK;
}


//-----------------------------------------------------------------------------
static int _analyzeVFV( U32 fvf, int *offsetNormal, int *offsetTexCoord )
{
	if ( offsetNormal ) *offsetNormal = -1;
	if ( offsetTexCoord )	*offsetTexCoord = -1;

	int dataSize = FVF_SIZEOF_POSITION( fvf );

	if ( ( fvf & D3DFVF_NORMAL ) && offsetNormal ) *offsetNormal = dataSize;
	dataSize += FVF_SIZEOF_OTHER( fvf ) + FVF_SIZEOF_TEXCOORDS( fvf );
	if ( offsetTexCoord ) *offsetTexCoord = FVF_TEXCOORD_U0_OFS( fvf );

	return dataSize;
}

//-----------------------------------------------------------------------------
GENRESULT ToonShaderMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
{
	IRP_VERTEXBUFFERHANDLE	vb;
	void										*vbmem;
	U32											num_verts;
	U32 fvf = ( Vertices->vertex_format & (~D3DFVF_TEXCOUNT_MASK) ) | D3DFVF_TEX1;

	if( FAILED( vbuffer_manager->acquire_vertex_buffer( fvf,
														   Vertices->num_vertices, 
														   0, 
														   DDLOCK_DISCARDCONTENTS,
														   0,
														   &vb,
														   &vbmem,
														   &fvf,
														   &num_verts ) ) ) return GR_OK;

	vbuffer_manager->copy_vertex_data( vbmem, fvf, Vertices );

	// Generates UV values
	int offsetNormal;
	int offsetTexCoord;
	int dataSize = _analyzeVFV( fvf, &offsetNormal, &offsetTexCoord );

	// calc light vector in object world
	// #### ToDo Should I need to code each vertext calculation version?
	D3DLIGHT7 light;
	render_pipeline->get_light( 0, &light );
	Vector lv( light.dvPosition.x, light.dvPosition.y, -light.dvPosition.z );
	lv = context->object_to_view->inverse_rotate_translate( lv );
	lv.normalize();

	int			incSize		= dataSize;
	float		*uvPtr		=	(float*)( (U32)vbmem + offsetTexCoord );
	Vector	*nrmlPtr	= (Vector*)( (U32)vbmem + offsetNormal );
	for ( U32 i = 0; i < Vertices->num_vertices; ++i  )
	{
		uvPtr[ 0 ] = dot_product( lv, *nrmlPtr );
		uvPtr[ 1 ] = 0.5f;

		uvPtr = (float*)( (U32)uvPtr + incSize );
		nrmlPtr = (Vector*)( (U32)nrmlPtr + incSize );
	}

	render_pipeline->unlock_vertex_buffer( vb );

	GENRESULT gr = render_pipeline->draw_indexed_primitive_vb(
			PrimitiveType, vb, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );

	vbuffer_manager->release_vertex_buffer( vb );

	return gr;
}


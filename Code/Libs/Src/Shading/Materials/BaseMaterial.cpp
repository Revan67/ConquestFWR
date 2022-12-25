/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	BaseMaterial.cpp

	MAR.15 2000 Written by Yuichi Ito

=============================================================================*/
#define __BASEMATERIAL_CPP

#include "BaseMaterial.h"

using namespace std;

//-----------------------------------------------------------------------------
const char *CLSID_BaseMaterial						= "BaseMaterial";
const char *CLSID_TwoSidedMaterial				= "TwoSided";
const char *CLSID_DcDtTwoMaterial					= "DcDtTwo";
const char *CLSID_DcDtOcTwoMaterial				= "DcDtOcTwo";
const char *CLSID_DcDtOcOtTwoMaterial			= "DcDtOcOtTwo";
const char *CLSID_DcDtEcTwoMaterial				= "DcDtEcTwo";

const char *CLSID_SpecularMaterial				= "Specular";
const char *CLSID_ReflectionMaterial			= "Reflection";
const char *CLSID_SpecularGlossMaterial		= "SpecularGloss";
const char *CLSID_ReflectionGlossMaterial	= "ReflectionGloss";
const char *CLSID_EmbossBumpMaterial			= "EmbossBump";

const char *CLSID_ProceduralMaterial				= "Procedural";

/*-----------------------------------------------------------------------------
	TexInfo class implementations
=============================================================================*/
void TexInfo::operator=( const TexInfo &a )
{
	// copy our state
	// DO NOT copy the texture ref id.  the clone will pick up
	// a new reference when it needs one.
	strcpy( name, a.name );
	ref_id = ITL_INVALID_REF_ID;
}

//-----------------------------------------------------------------------------
void TexInfo::init()
{
	ref_id		= ITL_INVALID_REF_ID;
	name[0]		= 0;
}

//-----------------------------------------------------------------------------
void TexInfo::cleanup( ITextureLibrary *texture_library )
{
	if( ref_id != ITL_INVALID_REF_ID )
		texture_library->release_texture_ref( ref_id );
}

//-----------------------------------------------------------------------------
ITL_TEXTURE_REF_ID TexInfo::addRef( ITextureLibrary *texture_library )
{
	texture_library->add_ref_texture_ref( ref_id );
	return ref_id;
}

//-----------------------------------------------------------------------------
GENRESULT TexInfo::set_texture( ITextureLibrary *texture_library, ITL_TEXTURE_REF_ID trid ) 
{
	ITL_TEXTURE_ID tid;
		
	texture_library->release_texture_ref( ref_id );
	ref_id = trid;
		
	if( ref_id != ITL_INVALID_REF_ID )
	{
		texture_library->add_ref_texture_ref( ref_id );

		texture_library->get_texture_ref_texture_id( ref_id, &tid );
		texture_library->get_texture_name( tid, name, MAX_PATH );
	}
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT TexInfo::load( int stage, IFileSystem *IFS )
{
	static char *nameStr = "T0_name";
	static char *flagStr = "T0_flags";

	nameStr[ 1 ] = flagStr[ 1 ] = '0' + char(stage); // Um this is kind of hack...

	if ( FAILED( read_string( IFS, nameStr, MAX_PATH, name ) ) )
	{
		name[ 0 ] = 0;
		return GR_GENERIC;
	}

	if ( FAILED( read_type<U32>( IFS, flagStr, &flags ) ) )
	{
		flags = 0;
		return GR_GENERIC;
	}
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT TexInfo::verify( ITextureLibrary *texture_library )
{
	ITL_TEXTURE_ID			tid;
	ITL_TEXTURE_REF_ID trid = ITL_INVALID_REF_ID;

	if( name[0] != 0 )
	{
		if( SUCCEEDED( texture_library->get_texture_id( name, &tid ) ) )
		{
			texture_library->add_ref_texture_id( tid, &trid );
			texture_library->release_texture_id( tid );
		}
	}

	if( ref_id != ITL_INVALID_REF_ID ) texture_library->release_texture_ref( ref_id );

	ref_id							= trid;
	frame.rp_texture_id = 0;

	return GR_OK;
}

//-----------------------------------------------------------------------------
inline GENRESULT TexInfo::update( ITextureLibrary *texture_library, float dt )
{
	if( ref_id != ITL_INVALID_REF_ID )
	{
		texture_library->get_texture_ref_frame( ref_id, ITL_FRAME_CURRENT, &frame );
	}
	return GR_OK;
}

typedef vector<TexInfo> TexInfoArray;

/*-----------------------------------------------------------------------------
	BaseMaterial
=============================================================================*/
DECLARE_MATERIAL( BaseMaterial, IS_SIMPLE );

/*-----------------------------------------------------------------------------
=============================================================================*/
BaseMaterial::BaseMaterial( void ) : m_detail( 1.0f ), m_numPass( 1 )
{
	init( NULL );
}

//-----------------------------------------------------------------------------
BaseMaterial::~BaseMaterial( void )
{
	cleanup();
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::init( DACOMDESC *desc )
{
	system_services = NULL;
	render_pipeline = NULL;
	texture_library = NULL;
	vbuffer_manager = NULL;

	material_name[0]	= 0;

	Textures.clear();

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

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::cleanup( void ) 
{
	for ( TexInfoArray::iterator it = Textures.begin(); it != Textures.end(); ++it )
	{
		it->cleanup( texture_library );
	}

	init( NULL );

	return GR_OK;
}
	
//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::initialize( IDAComponent *system_container ) 
{
	cleanup();

	if( system_container == NULL ) return GR_GENERIC;

	// Query to get interface pointer
	system_services = system_container;

	if( FAILED( system_container->QueryInterface( IID_ITextureLibrary, (void**)&texture_library ) ) ) {
		GENERAL_TRACE_1( "BaseMaterial: intitialize: this material requires IID_ITextureLibrary\n" );
		return GR_GENERIC;
	}
	texture_library->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IRenderPipeline, (void**)&render_pipeline ) ) ) {
		GENERAL_TRACE_1( "BaseMaterial: intitialize: this material requires IID_IRenderPipeline\n" );
		return GR_GENERIC;
	}
	render_pipeline->Release();	// maintain artificial reference

	if( FAILED( system_container->QueryInterface( IID_IVertexBufferManager, (void**)&vbuffer_manager ) ) ) {
		GENERAL_TRACE_1( "BaseMaterial: intitialize: this material requires IID_IVertexBufferManager\n" );
		return GR_GENERIC;
	}
	vbuffer_manager->Release();	// maintain artificial reference

	return GR_OK;
}

/*-----------------------------------------------------------------------------
	Data access methods
=============================================================================*/
GENRESULT BaseMaterial::set_name( const char *new_name ) 
{
	strcpy( material_name, new_name );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::get_name( char *out_name, U32 max_name_len ) 
{
	strcpy( out_name, material_name );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::get_num_passes( U32 *out_num_passes )
{
	*out_num_passes = 1;
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::get_type( char *out_type, U32 max_type_len )
{
	strncpy( out_type, CLSID_BaseMaterial, max_type_len );
	out_type[max_type_len-1] = 0;
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::get_num_textures( U32 *out_num_textures ) 
{
	*out_num_textures = 1;
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::get_texture( U32 texture_num, ITL_TEXTURE_REF_ID *out_trid ) 
{
	if ( Textures.size() <= texture_num )
	{
		*out_trid = ITL_INVALID_REF_ID;
		return GR_GENERIC;
	}
	*out_trid = Textures[ texture_num ].addRef( texture_library );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::set_texture( U32 texture_num, ITL_TEXTURE_REF_ID trid ) 
{
	if ( Textures.size() <= texture_num ) return GR_GENERIC;

	return Textures[ texture_num ].set_texture( texture_library, trid );
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::set_texture_addr_mode( U32 texture_num, TC_COORD which_uv, TC_ADDRMODE mode ) 
{
	if ( Textures.size() <= texture_num ) return GR_GENERIC;

	SET_TC_ADDRESS_MODE( Textures[ texture_num ].flags, mode, which_uv );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::get_texture_addr_mode( U32 texture_num, TC_COORD which_uv, TC_ADDRMODE *out_mode ) 
{
	if ( Textures.size() <= texture_num ) return GR_GENERIC;
	
	*out_mode = GET_TC_ADDRESS_MODE( Textures[ texture_num ].flags, which_uv );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::set_texture_wrap_mode( U32 texture_num, TC_WRAPMODE mode ) 
{
	if ( Textures.size() <= texture_num ) return GR_GENERIC;
	
	SET_TC_WRAP_MODE( Textures[ texture_num ].flags, mode );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::get_texture_wrap_mode( U32 texture_num, TC_WRAPMODE *out_mode ) 
{
	if ( Textures.size() <= texture_num ) return GR_GENERIC;
	
	*out_mode = GET_TC_WRAP_MODE( Textures[ texture_num ].flags );
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::get_num_constants( U32 *out_num_constants ) 
{
	*out_num_constants = 0;
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::set_constant( U32 constant_num, U32 num_values, float *values ) 
{
	return GR_GENERIC;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::get_constant( U32 constant_num, U32 max_num_values, float *out_values ) 
{
	return GR_GENERIC;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::get_constant_length( U32 constant_num, U32 *out_num_values ) 
{
	return GR_GENERIC;
}


//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::clone( IMaterial **out_material ) 
{
	IMaterial					*material;
	BaseMaterial *new_material;
	DACOMDESC					desc;

	desc.interface_name = CLSID_BaseMaterial;

	if( FAILED( DACOM_Acquire()->CreateInstance( &desc, (void**)&material ) ) ) return GR_GENERIC;

	// this is safe assuming no one registers a new component
	// under CLSID_BaseMaterial
	new_material = static_cast<BaseMaterial*>( material );

	if( FAILED( new_material->initialize( system_services ) ) )	return GR_GENERIC;

	new_material->Textures = Textures;
	memcpy( &new_material->d3d_material, &d3d_material, sizeof(d3d_material) );

	return GR_OK;
}

//-----------------------------------------------------------------------------
#ifdef  _MSC_VER
#pragma warning( disable: 4701 )
#endif
GENRESULT BaseMaterial::load_from_filesystem( IFileSystem *IFS ) 
{
	static struct	{
		const char		*clsid;
		MatType			type;
	} table[] = 
	{
		{ CLSID_DcDtTwoMaterial,					mtDcDtTwo					},
		{ CLSID_DcDtOcTwoMaterial,				mtDcDtOcTwo				},
		{ CLSID_DcDtOcOtTwoMaterial,			mtDcDtOcOtTwo			},
		{ CLSID_DcDtEcTwoMaterial,				mtDcDtEcTwo				},
		{ CLSID_SpecularMaterial,					mtSpecular				},
		{ CLSID_ReflectionMaterial,				mtReflection			},
		{ CLSID_SpecularGlossMaterial,		mtSpecularGloss		},
		{ CLSID_ReflectionGlossMaterial,	mtReflectionGloss	},
		{ NULL,														MatType(0)				}
	};

	Vector color;

	char material_type[IM_MAX_TYPE_LEN];

	Type = mtDcDtOcOtTwo;	//#####hook!
	// determine the type
	read_string( IFS, "Type", IM_MAX_TYPE_LEN, material_type );
	for ( int i = 0; table[ i ].clsid; ++i )
	{
		if ( strcmp( material_type, table[ i ].clsid ) == 0 )
		{
			Type = table[ i ].type;
			break;
		}
	}

	// read constants
	if( SUCCEEDED( read_type<Vector>( IFS, "C0", &color ) ) )
	{
		d3d_material.diffuse.r = color.x;
		d3d_material.diffuse.g = color.y;
		d3d_material.diffuse.b = color.z;

		d3d_material.ambient.r = color.x;
		d3d_material.ambient.g = color.y;
		d3d_material.ambient.b = color.z;
	}

	if( Type == mtDcDtEcTwo )
	{
		if( SUCCEEDED( read_type<Vector>( IFS, "C1", &color ) ) ) {
			d3d_material.emissive.r = color.x;
			d3d_material.emissive.g = color.y;
			d3d_material.emissive.b = color.z;
		}
	}
	else
	{
		// opacity types
		if( SUCCEEDED( read_type<float>( IFS, "C1", &color.x ) ) )
		{
			d3d_material.diffuse.a = color.x;
		}
	}

	TexInfo info;
	for ( int it = 0; it < 8; ++it )
	{
		if ( FAILED( info.load( it, IFS ) ) ) break;
		Textures.push_back( info );
		info.cleanup( texture_library );	// for Just in case
	}
	return GR_OK;
}

#ifdef  _MSC_VER
#pragma warning( default: 4701 )
#endif

/*-----------------------------------------------------------------------------
=============================================================================*/
GENRESULT BaseMaterial::verify( U32 max_num_passes, float max_detail_level ) 
{
	m_numPass	= max_num_passes;
	if ( m_numPass == 0 ) m_numPass = 256;	// ####
	m_detail		= max_detail_level;

	for ( TexInfoArray::iterator it = Textures.begin(); it != Textures.end(); ++it )
		it->verify( texture_library );

	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::update( float dt )
{
	for ( TexInfoArray::iterator it = Textures.begin(); it != Textures.end(); ++it )
	{
		it->update( texture_library, dt );
	}
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT BaseMaterial::apply( void ) 
{
	// Usual setting
	render_pipeline->set_material( &d3d_material );

	render_pipeline->set_render_state( D3DRS_LIGHTING, TRUE );
	render_pipeline->set_render_state( D3DRS_SPECULARENABLE, FALSE );

	if( d3d_material.diffuse.a < 0.999f || IsUseAlpha() )
	{
		render_pipeline->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
		render_pipeline->set_render_state( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		render_pipeline->set_render_state( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	}
	else
	{
		render_pipeline->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );
	}

	for ( int i = 0; i < Textures.size(); ++i )
	{
		DWORD value = ( i == 0 )? D3DTA_DIFFUSE: D3DTA_CURRENT;

		render_pipeline->set_texture_stage_texture( i, Textures[ i ].frame.rp_texture_id );

		render_pipeline->set_texture_stage_state( i, D3DTSS_COLOROP,	  D3DTOP_MODULATE ); 
		render_pipeline->set_texture_stage_state( i, D3DTSS_COLORARG1, D3DTA_TEXTURE ); 
		render_pipeline->set_texture_stage_state( i, D3DTSS_COLORARG2, value ); 

		render_pipeline->set_texture_stage_state( i, D3DTSS_ALPHAOP,	  D3DTOP_MODULATE ); 
		render_pipeline->set_texture_stage_state( i, D3DTSS_ALPHAARG1, D3DTA_TEXTURE ); 
		render_pipeline->set_texture_stage_state( i, D3DTSS_ALPHAARG2, value ); 
	}

	return GR_OK;
}

//-----------------------------------------------------------------------------
int BaseMaterial::analyzeVFV( U32 fvf, int *offsetNormal, int *offsetTexCoord )
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
GENRESULT BaseMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
{
	render_pipeline->draw_indexed_primitive_vb(
				PrimitiveType, context->vertex_buffer, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );

	return GR_OK;
}


//-----------------------------------------------------------------------------
void BaseMaterial::getLightVector( const MaterialContext *context, Vector &lv )
{
		// calc light vector in object world
	D3DLIGHT7 light;
	render_pipeline->get_light( 0, &light );

	if ( light.dltType == D3DLIGHT_DIRECTIONAL )
	{
		lv.set( light.dvDirection.x, light.dvDirection.y, -light.dvDirection.z );
		lv = context->object_to_view->inverse_rotate( lv );
	}
	else
	{
		lv.set( light.dvPosition.x, light.dvPosition.y, -light.dvPosition.z );
		lv = context->object_to_view->inverse_rotate_translate( lv );
	}
	lv.normalize();
}


/*-----------------------------------------------------------------------------
	Texture coordinate functions	
=============================================================================*/
void texgen_sphereMap( MaterialContext *context, BaseMaterial::VBIterator &it )
{
	for ( ; !it.isEnd() ; ++it )
	{
		Vector eyeV = context->object_to_view->rotate_translate( it.vertex() );
//		eyeV.z = -eyeV.z;
		eyeV.normalize();

		// calc refrection vector
		Vector rv = reflection( context->object_to_view->rotate( it.normal() ), eyeV );

		// projection to tex-Coordinate
		rv.z += 1.0f;
		float rm = 1.0f / ( 2.0f * rv.magnitude() );

		it.uv().set( rv.x * rm + 0.5f, rv.y * rm + 0.5f );
	}
}


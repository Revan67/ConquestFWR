/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	SpecularMaterial.cpp

	MAR.15 2000 Written by Yuichi Ito

=============================================================================*/
#define __SPECULARMATERIAL_CPP

#include "BaseMaterial.h"

/*-----------------------------------------------------------------------------
	specular texture
=============================================================================*/
#define _SPC_TEXW	64
#define _SPC_TEXH	64
#define _SPC_MAXSHININESS	32

class _SpecularTexture
{
public:
	static void addRef( IRenderPipeline *render_pipeline );
	static void release( IRenderPipeline *render_pipeline );
	static U32 hTexture(){ return m_hTexture; }

	static U32 m_hTexture;
	static U32 m_refcnt;
};

U32 _SpecularTexture::m_hTexture	= 0;
U32 _SpecularTexture::m_refcnt		= 0;

//-----------------------------------------------------------------------------
void _SpecularTexture::addRef( IRenderPipeline *render_pipeline )
{
	m_refcnt++;
	if ( m_hTexture ) return;

	// If There is no No texture handle, Then make it.
	PixelFormat fmt;
	fmt.init( PF_RGB5_A1 );
	if ( SUCCEEDED( render_pipeline->create_texture( _SPC_TEXW, _SPC_TEXH, fmt, 0, 0, m_hTexture ) ) )
	{
		// make Speculare texture now
		RPLOCKDATA data;
		render_pipeline->lock_texture( m_hTexture, 0, &data );
		char *dstPtr = (char*)data.pixels;
		for ( int y = 0; y < data.height; ++y )
		{
			double shininess = (double)y / (double)data.height * _SPC_MAXSHININESS;
			for ( int x = 0; x < data.width; ++x, dstPtr += 2 )
			{
				U16 ans = (U16)( pow( (double)x / (double)data.width, shininess ) * 32 );
				U16 col = ans | ( ans << 5 );
				col = col | ( ans << 10 );
				col |= 0x8000;
				*((U16*)dstPtr) = col;
			}
		}
		render_pipeline->unlock_texture( m_hTexture, 0 );
	}
}

//-----------------------------------------------------------------------------
void _SpecularTexture::release( IRenderPipeline *render_pipeline )
{
	// If refcnt == 0 then release texture
	m_refcnt--;
	if ( m_refcnt == 0 )
	{
		render_pipeline->destroy_texture( m_hTexture );
		m_hTexture = 0;
	}
}

/*-----------------------------------------------------------------------------
	Specular Material
=============================================================================*/
struct SpecularMaterial : public BaseMaterial
{
public:		// public interface

	SpecularMaterial() : BaseMaterial(){};
	virtual ~SpecularMaterial();

	GENRESULT COMAPI initialize( IDAComponent *system_container );
	GENRESULT COMAPI render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) ;

protected:
	void texgen_specularMap( MaterialContext *context, BaseMaterial::VBIterator &it, float vValue );
};

DECLARE_MATERIAL( SpecularMaterial, IS_SIMPLE );

//-----------------------------------------------------------------------------
SpecularMaterial::~SpecularMaterial()
{
	_SpecularTexture::release( render_pipeline );
}

//-----------------------------------------------------------------------------
GENRESULT SpecularMaterial::initialize( IDAComponent *system_container )
{
	GENRESULT result = BaseMaterial::initialize( system_container );
	_SpecularTexture::addRef( render_pipeline );
	return result;
}

//-----------------------------------------------------------------------------
void SpecularMaterial::texgen_specularMap( MaterialContext *context, BaseMaterial::VBIterator &it, float vValue )
{
	// get Eye-Vector & light vector
	Vector eyeV, lv;
	getEyeVector( context, eyeV );
	getLightVector( context, lv );

	// calc UV's
	for ( ; !it.isEnd(); ++it )
	{
		Vector rv = reflection( it.normal(), eyeV );
		it.uv().set( dot_product( rv, lv ), vValue );
	}
}

//-----------------------------------------------------------------------------
GENRESULT SpecularMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
{
	IRP_VERTEXBUFFERHANDLE	vb;
	void										*vbmem;
	U32											num_verts;
	U32			fvf;

	float		vValue = d3d_material.dvPower * (1/_SPC_MAXSHININESS);

#if 0 //#### for testing
	BOOL multiPass = m_numPass >= 2;

	if ( multiPass )
	{
		fvf = Vertices->vertex_format + D3DFVF_TEX1;																// append second stage UVs
		if( FAILED( vbuffer_manager->acquire_vertex_buffer( fvf, Vertices->num_vertices, 0, DDLOCK_DISCARDCONTENTS,
													   0, &vb, &vbmem, &fvf, &num_verts ) ) ) return GR_OK;

		vbuffer_manager->copy_vertex_data( vbmem, fvf, Vertices );
		VBIterator vbi( vbmem, fvf, Vertices->num_vertices );
		texgen_specularMap( context, vbi, vValue );
		render_pipeline->unlock_vertex_buffer( vb );

		// set second stage texture states
		render_pipeline->set_texture_stage_state( 1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP  );
		render_pipeline->set_texture_stage_state( 1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP  );
		render_pipeline->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_ADD );
		render_pipeline->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		render_pipeline->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
		render_pipeline->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

		render_pipeline->draw_indexed_primitive_vb( PrimitiveType, vb, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );
		vbuffer_manager->release_vertex_buffer( vb );
	}
	else
#endif
	{
		// first. draw normaly
		BaseMaterial::render( context, PrimitiveType, Vertices, StartVertex, NumVertices,
														FaceIndices, NumFaceIndices, im_rf_flags );

		// then add Specular mapped texture
		fvf = ( Vertices->vertex_format & (~D3DFVF_TEXCOUNT_MASK) ) | D3DFVF_TEX1;	// needs one texcoord
		if( FAILED( vbuffer_manager->acquire_vertex_buffer( fvf, Vertices->num_vertices, 0, DDLOCK_DISCARDCONTENTS,
													   0, &vb, &vbmem, &fvf, &num_verts ) ) ) return GR_OK;

		vbuffer_manager->copy_vertex_data( vbmem, fvf, Vertices );
		VBIterator vbi( vbmem, fvf, Vertices->num_vertices );
		texgen_specularMap( context, vbi, vValue );
		render_pipeline->unlock_vertex_buffer( vb );

		// set second texture states
		render_pipeline->set_render_state( D3DRS_LIGHTING, FALSE );

		render_pipeline->set_render_state( D3DRS_ZFUNC , D3DCMP_LESSEQUAL  );
		render_pipeline->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
		render_pipeline->set_render_state( D3DRS_SRCBLEND, D3DBLEND_ONE );
		render_pipeline->set_render_state( D3DRS_DESTBLEND, D3DBLEND_ONE );

		render_pipeline->set_texture_stage_texture( 0, _SpecularTexture::hTexture() );
		render_pipeline->set_texture_stage_state( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP  );
		render_pipeline->set_texture_stage_state( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP  );

		render_pipeline->draw_indexed_primitive_vb( PrimitiveType, vb, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );
		vbuffer_manager->release_vertex_buffer( vb );
	}
	return GR_OK;
}

/*-----------------------------------------------------------------------------
	Specular Gloss Material
=============================================================================*/
struct SpecularGlossMaterial : public BaseMaterial
{
public:		// public interface

	SpecularGlossMaterial() : BaseMaterial(){};
	virtual ~SpecularGlossMaterial();

	GENRESULT COMAPI initialize( IDAComponent *system_container );
	GENRESULT COMAPI render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) ;
	GENRESULT COMAPI apply( void );

protected:
	void texgen_specularMap( MaterialContext *context, BaseMaterial::VBIterator &it, float vValue );

};

DECLARE_MATERIAL( SpecularGlossMaterial, IS_SIMPLE );

//-----------------------------------------------------------------------------
SpecularGlossMaterial::~SpecularGlossMaterial()
{
	_SpecularTexture::release( render_pipeline );
}

//-----------------------------------------------------------------------------
GENRESULT SpecularGlossMaterial::initialize( IDAComponent *system_container )
{
	GENRESULT result = BaseMaterial::initialize( system_container );
	_SpecularTexture::addRef( render_pipeline );
	return result;
}

//-----------------------------------------------------------------------------
GENRESULT SpecularGlossMaterial::apply( void )
{
	// render function takes care of this...

	d3d_material.diffuse.a = 0.5f;//####
	return GR_OK;
}

//-----------------------------------------------------------------------------
void SpecularGlossMaterial::texgen_specularMap( MaterialContext *context, BaseMaterial::VBIterator &it, float vValue )
{
	// get Eye-Vector & light vector
	Vector eyeV, lv;
	getEyeVector( context, eyeV );
	getLightVector( context, lv );

	// calc UV's
	for ( ; !it.isEnd(); ++it )
	{
		Vector rv = reflection( it.normal(), eyeV );
		it.uv().set( dot_product( rv, lv ), vValue );
	}
}

//-----------------------------------------------------------------------------
GENRESULT SpecularGlossMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
{
	IRP_VERTEXBUFFERHANDLE	vb;
	void										*vbmem;
	U32											num_verts;
	U32			fvf;

	BOOL multiPass = m_numPass >= 2;
	

	float		vValue = d3d_material.dvPower * (1/_SPC_MAXSHININESS);

	//#### for testing
	multiPass = FALSE; vValue = 0.4f;


	if ( multiPass )
	{
	}
	else
	{
		// first. draw specular map
		render_pipeline->set_render_state( D3DRS_LIGHTING, FALSE );
		render_pipeline->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );

		render_pipeline->set_texture_stage_texture( 0, _SpecularTexture::hTexture() );
		render_pipeline->set_texture_stage_state( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP  );
		render_pipeline->set_texture_stage_state( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP  );

		fvf = ( Vertices->vertex_format & (~D3DFVF_TEXCOUNT_MASK) ) | D3DFVF_TEX1;	// needs one texcoord
		if( FAILED( vbuffer_manager->acquire_vertex_buffer( fvf, Vertices->num_vertices, 0, DDLOCK_DISCARDCONTENTS,
													   0, &vb, &vbmem, &fvf, &num_verts ) ) ) return GR_OK;

		vbuffer_manager->copy_vertex_data( vbmem, fvf, Vertices );
		VBIterator vbi( vbmem, fvf, Vertices->num_vertices );
		texgen_specularMap( context, vbi, vValue );
		render_pipeline->unlock_vertex_buffer( vb );

		render_pipeline->draw_indexed_primitive_vb( PrimitiveType, vb, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );
		vbuffer_manager->release_vertex_buffer( vb );

		// Then Draw gloss map
		render_pipeline->set_render_state( D3DRS_ZFUNC , D3DCMP_LESSEQUAL  );
		BaseMaterial::apply();
		render_pipeline->set_render_state( D3DRS_SRCBLEND, D3DBLEND_ONE );						// This is the key point!!
		render_pipeline->set_render_state( D3DRS_DESTBLEND, D3DBLEND_SRCALPHA );

		BaseMaterial::render( context, PrimitiveType, Vertices, StartVertex, NumVertices,
														FaceIndices, NumFaceIndices, im_rf_flags );


	}
	return GR_OK;
}




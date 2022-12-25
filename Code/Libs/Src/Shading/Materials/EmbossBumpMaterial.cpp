/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	EmbossBumpMaterial.cpp

	MAR.17 2000 Written by Yuichi Ito

=============================================================================*/
#define __EMBOSSBUMPMATERIAL_CPP

#include "BaseMaterial.h"

/*-----------------------------------------------------------------------------
	EmbossBump Material
=============================================================================*/
struct EmbossBumpMaterial : public BaseMaterial
{
public:		// public interface

	EmbossBumpMaterial() : BaseMaterial(){};
	virtual ~EmbossBumpMaterial(){};

	GENRESULT COMAPI load_from_filesystem( IFileSystem *IFS );
	GENRESULT COMAPI apply( void );
	GENRESULT COMAPI render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) ;

protected:
	void texgen_bumpMap( MaterialContext *context, BaseMaterial::VBIterator &it, float offset );

};

DECLARE_MATERIAL( EmbossBumpMaterial, IS_SIMPLE );

//-----------------------------------------------------------------------------
GENRESULT EmbossBumpMaterial::load_from_filesystem( IFileSystem *IFS )
{
	GENRESULT result = BaseMaterial::load_from_filesystem( IFS );
	
	//#### hack!!
	TexInfo info;
	strcpy( info.name, "basebump.bmp" );
	info.flags = 0;
	Textures.push_back( info );

	strcpy( info.name, "invbump.bmp" );
	info.flags = 0;
	Textures.push_back( info );

	return result;
}

//-----------------------------------------------------------------------------
GENRESULT EmbossBumpMaterial::apply( void )
{
	// render function takes care of this...
	return GR_OK;
}

//-----------------------------------------------------------------------------
void EmbossBumpMaterial::texgen_bumpMap( MaterialContext *context, BaseMaterial::VBIterator &it, float offset )
{
	// calc light vector in object world
	D3DLIGHT7 light;
	render_pipeline->get_light( 0, &light );

	Vector lp;
	//#### you know what, This codes doesn't work correctory
	//     I should use projection matrix instead of this!!
	if ( light.dltType == D3DLIGHT_DIRECTIONAL )
	{
		lp.set( light.dvDirection.x, light.dvDirection.y, -light.dvDirection.z );
		lp = context->object_to_view->inverse_rotate( lp );
		lp.normalize();
		lp *= offset;
		// calc UV's
		for ( ; !it.isEnd(); ++it )
		{
			BaseMaterial::TexCoord &tc = it.uv();
			tc.u += lp.x;
			tc.v += lp.y;
		}
	}
	else
	{
		lp.set( light.dvPosition.x, light.dvPosition.y, -light.dvPosition.z );
		lp = context->object_to_view->inverse_rotate_translate( lp );

		// calc UV's
		for ( ; !it.isEnd(); ++it )
		{
			Vector r = it.vertex() - lp;
			r.normalize();
			r *= offset;

			BaseMaterial::TexCoord &tc = it.uv();
			tc.u += r.x;
			tc.v += r.y;
		}
	}

}

//-----------------------------------------------------------------------------
GENRESULT EmbossBumpMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
{
	IRP_VERTEXBUFFERHANDLE	vb;
	void										*vbmem;
	U32											num_verts;
	U32											fvf;

	BOOL multiPass = m_numPass >= 2;
	
	//#### for testing
	float offset = 0.004f;
	multiPass = FALSE;

	if ( multiPass )
	{
	}
	else
	{

		// 1st stage -> draw Bump map
		// 2nd stage -> draw InvBump map w/ generate texture coordinate
		// 3rd stage -> draw base texture w/ mudulate*2 operator

		// disable other stage texture drawing
		render_pipeline->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
		render_pipeline->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		render_pipeline->set_texture_stage_state( 2, D3DTSS_COLOROP, D3DTOP_DISABLE );
		render_pipeline->set_texture_stage_state( 2, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

		// 1st stage
		render_pipeline->set_render_state( D3DRS_LIGHTING, FALSE );
		render_pipeline->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );
		render_pipeline->set_texture_stage_state( 0, D3DTSS_COLOROP,	 D3DTOP_SELECTARG1 );
		render_pipeline->set_texture_stage_state( 0, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE );

#if 1
		render_pipeline->set_texture_stage_texture( 0, Textures[ 1 ].frame.rp_texture_id );

		BaseMaterial::render( context, PrimitiveType, Vertices, StartVertex, NumVertices,
														FaceIndices, NumFaceIndices, im_rf_flags );
#endif
#if 1
		// 2nd stage
		render_pipeline->set_render_state( D3DRS_ZFUNC , D3DCMP_LESSEQUAL  );
		render_pipeline->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
		render_pipeline->set_render_state( D3DRS_SRCBLEND, D3DBLEND_ONE );
		render_pipeline->set_render_state( D3DRS_DESTBLEND, D3DBLEND_ONE );

		U32 id = Textures[ 2 ].frame.rp_texture_id;
		render_pipeline->set_texture_stage_texture( 0, id );

		fvf = ( Vertices->vertex_format & (~D3DFVF_TEXCOUNT_MASK) ) | D3DFVF_TEX1;	// needs one texcoord
		if( FAILED( vbuffer_manager->acquire_vertex_buffer( fvf, Vertices->num_vertices, 0, DDLOCK_DISCARDCONTENTS,
													   0, &vb, &vbmem, &fvf, &num_verts ) ) ) return GR_OK;

		vbuffer_manager->copy_vertex_data( vbmem, fvf, Vertices );
		VBIterator vbi( vbmem, fvf, Vertices->num_vertices );
		texgen_bumpMap( context, vbi, offset );
		render_pipeline->unlock_vertex_buffer( vb );
		render_pipeline->draw_indexed_primitive_vb( PrimitiveType, vb, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );
		vbuffer_manager->release_vertex_buffer( vb );
#endif

		// 3rd stage
		BaseMaterial::apply();
#if 1
		render_pipeline->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
		render_pipeline->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		render_pipeline->set_texture_stage_state( 2, D3DTSS_COLOROP, D3DTOP_DISABLE );
		render_pipeline->set_texture_stage_state( 2, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

		render_pipeline->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
		render_pipeline->set_render_state( D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR );
		render_pipeline->set_render_state( D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR );


//		render_pipeline->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE2X );
		BaseMaterial::render( context, PrimitiveType, Vertices, StartVertex, NumVertices,
														FaceIndices, NumFaceIndices, im_rf_flags );
#endif
	}

	return GR_OK;
}


/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	ReflectionMaterial.cpp

	MAR.16 2000 Written by Yuichi Ito

=============================================================================*/
#define __REFLECTIONMATERIAL_CPP

#include "BaseMaterial.h"

//#### for testing
#define TEST_TEXNAME	"spheremap.bmp"

/*-----------------------------------------------------------------------------
	Reflection Material
=============================================================================*/
struct ReflectionMaterial : public BaseMaterial
{
public:		// public interface

	ReflectionMaterial() : BaseMaterial(){};
	virtual ~ReflectionMaterial(){};

	GENRESULT COMAPI load_from_filesystem( IFileSystem *IFS );

	GENRESULT COMAPI render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) ;

protected:
};

DECLARE_MATERIAL( ReflectionMaterial, IS_SIMPLE );

//-----------------------------------------------------------------------------
GENRESULT ReflectionMaterial::load_from_filesystem( IFileSystem *IFS )
{
	GENRESULT result = BaseMaterial::load_from_filesystem( IFS );
	
	//#### hack!!
	TexInfo info;
	strcpy( info.name, TEST_TEXNAME );
	info.flags = 0;
	Textures.push_back( info );

	return result;
}

//-----------------------------------------------------------------------------
GENRESULT ReflectionMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
{
	IRP_VERTEXBUFFERHANDLE	vb;
	void										*vbmem;
	U32											num_verts;
	U32											fvf;

	BOOL multiPass = m_numPass >= 2;
	
	//#### for testing
	multiPass = FALSE;

	if ( multiPass )
	{
	}
	else
	{
		render_pipeline->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
		render_pipeline->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

		// first. draw normaly
		BaseMaterial::render( context, PrimitiveType, Vertices, StartVertex, NumVertices,
														FaceIndices, NumFaceIndices, im_rf_flags );

		// then add Reflection mapped texture
		fvf = ( Vertices->vertex_format & (~D3DFVF_TEXCOUNT_MASK) ) | D3DFVF_TEX1;	// needs one texcoord
		if( FAILED( vbuffer_manager->acquire_vertex_buffer( fvf, Vertices->num_vertices, 0, DDLOCK_DISCARDCONTENTS,
													   0, &vb, &vbmem, &fvf, &num_verts ) ) ) return GR_OK;

		vbuffer_manager->copy_vertex_data( vbmem, fvf, Vertices );
		VBIterator vbi( vbmem, fvf, Vertices->num_vertices );
		texgen_sphereMap( context, vbi );
		render_pipeline->unlock_vertex_buffer( vb );

		// set second texture states
		render_pipeline->set_render_state( D3DRS_LIGHTING, FALSE );
		render_pipeline->set_render_state( D3DRS_ZFUNC , D3DCMP_LESSEQUAL );
		render_pipeline->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
		render_pipeline->set_render_state( D3DRS_SRCBLEND, D3DBLEND_ONE );
		render_pipeline->set_render_state( D3DRS_DESTBLEND, D3DBLEND_ONE );

		render_pipeline->set_texture_stage_texture( 0, Textures[ 1 ].frame.rp_texture_id );

		render_pipeline->draw_indexed_primitive_vb( PrimitiveType, vb, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );

//		apply();
		vbuffer_manager->release_vertex_buffer( vb );
	}

	return GR_OK;
}

/*-----------------------------------------------------------------------------
	Reflection Gloss
=============================================================================*/
struct ReflectionGlossMaterial : public BaseMaterial
{
public:		// public interface

	ReflectionGlossMaterial() : BaseMaterial(){};
	virtual ~ReflectionGlossMaterial(){};

	GENRESULT COMAPI load_from_filesystem( IFileSystem *IFS );
	GENRESULT COMAPI render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) ;
	GENRESULT COMAPI apply( void );

protected:
};

DECLARE_MATERIAL( ReflectionGlossMaterial, IS_SIMPLE );

//-----------------------------------------------------------------------------
GENRESULT ReflectionGlossMaterial::load_from_filesystem( IFileSystem *IFS )
{
	GENRESULT result = BaseMaterial::load_from_filesystem( IFS );
	
	//#### hack!!
	TexInfo info;
	strcpy( info.name, TEST_TEXNAME );
	info.flags = 0;
	Textures.push_back( info );

	return result;
}

//-----------------------------------------------------------------------------
GENRESULT ReflectionGlossMaterial::apply( void )
{
	// render function takes care of this...

	d3d_material.diffuse.a = 0.5f;//####Test
	return GR_OK;
}

//-----------------------------------------------------------------------------
GENRESULT ReflectionGlossMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
{
	IRP_VERTEXBUFFERHANDLE	vb;
	void										*vbmem;
	U32											num_verts;
	U32											fvf;

	BOOL multiPass = m_numPass >= 2;
	
	//#### for testing
	multiPass = FALSE;

	if ( multiPass )
	{
	}
	else
	{
		// First Draw reflection mapped texture.
		render_pipeline->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
		render_pipeline->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

		render_pipeline->set_render_state( D3DRS_LIGHTING, FALSE );
		render_pipeline->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );
		render_pipeline->set_texture_stage_texture( 0, Textures[ 1 ].frame.rp_texture_id );

		fvf = ( Vertices->vertex_format & (~D3DFVF_TEXCOUNT_MASK) ) | D3DFVF_TEX1;	// needs one texcoord
		if( FAILED( vbuffer_manager->acquire_vertex_buffer( fvf, Vertices->num_vertices, 0, DDLOCK_DISCARDCONTENTS,
													   0, &vb, &vbmem, &fvf, &num_verts ) ) ) return GR_OK;

		vbuffer_manager->copy_vertex_data( vbmem, fvf, Vertices );
		VBIterator vbi( vbmem, fvf, Vertices->num_vertices );
		texgen_sphereMap( context, vbi );
		render_pipeline->unlock_vertex_buffer( vb );

		render_pipeline->draw_indexed_primitive_vb( PrimitiveType, vb, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );
		vbuffer_manager->release_vertex_buffer( vb );

		// Then draw gloss map data
		render_pipeline->set_render_state( D3DRS_ZFUNC , D3DCMP_LESSEQUAL  );
		BaseMaterial::apply();
		render_pipeline->set_render_state( D3DRS_SRCBLEND, D3DBLEND_ONE );						// This is the key point!!
		render_pipeline->set_render_state( D3DRS_DESTBLEND, D3DBLEND_SRCALPHA );
		render_pipeline->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
		render_pipeline->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

		BaseMaterial::render( context, PrimitiveType, Vertices, StartVertex, NumVertices,
														FaceIndices, NumFaceIndices, im_rf_flags );

	}

	return GR_OK;
}







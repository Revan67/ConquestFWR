/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	TwoSidedMaterial.cpp

	MAR.9 2000 Written by Yuichi Ito

=============================================================================*/
#define __TWOSIDEDMATERIAL_CPP

#include "BaseMaterial.h"

/*-----------------------------------------------------------------------------
	TwoSided material
=============================================================================*/
struct TwoSidedMaterial : public BaseMaterial
{
public:		// public interface

	TwoSidedMaterial() : BaseMaterial(){};
	virtual ~TwoSidedMaterial(){};

	GENRESULT COMAPI render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) ;

};

DECLARE_MATERIAL( TwoSidedMaterial, IS_SIMPLE );
DECLARE_MATERIAL_WITH_IMPL( DcDtTwoMaterial,		TwoSidedMaterial, IS_SIMPLE );
DECLARE_MATERIAL_WITH_IMPL( DcDtOcTwoMaterial,	TwoSidedMaterial, IS_SIMPLE );
DECLARE_MATERIAL_WITH_IMPL( DcDtOcOtTwoMaterial,TwoSidedMaterial, IS_SIMPLE );
DECLARE_MATERIAL_WITH_IMPL( DcDtEcTwoMaterial,	TwoSidedMaterial, IS_SIMPLE );

//-----------------------------------------------------------------------------
GENRESULT TwoSidedMaterial::render( MaterialContext *context, D3DPRIMITIVETYPE PrimitiveType, VertexBufferDesc *Vertices, U32 StartVertex, U32 NumVertices, U16 *FaceIndices, U32 NumFaceIndices, U32 im_rf_flags ) 
{
	IRP_VERTEXBUFFERHANDLE	vb;
	void										*vbmem;
	U32											num_verts;
	U32 fvf = Vertices->vertex_format;

	// first. Draw normaly
	DWORD oldZEnable;
	render_pipeline->get_render_state( D3DRS_ZWRITEENABLE, &oldZEnable );
	render_pipeline->set_render_state( D3DRS_ZWRITEENABLE, !IsUseAlpha() );

	BaseMaterial::render( context, PrimitiveType, Vertices, StartVertex, NumVertices,
													FaceIndices, NumFaceIndices, im_rf_flags );

	// second. change normal & change clock wise
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

	// Change Normal
	int offsetNormal;
	int dataSize = analyzeVFV( fvf, &offsetNormal, NULL );
	Vector	*nrmlPtr	= (Vector*)( (U32)vbmem + offsetNormal );
	for ( U32 i = 0; i < Vertices->num_vertices; ++i  )
	{
		*nrmlPtr = nrmlPtr->negative();
		nrmlPtr = (Vector*)( (U32)nrmlPtr + dataSize );
	}
	render_pipeline->unlock_vertex_buffer( vb );

	// change cull mode
	DWORD culMode;
	static DWORD tbl[] = 
	{
    D3DCULL_NONE,
    D3DCULL_CCW,
    D3DCULL_CW
	};

	render_pipeline->get_render_state( D3DRS_CULLMODE, &culMode );
	render_pipeline->set_render_state( D3DRS_CULLMODE, tbl[ culMode - 1 ] );
	render_pipeline->draw_indexed_primitive_vb(	PrimitiveType, vb, StartVertex, NumVertices, FaceIndices, NumFaceIndices, 0 );
	vbuffer_manager->release_vertex_buffer( vb );

	// restore cull mode
	render_pipeline->set_render_state( D3DRS_CULLMODE, culMode );
	render_pipeline->set_render_state( D3DRS_ZWRITEENABLE, oldZEnable );

	return GR_OK;
}


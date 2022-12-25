/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	BaseMaterial.h

	MAR.15 2000 Written by Yuichi Ito

=============================================================================*/
#ifndef __BASEMATERIAL_H
#define __BASEMATERIAL_H

#include "Materials.h"
#include <3dmath.h>
#include <fvf.h>
#include <vector>

//-----------------------------------------------------------------------------
#ifndef __BASEMATERIAL_CPP
extern const char *CLSID_BaseMaterial;
extern const char *CLSID_TwoSidedMaterial;
extern const char *CLSID_DcDtTwoMaterial;
extern const char *CLSID_DcDtOcTwoMaterial;
extern const char *CLSID_DcDtOcOtTwoMaterial;
extern const char *CLSID_DcDtEcTwoMaterial;
extern const char *CLSID_SpecularMaterial;
extern const char *CLSID_ReflectionMaterial;
extern const char *CLSID_SpecularGlossMaterial;
extern const char *CLSID_ReflectionGlossMaterial;
extern const char *CLSID_EmbossBumpMaterial;

extern const char *CLSID_ProceduralMaterial;

#endif // __BASEMATERIAL_CPP

/*-----------------------------------------------------------------------------
	TexInfo class
=============================================================================*/
class TexInfo
{
public:
	TexInfo(){ init(); }
	TexInfo( const TexInfo &a ){ *this = a; }

	char									name[MAX_PATH];
	ITL_TEXTURE_REF_ID		ref_id;
	ITL_TEXTUREFRAME_IRP	frame;
	U32										flags;

	void operator=( const TexInfo &a );

	void init();
	ITL_TEXTURE_REF_ID addRef( ITextureLibrary *texture_library );
	GENRESULT set_texture( ITextureLibrary *texture_library, ITL_TEXTURE_REF_ID trid );
	void cleanup( ITextureLibrary *texture_library );
	GENRESULT load( int stage, IFileSystem *IFS );
	GENRESULT verify( ITextureLibrary *texture_library );
	GENRESULT update( ITextureLibrary *texture_library, float dt );

};

typedef std::vector<TexInfo> TexInfoArray;

/*-----------------------------------------------------------------------------
	TwoSided material
=============================================================================*/
struct BaseMaterial : public IMaterial, public IMaterialProperties
{
	BEGIN_DACOM_MAP_INBOUND(BaseMaterial)
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

	BaseMaterial();
	virtual ~BaseMaterial();

	GENRESULT init( DACOMDESC *desc );


	//-----------------------------------------------------------------------------
	typedef struct
	{
	public:
		float u, v;
		inline void set( float _u, float _v ){ u = _u; v = _v; }
	} TexCoord;

	class VBIterator
	{
	public:
		VBIterator( void *vbmem, U32 fvf, int size )
		{
			nodeSize = FVF_SIZEOF_POSITION( fvf );
			if ( fvf & D3DFVF_NORMAL ) ofstNormal = nodeSize;
			nodeSize += FVF_SIZEOF_OTHER( fvf ) + FVF_SIZEOF_TEXCOORDS( fvf );
			ofstUV = FVF_TEXCOORD_U0_OFS( fvf );

			current = vbmem;
			endPtr  = (void*)( (U32)current + nodeSize * size );
		}

		inline void operator++(){ current = (void*)( (U32)current + nodeSize ); }
		inline void operator--(){ current = (void*)( (U32)current - nodeSize ); }
		inline BOOL isEnd() const { return current == endPtr; }

		inline Vector		&vertex() const { return *(Vector*)( current ); }
		inline Vector		&normal() const { return *(Vector*)( (U32)current + ofstNormal ); }
		inline TexCoord	&uv() const { return *(TexCoord*)( (U32)current + ofstUV ); };
		inline TexCoord	&uv( int idx ) const { return *(TexCoord*)( (U32)current + ofstUV + idx * sizeof( TexCoord ) ); }

	protected:
		int nodeSize;
		int	ofstNormal;
		int ofstUV;
		void *current;
		void *endPtr;
	};


protected:	// protected interface

	virtual GENRESULT cleanup( void );
	inline BOOL IsUseAlpha() const { return Type >= _mtAlpha; }

	// help methods
	void getLightVector( const MaterialContext *context, Vector &v );			// It returns light vector in object(local) world coordinate
	void getEyeVector( const MaterialContext *context, Vector &v );				// It returns light vector in object(local) world coordinate


	static analyzeVFV( U32 fvf, int *offsetNormal, int *offsetTexCoord );

	static int getTexCnt( U32 flag );

protected:	// protected data

	enum MatType
	{
		_mtNoAlpha,
			mtDcDtTwo,
			mtDcDtEcTwo,
			mtSpecular,
			mtReflection,
		_mtAlpha,
			mtDcDtOcTwo,
			mtDcDtOcOtTwo,
			mtSpecularGloss,
			mtReflectionGloss
	};

	MatType								Type;

	U32										m_numPass;
	float									m_detail;

	IDAComponent					*system_services;
	ITextureLibrary				*texture_library;
	IRenderPipeline				*render_pipeline;
	IVertexBufferManager	*vbuffer_manager;

	char									material_name[ IM_MAX_NAME_LEN ];
	D3DMATERIAL7					d3d_material;
	TexInfoArray					Textures;

};

//-----------------------------------------------------------------------------
inline void BaseMaterial::getEyeVector( const MaterialContext *context, Vector &v )
{
	v = context->object_to_view->inverse_rotate( Vector( 0.0f, 0.0f, 1.0f ) );
	v.normalize();
}

inline int BaseMaterial::getTexCnt( U32 flag )
{
	return ( flag & D3DFVF_TEXCOUNT_MASK ) >> D3DFVF_TEXCOUNT_SHIFT;
}


/*-----------------------------------------------------------------------------
	prottype
=============================================================================*/
void texgen_sphereMap( MaterialContext *context, BaseMaterial::VBIterator &it );
void texgen_specularMap( MaterialContext *context, BaseMaterial::VBIterator &it, float vValue );


#endif // __BASEMATERIAL_H

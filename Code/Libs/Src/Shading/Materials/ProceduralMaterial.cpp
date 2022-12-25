/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	ProceduralMaterial.cpp

	MAR.23 2000 Written by Yuichi Ito

=============================================================================*/
#define __PROCEDURALMATERIAL_CPP

#include "BaseMaterial.h"
#include "ProceduralBase.h"

#include "ProceduralNoise.h"
#include "ProceduralSwirl.h"

//-----------------------------------------------------------------------------
#define ARGB1555(r,g,b,a)	\
( \
	( 0x8000 & ( ((unsigned short)(a)) << 8 ) ) | \
	( 0x7c00 & ( ((unsigned short)(r)) << 7 ) ) | \
	( 0x03e0 & ( ((unsigned short)(g)) << 2 ) ) | \
	( 0x001f & ( ((unsigned short)(b)) >> 3 ) ) \
)

#define _TEXW 64
#define _TEXH 64

/*-----------------------------------------------------------------------------
	Procedural Material
=============================================================================*/
struct ProceduralMaterial : public BaseMaterial
{
public:		// public interface

	ProceduralMaterial() : BaseMaterial(), m_hTexture( NULL ), m_offset( 0.0f ), m_procTex( NULL ){};
	virtual ~ProceduralMaterial();

	GENRESULT COMAPI load_from_filesystem( IFileSystem *IFS );
	GENRESULT COMAPI apply( void );
	GENRESULT COMAPI update( float dt );

protected:
	void generateTexture();

	float						m_offset;


	ProceduralBase	*m_procTex;
	U32							m_hTexture;
};

DECLARE_MATERIAL( ProceduralMaterial, IS_SIMPLE );

//-----------------------------------------------------------------------------
ProceduralMaterial::~ProceduralMaterial()
{
	render_pipeline->destroy_texture( m_hTexture );
	delete m_procTex; m_procTex = NULL;
}

//-----------------------------------------------------------------------------
GENRESULT ProceduralMaterial::load_from_filesystem( IFileSystem *IFS )
{
	GENRESULT result = BaseMaterial::load_from_filesystem( IFS );

//	m_procTex = ProceduralBase::create( ProceduralBase::ptNoise );
	m_procTex = ProceduralBase::create( ProceduralBase::ptSwirl );

	//#### test code
	switch ( m_procTex->get_type() )
	{
		case ProceduralBase::ptNoise:
		{
			m_procTex->set_parami( ProceduralNoise::ptNoiseType, ProceduralNoise::ntTurbulence );
			m_procTex->set_paramf( ProceduralNoise::ptSize, 15.0f );
//			m_procTex->set_paramf( ProceduralNoise::ptLow, 0.2f );
			m_procTex->set_paramf( ProceduralNoise::ptLevels, 5.0f );
				

			m_procTex->set_color( 1, Vector( 1.0f, 0.3f, 0.8f ) );
		}
		break;

		case ProceduralBase::ptSwirl:
		{
			m_procTex->set_paramf( ProceduralSwirl::ptContrast, 0.7f );
			m_procTex->set_parami( ProceduralSwirl::ptDetail, 5 );
		}
		break;
	}

	return result;
}

//-----------------------------------------------------------------------------
#define _cnv(a) \
	ARGB1555( (U16)( a.x * 255.0f ), (U16)( a.y * 255.0f ), (U16)( a.z * 255.0f ), 255 )


void ProceduralMaterial::generateTexture()
{
	if ( m_hTexture == NULL )
	{
		PixelFormat fmt;
		fmt.init( PF_RGB5_A1 );
		if ( FAILED( render_pipeline->create_texture( _TEXW, _TEXH, fmt, 0, 0, m_hTexture ) ) ) return;
	}

	if ( m_hTexture == NULL ) return;

	RPLOCKDATA data;
	render_pipeline->lock_texture( m_hTexture, 0, &data );
	char *dstPtr = (char*)data.pixels;

	Vector p, dp;
	dp.set( 0.0f, 0.0f, 1.0f );
	p.set( 0.0f, 0.0f, 0.0f );

	if ( m_procTex->get_dimention() == 2 )
	{
		// 2D Map
		float invW = 1.0f / (float)data.width;
		float invH = 1.0f / (float)data.height;
		for ( float y = 0.0f; y < 1.0f; y += invH )
		{
			for ( float x = 0.0f; x < 1.0f; x += invW, dstPtr += 2 )
			{
				p.x = x; p.y = y;
				Vector c = m_procTex->evaluate( p, dp );
				*((U16*)dstPtr) = _cnv( c );
			}
		}
	}
	else
	{
		for ( float y = 0.0f; y < (float)data.height; y += 1.0f )
		{
			for ( float x = 0.0f; x < (float)data.width; x += 1.0f, dstPtr += 2 )
			{
				p.x = x; p.y = y;
				Vector c = m_procTex->evaluate( p, dp );
				*((U16*)dstPtr) = _cnv( c );
			}
		}
	}

	render_pipeline->unlock_texture( m_hTexture, 0 );
}

//-----------------------------------------------------------------------------
GENRESULT ProceduralMaterial::update( float dt )
{
	//#### test code
	switch ( m_procTex->get_type() )
	{
		case ProceduralBase::ptNoise:
		{
			float pahse;
			m_procTex->get_paramf( ProceduralNoise::ptPhase, pahse );
			pahse += dt;
			m_procTex->set_paramf( ProceduralNoise::ptPhase, pahse );
		}
		break;

		case ProceduralBase::ptSwirl:
		{
			static float param = 0.0f;
			param += dt;
			float t = sinf( param ) * 1.0f;
			m_procTex->set_paramf( ProceduralSwirl::ptTwist, t );
		}
		break;
	}

	generateTexture();

	return BaseMaterial::update( dt );
}

//-----------------------------------------------------------------------------
GENRESULT ProceduralMaterial::apply( void )
{
	GENRESULT result = BaseMaterial::apply();
	render_pipeline->set_render_state( D3DRS_LIGHTING, FALSE );

	render_pipeline->set_texture_stage_texture( 0, m_hTexture );

	return result;
}

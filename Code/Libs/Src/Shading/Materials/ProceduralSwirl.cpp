/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	ProceduralSwirl.cpp

	MAR.24 2000 Written by Yuichi Ito

=============================================================================*/
#define __PROCEDURALSWIRL_CPP

#include "ProceduralSwirl.h"
#include "perlin.h"

#define TWOPI ((float)6.283185307)

//-----------------------------------------------------------------------------
void ProceduralSwirl::init()
{
	// These default paramete came from 3D studio MAX
	m_color[ 0 ].set( 229.0f/255.0f, 148.0f/255.0f, 77.0f/255 );
	m_color[ 1 ].set( 0.0f, 0.0f, 0.0f );

	m_contrast	= 0.4f;
	m_intensity	= 2.0f;
	m_amount		= 1.0f;
	m_twist			= 1.0f;
	m_centerX		= -0.5f;
	m_centerY		= -0.5f;
	m_randomSeed	= 31842.486f;

	m_detail		= 1;
}

//-----------------------------------------------------------------------------
Vector ProceduralSwirl::evaluate( const Vector &p, const Vector&dp ) const
{
	float u = m_centerX + p.x;
	float v = m_centerY + p.y;

  float angle		= m_twist * TWOPI * ( u*u + v*v );
	float sine		= sinf( angle );
	float cosine	= cosf( angle );

	Vector pp( v*cosine - u*sine, v*sine + u*cosine, m_randomSeed );

	// Compute VLfBm
	float l, o, a;
	l = 1.0f;  o = 1.0f;  a = 0.0f;
	for ( int i = 0;  i < m_detail;  ++i )
	{
		Vector ppl( pp.x * l, pp.y * l, pp.z * l );
		a += o * noise3( &ppl.x );
		l *= 2.0f;
		o *= m_contrast;
	}
	float t = ( ( m_amount * m_intensity ) * a );

	Vector vv = lerp( m_color[ 0 ], m_color[ 1 ], t );

	if ( vv.x < 0.0f ) vv.x = 0.0f;
	if ( vv.y < 0.0f ) vv.y = 0.0f;
	if ( vv.z < 0.0f ) vv.z = 0.0f;
	if ( vv.x > 1.0f ) vv.x = 1.0f;
	if ( vv.y > 1.0f ) vv.y = 1.0f;
	if ( vv.z > 1.0f ) vv.z = 1.0f;

	return vv;
}


/*-----------------------------------------------------------------------------
	Parameter access
=============================================================================*/
BOOL32 ProceduralSwirl::get_parami( int paramType, int &value ) const
{
	if ( paramType == ptDetail )
	{
		value = m_detail;
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
BOOL32 ProceduralSwirl::get_paramf( int paramType, float &value ) const
{
	switch ( paramType )
	{
	case ptIntensity: value = m_intensity;	break;
	case ptTwist:			value = m_twist;			break;
	case ptContrast:	value = m_contrast;		break;
	case ptCenterX:		value = m_centerX;		break;
	case ptCenterY:		value = m_centerY;		break;
	case ptAmount:		value = m_amount;			break;
//	case ptDetail:		value = m_Detail;			break;
	default:
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
BOOL32 ProceduralSwirl::get_color( int colorIdx, Vector &color ) const
{
	if ( colorIdx < 2 )
	{
		color = m_color[ colorIdx ];
		return TRUE;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
BOOL32 ProceduralSwirl::set_parami( int paramType, int value )
{
	if ( paramType == ptDetail )
	{
		m_detail = value;
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
BOOL32 ProceduralSwirl::set_paramf( int paramType, float value )
{
	switch ( paramType )
	{
	case ptIntensity: m_intensity = value; break;
	case ptTwist:			m_twist			= value; break;
	case ptContrast:	m_contrast	= value; break;
	case ptCenterX:		m_centerX		= value; break;
	case ptCenterY:		m_centerY		= value; break;
	case ptAmount:		m_amount		= value; break;
	default:
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
BOOL32 ProceduralSwirl::set_color( int colorIdx, const Vector &color )
{
	if ( colorIdx < 2 )
	{
		m_color[ colorIdx ] = color;
		return TRUE;
	}

	return FALSE;
}


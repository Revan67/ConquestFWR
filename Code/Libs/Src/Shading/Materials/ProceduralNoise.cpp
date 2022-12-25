/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	ProceduralNoise.cpp

	MAR.23 2000 Written by Yuichi Ito

Notice:
	This idea came from 3D Studio MAX plug in source.

=============================================================================*/
#define __PROCEDURALNOISE_CPP

#include <stdlib.h>
#include "perlin.h"
#include "ProceduralNoise.h"

#pragma warning( disable : 4514 4701 )

//-----------------------------------------------------------------------------
void clamp( float &v, float _min, float _max )
{
	if ( v < _min )
		v = _min;
	else if ( v > _max )
		v = _max;
}

float threshold( float x, float a, float b )
{
	if ( x < a )
		return 0.0f;
	else if ( x > b )
		return 1.0f;
	return x;
}

inline float logb2(float x) { return float(log(x)/.6931478); }

#define DOAA
#define BLENDBAND 4.0f  // blend to average value over this many orders of magnitude

float ProceduralNoise::m_avgAbsNs = -1.0f;

//-----------------------------------------------------------------------------
void ProceduralNoise::init()
{
	m_color[ 0 ].set( 0.0f, 0.0f, 0.0f );
	m_color[ 1 ].set( 1.0f, 1.0f, 1.0f );

	m_noiseType = ntRegular;
	m_size = 25.0f;
	m_phase = 1.0f;
	m_levels = 3.0f;

	m_low = 0.0f; m_high = 1.0f;

	computeAvgValue();
}

//-----------------------------------------------------------------------------
#define NAVG 10000
		
void ProceduralNoise::computeAvgValue()
{
#ifdef DOAA
	srand(1345);
	Vector p;
	int i;
	float sum = 0.0f;
	m_filter = FALSE;
	for ( i = 0; i < NAVG; i++ )
	{
		p.x = float(rand())/100.0f;
		p.y = float(rand())/100.0f;
		p.z = float(rand())/100.0f;
		sum += noise3DFunction( p, m_levels, 0.0f );
	}
	m_avgValue = sum/float(NAVG);
	sum = 0.0f;

	if ( m_avgAbsNs < 0.0f )
	{
#define NAVGNS 10000
		float phase;
		for (i=0; i<NAVGNS; i++)
		{
			p.x = float(rand())/100.0f;
			p.y = float(rand())/100.0f;
			p.z = float(rand())/100.0f;
			phase = float(rand())/100.0f;
			sum += fabsf( noise4( &p.x, phase ) );
		}
		m_avgAbsNs = sum/float(NAVGNS);
	}
#endif
}

//-----------------------------------------------------------------------------
#define NOISE01(p) ((1.0f+noise4(p,m_phase))*.5f)

//-----------------------------------------------------------------------------
float ProceduralNoise::turbulence( const Vector &p, float lev ) const
{
	float sum = 0.0f;
	float l,f = 1.0f;
	float ml = m_levels;
	Vector pf;
	for ( l = lev; l >= 1.0f; l-=1.0f, ml-=1.0f)
	{
		pf = p;	pf *= f;
		sum += fabsf(noise4( &pf.x,m_phase))/f;
		f *= 2.0f;
	}
	if ( l > 0.0f )
	{
		pf = p;	pf *= f;
		sum += l*fabsf(noise4( &pf.x,m_phase))/f;
	}

#ifdef DOAA
	if ( m_filter && ( ml > l ) )
	{
		float r = 0;
    if (ml<1.0f)
		{
			r += (ml-l)/f;
		}
		else
		{
			r  += (1.0f-l)/f;
			ml -= 1.0f;
			f  *= 2.0f;
			for (l = ml; l >=1.0f; l-=1.0f)
			{
				r += 1.0f/f;
				f *= 2.0f;
			}
			if (l>0.0f)	r+= l/f;
			sum += r*m_avgAbsNs;
		}
	}
#endif
	return sum;
}

//-----------------------------------------------------------------------------
float SmoothThresh( float x, float a, float b, float d )
{
	float al = a-d;  
	float ah = a+d;	
	float bl = b-d;
	float bh = b+d;

	if ( x < al ) return 0.0f;
	if ( x > bh ) return 1.0f;
	if ( x < ah )
	{
		float u = (x-al)/(ah-al);
		u = u*u*(3-2*u);  // m_smooth cubic curve 
		return u*(x-a)/(b-a);
	}
	if (x<bl)	return (x-a)/(b-a);

	float u = (x-bl)/(bh-bl);
	u = u*u*(3-2*u);  // m_smooth cubic curve 
	return (1.0f-u)*(x-a)/(b-a) + u;
}

//-----------------------------------------------------------------------------
float ProceduralNoise::noise3DFunction( const Vector &p, float limitLev, float smWidth ) const
{
	Vector pf;
	float res;
	float lev = limitLev;

	if ( limitLev < ( 1.0f - BLENDBAND ) ) return m_avgValue;

	if ( lev < 1.0f ) lev = 1.0f;

	switch ( m_noiseType )
	{
	case ntTurbulence:
		res = turbulence( p,lev );
		break;

	case ntRegular:
		pf = p;
		res = NOISE01(&pf.x);
		break;

	case ntFractal:
		{
			float sum = 0.0f;
			float l, f = 1.0f;
			for ( l = lev; l >= 1.0f; l -= 1.0f )
			{
				pf = p;	pf *= f;
				sum += noise4( &pf.x, m_phase ) / f;
				f *= 2.0f;
			}
			if ( l > 0.0f )
			{
				pf = p;	pf *= f;
				sum += l*noise4(&pf.x,m_phase)/f;
			}
			res = 0.5f*(sum+1.0f);
		}
	}

	if ( m_low < m_high )
	{
		//res = threshold(res,m_low,m_high);
		res = m_filter? SmoothThresh( res,m_low,m_high, smWidth): threshold(res,m_low,m_high);
	}

	clamp( res, 0.0f, 1.0f );


	if ( m_filter )
	{
		if ( limitLev < 1.0f )
		{
			float u = (limitLev+BLENDBAND-1.0f)/BLENDBAND;
			res = u*res + (1.0f-u)*m_avgValue;
		}
	}

	return res;   
}


//-----------------------------------------------------------------------------
float  ProceduralNoise::limitLevel( const Vector &dp, float &smw ) const
{
#ifdef DOAA
	if ( m_filter )
	{
		float m = (float(fabsf(dp.x) + fabsf(dp.y) + fabsf(dp.z))/3.0f)/m_size;
		float l = logb2(1/m);
		float smWidth = m*.2f;
		if (smWidth>.4f) smWidth = .4f;
		smw = smWidth;
		return  (m_levels<l)?m_levels:l;
	}
#endif 
	smw = 0.0f;
	return m_levels;
}

//-----------------------------------------------------------------------------
Vector ProceduralNoise::evaluate( const Vector &pt, const Vector &dp ) const
{
	Vector p = pt;

	p /= m_size;
	m_filter = FALSE;

	float smw;
	float limlev = limitLevel( dp, smw );
	float d = noise3DFunction( p, limlev, smw );

	return lerp( m_color[ 0 ], m_color[ 1 ], d );
}

/*-----------------------------------------------------------------------------
	Parameter access
=============================================================================*/
BOOL32 ProceduralNoise::get_parami( int paramType, int &value ) const
{
	if ( paramType == ptNoiseType )
	{
		value = m_noiseType;
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
BOOL32 ProceduralNoise::get_paramf( int paramType, float &value ) const
{
	switch ( paramType )
	{
	case ptSize:		value = m_size;		break;
	case ptPhase:		value = m_phase;	break;
	case ptLevels:	value = m_levels; break;
	case ptLow:			value = m_low;		break;
	case ptHigh:		value = m_high;		break;
	default:
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
BOOL32 ProceduralNoise::get_color( int colorIdx, Vector &color ) const
{
	if ( colorIdx < 2 )
	{
		color = m_color[ colorIdx ];
		return TRUE;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
BOOL32 ProceduralNoise::set_parami( int paramType, int value )
{
	if ( paramType == ptNoiseType )
	{
		m_noiseType = value;
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
BOOL32 ProceduralNoise::set_paramf( int paramType, float value )
{
	switch ( paramType )
	{
	case ptSize:		m_size		= value; break;
	case ptPhase:		m_phase		= value; break;
	case ptLevels:	m_levels	= value; break;
	case ptLow:			m_low			= value; break;
	case ptHigh:		m_high		= value; break;
	default:
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
BOOL32 ProceduralNoise::set_color( int colorIdx, const Vector &color )
{
	if ( colorIdx < 2 )
	{
		m_color[ colorIdx ] = color;
		return TRUE;
	}

	return FALSE;
}


/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	ProceduralNoise.h

	MAR.23 2000 Written by Yuichi Ito

Notice:
	This class provides generate noise texture that same implementatinon as
	3D studio MAX R3.
=============================================================================*/
#ifndef __PROCEDURALNOISE_H
#define __PROCEDURALNOISE_H

#include "ProceduralBase.h"

//-----------------------------------------------------------------------------
class ProceduralNoise : public ProceduralBase
{
public:
	enum NoiseType
	{
		ntRegular,
		ntFractal,
		ntTurbulence
	};

	enum ParamType
	{
		ptNoiseType,
		ptSize,
		ptPhase,
		ptLevels,
		ptLow,
		ptHigh
	};

	// Constructor & Destructor
	ProceduralNoise(){ init(); }

	// Drived from ProceduralBase class
	virtual int		 get_dimention() const { return 3; }
	virtual Type	 get_type() const { return ProceduralBase::ptNoise; }

	virtual BOOL32 set_parami( int paramType, int value );
	virtual BOOL32 set_paramf( int paramType, float value );
	virtual BOOL32 set_color( int colorIdx, const Vector &color );

	virtual BOOL32 get_parami( int paramType, int &value ) const; 
	virtual BOOL32 get_paramf( int paramType, float &value ) const;
	virtual BOOL32 get_color( int colorIdx, Vector &color ) const;
	virtual Vector evaluate( const Vector &p, const Vector&dp ) const;

	static void registory()
	{
		ProceduralBase::registory_create_func( ProceduralBase::ptNoise, create_func );
	}

	// this function sets initial value of parameters
	void init();

	// When you change parameters then You should call this function.
//	void updateParameter();


protected:
	static ProceduralBase *create_func(){ return new ProceduralNoise(); }

	void computeAvgValue();

	float turbulence( const Vector &p, float lev ) const;
	float limitLevel( const Vector &dp, float &smw ) const;
	float noise3DFunction( const Vector &p,  float limLev, float smWidth ) const;
	
	static float m_avgAbsNs;

	// properties
	int		m_noiseType;
	float m_size;					// 0.001~1000000.0
	float m_phase;				// -1000~1000
	float m_levels;				// 1~10							This use for Fractal & Turbulance
	float m_low, m_high;	// 0~1
	Vector m_color[ 2 ];

	float m_avgValue;
	mutable BOOL32	m_filter;

private:
};

#endif // __PROCEDURALNOISE_H

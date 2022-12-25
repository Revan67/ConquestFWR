/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	ProceduralSwirl.h

	MAR.24 2000 Written by Yuichi Ito

Notice:
	This class provides generate swirl texture that same implementatinon as
	3D studio MAX R3.

=============================================================================*/
#ifndef __PROCEDURALSWIRL_H
#define __PROCEDURALSWIRL_H

#include "ProceduralBase.h"

//-----------------------------------------------------------------------------
class ProceduralSwirl : public ProceduralBase
{
public:
	enum ParamType
	{
		ptIntensity,
		ptTwist,
		ptContrast,
		ptCenterX,
		ptCenterY,
		ptAmount,
		ptDetail
	};

	// Constructor & Destructor
	ProceduralSwirl(){ init(); }

	// Drived from ProceduralBase class
	virtual int		 get_dimention() const { return 2; }
	virtual Type	 get_type() const { return ProceduralBase::ptSwirl; }

	virtual BOOL32 set_parami( int paramType, int value );
	virtual BOOL32 set_paramf( int paramType, float value );
	virtual BOOL32 set_color( int colorIdx, const Vector &color );

	virtual BOOL32 get_parami( int paramType, int &value ) const; 
	virtual BOOL32 get_paramf( int paramType, float &value ) const;
	virtual BOOL32 get_color( int colorIdx, Vector &color ) const;
	virtual Vector evaluate( const Vector &p, const Vector&dp ) const;

	static void registory()
	{
		ProceduralBase::registory_create_func( ProceduralBase::ptSwirl, create_func );
	}


protected:
	static ProceduralBase *create_func(){ return new ProceduralSwirl(); }

	// properties
	Vector m_color[ 2 ];

	float m_intensity;
	float m_twist;
	float m_contrast;
	float m_centerX;
	float m_centerY;
	float m_amount;
	float	m_randomSeed;

	int		m_detail;

	// this function sets initial value of parameters
	void init();

private:
};


#endif // __PROCEDURALSWIRL_H

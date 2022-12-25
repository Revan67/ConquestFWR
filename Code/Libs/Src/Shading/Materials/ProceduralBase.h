/*-----------------------------------------------------------------------------

	Copyright 2000 Digital anvil

	ProceduralBase.h

	MAR.24 2000 Written by Yuichi Ito

Notice:
	This class is Base class of any kind of Procedual Textures

=============================================================================*/
#ifndef __PROCEDURALBASE_H
#define __PROCEDURALBASE_H

#ifdef  _MSC_VER
#pragma warning( disable: 4512 4710 4100 4786 )
#endif

#include <map>
#include "Vector.h"

//-----------------------------------------------------------------------------
class ProceduralBase
{
public:
	enum Type
	{
		ptNoise,
		ptSwirl
	};

	// static Methods
	static BOOL32 install();
	static void unInstall();
	static ProceduralBase *create( Type type );

	// Constructor & Destructor
	ProceduralBase(){}
	virtual ~ProceduralBase(){}

	// Methods
	virtual Type	 get_type() const = 0;
	virtual int		 get_dimention() const = 0;

	virtual BOOL32 set_parami( int paramType, int value )				= 0;
	virtual BOOL32 set_paramf( int paramType, float value )			= 0;
	virtual BOOL32 set_color( int colorIdx, const Vector &color )			= 0;

	virtual BOOL32 get_parami( int paramType, int &value ) const		= 0;
	virtual BOOL32 get_paramf( int paramType, float &value ) const	= 0;
	virtual BOOL32 get_color( int colorIdx, Vector &color )	const		= 0;


	virtual Vector evaluate( const Vector &p, const Vector&dp ) const = 0;


protected:
	typedef ProceduralBase *(*CreateFunc)();
	typedef std::map<Type, CreateFunc> Container;

	// This function will called by each instances
	static BOOL32 registory_create_func( Type type, CreateFunc func );

	friend class ProceduralNoise;
	friend class ProceduralSwirl;

private:
	static Container m_createFuncs;

};


#endif // __PROCEDURALBASE_H

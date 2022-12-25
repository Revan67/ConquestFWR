// EngineLightInstance.h
//
//
//


#ifndef __EngineLightInstance_h__
#define __EngineLightInstance_h__

//

#include "ILight.h"

#include "BaseLight.h"

#include "EngineLightArchetype.h"

//

struct EngineLightInstance : public ILight
{
public: // Interface

	EngineLightInstance( void );
	~EngineLightInstance( void );
	EngineLightInstance( const EngineLightInstance &eli );
	EngineLightInstance &operator=( const EngineLightInstance &eli );

	bool initialize_from_archetype( EngineLightArchetype *ela, IDAComponent *_system, IEngine *_engine, INSTANCE_INDEX _inst_index );

	// ILight
	GENRESULT COMAPI GetTransform( class Transform &transform ) const ;
	GENRESULT COMAPI GetPosition( class Vector &position ) const ;
	GENRESULT COMAPI GetColor( struct LightRGB &color ) const ;
	GENRESULT COMAPI GetDirection( class Vector &direction ) const ;
	SINGLE COMAPI GetRange( void ) const ;
	BOOL32 COMAPI IsInfinite( void ) const ;
	SINGLE COMAPI GetCutoff( void ) const ;
	U32 COMAPI GetMap( void ) const ;

	// IDAComponent
	GENRESULT COMAPI QueryInterface( const char *iid, void **out_iif );
	U32 COMAPI AddRef( void );
	U32 COMAPI Release( void );


protected: // Data

	INSTANCE_INDEX inst_index;
	IEngine		  *engine;

	U32 eli_f_flags;

	U32 ela_f_flags;
	Vector diffuse_color;
	Vector direction_in_light_space;
	SINGLE range;
	SINGLE cutoff;
};

//

EngineLightInstance::EngineLightInstance()
{
	inst_index = INVALID_INSTANCE_INDEX;
	engine = NULL;

	eli_f_flags = 0;

	ela_f_flags = ELA_F_PARALLEL | ELA_F_INFINITE;
	diffuse_color.set( 1.0f, 1.0f, 1.0f );
	direction_in_light_space.set( 0.0f, 0.0f, 1.0f );
	range = std::numeric_limits<SINGLE>::max();
	cutoff = 0.0f;
}

//

EngineLightInstance::~EngineLightInstance()
{
}

//

EngineLightInstance::EngineLightInstance( const EngineLightInstance &ra )
{
	operator=( ra );
}

//

EngineLightInstance & EngineLightInstance::operator=( const EngineLightInstance &ra )
{
	ela_f_flags = ra.ela_f_flags;

	diffuse_color = ra.diffuse_color ;
	direction_in_light_space = ra.direction_in_light_space;

	range = ra.range ;
	cutoff = ra.cutoff;

	return *this;
}

//

bool EngineLightInstance::initialize_from_archetype( EngineLightArchetype *ela, IDAComponent *_system, IEngine *_engine, INSTANCE_INDEX _inst_index )
{
	ASSERT( ela );

	inst_index = _inst_index;
	engine = _engine;
	eli_f_flags = 0;

/*
	if( !_system || FAILED( _system->QueryInterface( IID_ILightManager, &lightmanager ) ) ) {
		GENERAL_TRACE_1( "EngineLightInstance: initialize_from_archetype: not using light manager\n" );
	}
*/

	LightRGB rgb;

	ela->GetColor( rgb );

	diffuse_color.x = (float)rgb.r / 255.0f;
	diffuse_color.y = (float)rgb.g / 255.0f;
	diffuse_color.z = (float)rgb.b / 255.0f;

	ela->GetDirection( direction_in_light_space );
	range = ela->GetRange();
	cutoff = ela->GetCutoff();

	ela_f_flags = 0;

	if( ela->is_infinite() ) {
		ela_f_flags |= ELA_F_INFINITE;
	}
	else {
		ela_f_flags &= ~(ELA_F_INFINITE);
	}

	if( ela->is_parallel() ) {
		ela_f_flags |= ELA_F_PARALLEL;
	}
	else {
		ela_f_flags &= ~(ELA_F_PARALLEL);
	}


	return true;
}

//

GENRESULT COMAPI EngineLightInstance::GetTransform( class Transform &transform ) const 
{
	transform = engine->get_transform( inst_index );
	return GR_OK;
}

//

GENRESULT COMAPI  EngineLightInstance::GetPosition( class Vector &position ) const 
{
	position = engine->get_position( inst_index );
	return GR_OK;
}

//

GENRESULT COMAPI EngineLightInstance::GetColor( struct LightRGB &color ) const 
{
	color.r = (int)(diffuse_color.x * 255.0f);
	color.g = (int)(diffuse_color.y * 255.0f);
	color.b = (int)(diffuse_color.z * 255.0f);

	return GR_OK;
}

//

GENRESULT COMAPI EngineLightInstance::GetDirection( class Vector &direction ) const 
{
	direction = direction_in_light_space;

	return GR_OK;
}

//

SINGLE COMAPI EngineLightInstance::GetRange( void ) const 
{
	return range;
}

//

BOOL32 COMAPI EngineLightInstance::IsInfinite( void ) const 
{
	return (ela_f_flags & ELA_F_INFINITE);
}

//

SINGLE COMAPI EngineLightInstance::GetCutoff( void ) const 
{
	return cutoff;
}

//

U32 COMAPI EngineLightInstance::GetMap( void ) const 
{
	return 0;
}

//

GENRESULT COMAPI EngineLightInstance::QueryInterface( const char *iid, void **out_iif )
{
	if( strcmp( iid, IID_ILight ) == 0 ) {
		*out_iif = static_cast<ILight*>(this);
		return GR_OK;
	}
	else if( strcmp( iid, IID_IDAComponent ) == 0 ) {
		*out_iif = static_cast<ILight*>(this);
		return GR_OK;
	}

	return GR_GENERIC;
}

//

U32 COMAPI EngineLightInstance::AddRef( void )
{
	return 1;
}

//

U32 COMAPI EngineLightInstance::Release( void )
{
	return 1;
}


#endif // EOF
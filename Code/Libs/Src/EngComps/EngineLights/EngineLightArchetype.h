// RenderArchetype.h
//
//
//


#ifndef __EngineLightArchetype_h__
#define __EngineLightArchetype_h__

//

#include <limits>

#include "FileSys_Utility.h"
#include "ILight.h"

//

#undef max	//
#undef min	//

//

#define ELA_F_PARALLEL	(1<<0)
#define ELA_F_INFINITE	(1<<1)

//

struct EngineLightArchetype : public ILight
{
public: // Interface

	EngineLightArchetype();
	EngineLightArchetype( const EngineLightArchetype &ra );
	~EngineLightArchetype();
	EngineLightArchetype &operator=( const EngineLightArchetype &ra );

	bool load_from_filesystem( IFileSystem *ifs );
	bool is_infinite( void );
	bool is_parallel( void );

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

protected:	// Data
	
	U32 ela_f_flags;
	Vector diffuse_color;
	Vector direction_in_light_space;
	SINGLE range;
	SINGLE cutoff;
};

//

EngineLightArchetype::EngineLightArchetype()
{
	ela_f_flags = ELA_F_PARALLEL | ELA_F_INFINITE;

	diffuse_color.set( 1.0f, 1.0f, 1.0f );
	direction_in_light_space.set( 0.0f, 0.0f, 1.0f );

	range = std::numeric_limits<SINGLE>::max();
	cutoff = 0.0f;
}

//

EngineLightArchetype::~EngineLightArchetype()
{
}

//

EngineLightArchetype::EngineLightArchetype( const EngineLightArchetype &ra )
{
	operator=( ra );
}

//

EngineLightArchetype & EngineLightArchetype::operator=( const EngineLightArchetype &ra )
{
	ela_f_flags = ra.ela_f_flags;

	diffuse_color = ra.diffuse_color ;
	direction_in_light_space = ra.direction_in_light_space;

	range = ra.range ;
	cutoff = ra.cutoff;

	return *this;
}

//

bool EngineLightArchetype::load_from_filesystem( IFileSystem *ifs )
{
	ASSERT( ifs );

	int parallel;

	if( !ifs->SetCurrentDirectory( "Light" ) ) {
		return false;
	}

	ela_f_flags = 0;

	if( FAILED( read_type( ifs, "Range", &range ) ) ) {
		return false;
	}

	read_type( ifs, "Color", &diffuse_color );
	read_type( ifs, "Direction", &direction_in_light_space );
	read_type( ifs, "Cutoff", &cutoff );
	
	if( FAILED( read_type( ifs, "Parallel", &parallel ) ) ) {
		
		// no parallel found in file (old object)

		if( range > 0.0f ) {
			ela_f_flags &= ~(ELA_F_INFINITE);
			ela_f_flags &= ~(ELA_F_PARALLEL);
			cutoff *= 180.0f / PI;
		}
		else {
			range = 0.0f; 
			ela_f_flags |= ELA_F_INFINITE;
			ela_f_flags |= ELA_F_PARALLEL;
			cutoff = 0.0f;
		}
	}
	else if( parallel == 1 ) {
		ela_f_flags |= ELA_F_INFINITE;
		ela_f_flags |= ELA_F_PARALLEL;
		cutoff = 180.0f;
		range = -1.0f;
	}
	else {
		cutoff *= 180.0f / PI;
		ela_f_flags &= ~(ELA_F_INFINITE);
		ela_f_flags &= ~(ELA_F_PARALLEL);
		if( range <= 0.0f ) {
			range = 1E9; // arbitrary large number to simulate non parallel infinite light
		}
	}

	ifs->SetCurrentDirectory( ".." );

	return true;
}

//

bool EngineLightArchetype::is_infinite( void )
{
	return (ela_f_flags & ELA_F_INFINITE) != 0;
}

//

bool EngineLightArchetype::is_parallel( void )
{
	return (ela_f_flags & ELA_F_PARALLEL) != 0;
}

//

GENRESULT COMAPI EngineLightArchetype::GetTransform( class Transform &transform ) const 
{
	transform.set_identity();

	return GR_NOT_IMPLEMENTED;
}

//

GENRESULT COMAPI  EngineLightArchetype::GetPosition( class Vector &position ) const 
{
	position.zero();

	return GR_NOT_IMPLEMENTED;
}

//

GENRESULT COMAPI EngineLightArchetype::GetColor( struct LightRGB &color ) const 
{
	color.r = (int)(diffuse_color.x * 255.0f);
	color.g = (int)(diffuse_color.y * 255.0f);
	color.b = (int)(diffuse_color.z * 255.0f);

	return GR_OK;
}

//

GENRESULT COMAPI EngineLightArchetype::GetDirection( class Vector &direction ) const 
{
	direction = direction_in_light_space;

	return GR_OK;
}

//

SINGLE COMAPI EngineLightArchetype::GetRange( void ) const 
{
	return range;
}

//

BOOL32 COMAPI EngineLightArchetype::IsInfinite( void ) const 
{
	return (ela_f_flags & ELA_F_INFINITE);
}

//

SINGLE COMAPI EngineLightArchetype::GetCutoff( void ) const 
{
	return cutoff;
}

//

U32 COMAPI EngineLightArchetype::GetMap( void ) const 
{
	return 0;
}

//

GENRESULT COMAPI EngineLightArchetype::QueryInterface( const char *iid, void **out_iif )
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

U32 COMAPI EngineLightArchetype::AddRef( void )
{
	return 1;
}

//

U32 COMAPI EngineLightArchetype::Release( void )
{
	return 1;
}

#endif //EOF
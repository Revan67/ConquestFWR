// EngineCameraArchetype.h
//
//
//


#ifndef __EngineCameraArchetype_h__
#define __EngineCameraArchetype_h__

//

#include <limits>

#include "View2d.h"
#include "FileSys_Utility.h"
#include "ICamera.h"

#include "Tfuncs.h"

//

#undef max	//
#undef min	//

//

const long ECA_DEFAULT_WIDTH = 640;
const long ECA_DEFAULT_HEIGHT = 480;

//

struct EngineCameraArchetype : public ICamera
{
public: // Interface

	EngineCameraArchetype();
	EngineCameraArchetype( const EngineCameraArchetype &ra );
	~EngineCameraArchetype();
	EngineCameraArchetype &operator=( const EngineCameraArchetype &ra );

	bool load_from_filesystem( IFileSystem *ifs );

	// ICamera
	const Vector & COMAPI get_position( void ) const ;
	const Transform & COMAPI get_transform( void ) const ;
	Transform COMAPI get_inverse_transform( void ) const ;
	const struct ViewRect * COMAPI get_pane( void  ) const ;
	SINGLE COMAPI get_fovx( void ) const ;
	SINGLE COMAPI get_fovy( void ) const ;
	SINGLE COMAPI get_znear( void ) const ;
	SINGLE COMAPI get_zfar( void ) const ;
	SINGLE COMAPI get_aspect( void ) const ;
	SINGLE COMAPI get_hpc( void ) const ;
	SINGLE COMAPI get_vpc( void ) const ;
	bool COMAPI point_to_screen( float & screen_x, float & screen_y, float & depth, const Vector & world_vector ) const ;
	void COMAPI screen_to_point( Vector & world_vector, float screen_x, float screen_y ) const ;
	vis_state COMAPI object_visibility( const Vector &view_pos, float radius ) const ;

	// IDAComponent
	GENRESULT COMAPI QueryInterface( const char *iid, void **out_iif );
	U32 COMAPI AddRef( void );
	U32 COMAPI Release( void );

protected:	// Data
	
	SINGLE fovx;
	SINGLE fovy;
	SINGLE znear;
	SINGLE zfar;
	SINGLE aspect;

	SINGLE hpc;	
	SINGLE vpc;	

	ViewRect pane;
};

//

EngineCameraArchetype::EngineCameraArchetype()
{
}

//

EngineCameraArchetype::~EngineCameraArchetype()
{
}

//

EngineCameraArchetype::EngineCameraArchetype( const EngineCameraArchetype &ra )
{
	operator=( ra );
}

//

EngineCameraArchetype & EngineCameraArchetype::operator=( const EngineCameraArchetype &ra )
{
	fovx = ra.fovx;
	fovy = ra.fovy;
	znear = ra.znear;
	zfar = ra.zfar;
	aspect = ra.aspect;
	hpc = ra.hpc;
	vpc = ra.vpc;

	pane.x0 = ra.pane.x0;
	pane.y0 = ra.pane.y0;
	pane.x1 = ra.pane.x1;
	pane.y1 = ra.pane.y1;

	return *this;
}

//

bool EngineCameraArchetype::load_from_filesystem( IFileSystem *ifs )
{
	ASSERT( ifs );

	if( !ifs->SetCurrentDirectory( "Camera" ) ) {
		return false;
	}

	if( FAILED( read_type( ifs, "Fovx", &fovx ) ) ) {
		return false;
	}

	read_type( ifs, "Fovy", &fovy ) ;
	read_type( ifs, "Znear", &znear ) ;
	read_type( ifs, "Zfar", &zfar ) ;

	if( FAILED( read_type( ifs, "Pane", &pane ) ) ) {
		pane.x0 = 0;
		pane.y0 = 0;
		pane.x1 = ECA_DEFAULT_WIDTH-1;
		pane.y1 = ECA_DEFAULT_HEIGHT-1;
	}

	//
	// fovx and fovy are in RADIANS
	//

	aspect = tan( fovx ) / tan( fovy );
	float half_near_plane_w = znear * tan( fovx );
	float half_near_plane_h = half_near_plane_w / aspect;
	hpc = (pane.x0 + (pane.x1-pane.x0+1)/2) / half_near_plane_w;
	vpc = -(pane.y0 + (pane.y1-pane.y0+1)/2) / half_near_plane_h;

	fovx = Trad2deg( fovx ) ;
	fovy = Trad2deg( fovy ) ;

	ifs->SetCurrentDirectory( ".." );

	return true;
}

//

const Vector & COMAPI EngineCameraArchetype::get_position( void ) const 
{
	static Vector zero(0,0,0);
	
	return zero;
}

//

const Transform & COMAPI EngineCameraArchetype::get_transform( void ) const 
{
	static Transform I;
	
	return I;
}

//

Transform COMAPI EngineCameraArchetype::get_inverse_transform( void ) const 
{
	return Transform();
}

//

const struct ViewRect * COMAPI EngineCameraArchetype::get_pane( void  ) const 
{
	return &pane;
}

//

SINGLE COMAPI EngineCameraArchetype::get_fovx( void ) const 
{
	return fovx;
}

//

SINGLE COMAPI EngineCameraArchetype::get_fovy( void ) const 
{
	return fovy;
}

//

SINGLE COMAPI EngineCameraArchetype::get_znear( void ) const 
{
	return znear;
}

//

SINGLE COMAPI EngineCameraArchetype::get_zfar( void ) const 
{
	return zfar;
}

//

SINGLE COMAPI EngineCameraArchetype::get_aspect( void ) const 
{
	return aspect;
}

//

SINGLE COMAPI EngineCameraArchetype::get_hpc( void ) const 
{
	return hpc;
}

//

SINGLE COMAPI EngineCameraArchetype::get_vpc( void ) const 
{
	return vpc;
}

//

bool COMAPI EngineCameraArchetype::point_to_screen( float & screen_x, float & screen_y, float & depth, const Vector & world_vector ) const 
{
	screen_x = 0.0f;
	screen_y = 0.0f;
	depth = 1.0f;
	return false;
}

//

void COMAPI EngineCameraArchetype::screen_to_point( Vector & world_vector, float screen_x, float screen_y ) const 
{
	world_vector.zero();
	return ;
}

//

vis_state COMAPI EngineCameraArchetype::object_visibility( const Vector &view_pos, float radius ) const 
{
	return VS_UNKNOWN;
}

//

GENRESULT COMAPI EngineCameraArchetype::QueryInterface( const char *iid, void **out_iif )
{
	if( strcmp( iid, IID_ICamera ) == 0 ) {
		*out_iif = static_cast<ICamera*>(this);
		return GR_OK;
	}
	else if( strcmp( iid, IID_IDAComponent ) == 0 ) {
		*out_iif = static_cast<ICamera*>(this);
		return GR_OK;
	}

	return GR_GENERIC;
}

//

U32 COMAPI EngineCameraArchetype::AddRef( void )
{
	return 1;
}

//

U32 COMAPI EngineCameraArchetype::Release( void )
{
	return 1;
}

#endif //EOF
// EngineCameraInstance.h
//
//
//


#ifndef __EngineCameraInstance_h__
#define __EngineCameraInstance_h__

//

#include "Engine.h"
#include "ICamera.h"

//

#include "EngineCameraArchetype.h"

//

struct EngineCameraInstance : public ICamera
{
public: // Interface

	EngineCameraInstance( void );
	~EngineCameraInstance( void );
	EngineCameraInstance( const EngineCameraInstance &eli );
	EngineCameraInstance &operator=( const EngineCameraInstance &eli );

	bool initialize_from_archetype( EngineCameraArchetype *ela, IDAComponent *_system, IEngine *_engine, INSTANCE_INDEX _inst_index );

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


protected: // Data

	INSTANCE_INDEX inst_index;
	IEngine		  *engine;

	SINGLE fovx;
	SINGLE fovy;
	SINGLE znear;
	SINGLE zfar;
	SINGLE aspect;

	SINGLE hpc;	
	SINGLE vpc;	

	ViewRect pane;

	Vector h_norm;				
	Vector v_norm;				
};

//

EngineCameraInstance::EngineCameraInstance()
{
	inst_index = INVALID_INSTANCE_INDEX;
	engine = NULL;

}

//

EngineCameraInstance::~EngineCameraInstance()
{
	inst_index = INVALID_INSTANCE_INDEX;
	engine = NULL;
}

//

EngineCameraInstance::EngineCameraInstance( const EngineCameraInstance &ra )
{
	operator=( ra );
}

//

EngineCameraInstance & EngineCameraInstance::operator=( const EngineCameraInstance &ra )
{
	inst_index = ra.inst_index;
	engine = ra.engine;

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

	h_norm = ra.h_norm;
	v_norm = ra.v_norm;

	return *this;
}

//

bool EngineCameraInstance::initialize_from_archetype( EngineCameraArchetype *ela, IDAComponent *_system, IEngine *_engine, INSTANCE_INDEX _inst_index )
{
	ASSERT( ela );

	const ViewRect *_pane;

	inst_index = _inst_index;
	engine = _engine;

	fovx = ela->get_fovx();
	fovy = ela->get_fovy();
	znear = ela->get_znear();
	zfar = ela->get_zfar();
	aspect = ela->get_aspect();
	hpc = ela->get_hpc();
	vpc = ela->get_vpc();

	_pane = ela->get_pane();

	pane.x0 = _pane->x0;
	pane.y0 = _pane->y0;
	pane.x1 = _pane->x1;
	pane.y1 = _pane->y1;

	float half_near_plane_w = znear * tan( fovx );
	float half_near_plane_h = half_near_plane_w / aspect;

	h_norm.set( znear, 0, half_near_plane_w );	// points away from view volume
	h_norm.normalize();

	v_norm.set( 0, znear, half_near_plane_h );	// points away from view volume
	v_norm.normalize();

	return true;
}

//

const Vector & COMAPI EngineCameraInstance::get_position( void ) const 
{
	return engine->get_position( inst_index );
}

//

const Transform & COMAPI EngineCameraInstance::get_transform( void ) const 
{
	return engine->get_transform( inst_index );
}

//

Transform COMAPI EngineCameraInstance::get_inverse_transform( void ) const
{
	return engine->get_transform( inst_index ).get_inverse();
}

//

const struct ViewRect * COMAPI EngineCameraInstance::get_pane( void  ) const 
{
	return &pane;
}

//

SINGLE COMAPI EngineCameraInstance::get_fovx( void ) const 
{
	return fovx ;
}

//

SINGLE COMAPI EngineCameraInstance::get_fovy( void ) const 
{
	return fovy ;
}

//

SINGLE COMAPI EngineCameraInstance::get_znear( void ) const 
{
	return znear ;
}

//

SINGLE COMAPI EngineCameraInstance::get_zfar( void ) const 
{
	return zfar ;
}

//

SINGLE COMAPI EngineCameraInstance::get_aspect( void ) const 
{
	return aspect ;
}

//

SINGLE COMAPI EngineCameraInstance::get_hpc( void ) const 
{
	return hpc;
}

//

SINGLE COMAPI EngineCameraInstance::get_vpc( void ) const 
{
	return vpc;
}

//

bool COMAPI EngineCameraInstance::point_to_screen( float & screen_x, float & screen_y, float & depth, const Vector & world_vector ) const 
{
	Vector view_vector;
	
	view_vector = get_transform().inverse_rotate_translate( world_vector );

	if( view_vector.z <= -znear )
	{
		float w = -znear / view_vector.z;
		screen_x = pane.x0 + (pane.x1-pane.x0+1)/2 + view_vector.x * w * hpc;
		screen_y = pane.y0 + (pane.y1-pane.y0+1)/2 + view_vector.y * w * vpc;
		depth = -view_vector.z;
		return true;
	}

	return false;
}

//

void COMAPI EngineCameraInstance::screen_to_point( Vector & world_vector, float screen_x, float screen_y ) const 
{
	float sx = screen_x - (pane.x0 + (pane.x1-pane.x0+1)/2);
	float sy = screen_y - (pane.y0 + (pane.y1-pane.y0+1)/2);

	sx /= hpc;
	sy /= vpc;

	const Transform &camera_to_world = get_transform();

	world_vector = sx * camera_to_world.get_i() + sy * camera_to_world.get_j() - znear * camera_to_world.get_k();

	return ;
}

//

vis_state COMAPI EngineCameraInstance::object_visibility( const Vector &view_pos, float radius ) const 
{
	vis_state result = VS_FULLY_VISIBLE;

	// Check near 
	//
	if( view_pos.z > (radius - znear) ) {
		// Behind near plane
		return VS_NOT_VISIBLE;
	}

	if( !((view_pos.z + radius) < -znear) ) {
		// Intersects near plane
		result = VS_PARTIALLY_VISIBLE;
	}


	// Check far
	//
	if( (view_pos.z + radius) < -zfar ) {
		// Beyond far plane
		return VS_NOT_VISIBLE;
	}

	if( !(view_pos.z > (radius - zfar)) ) {
		// Intersects far plane
		result = VS_PARTIALLY_VISIBLE;
	}

	// Check x
	//
	float rx = (float) fabs(view_pos.x) * h_norm.x + view_pos.z * h_norm.z;
	
	if( rx > radius ) {
		// Outside x plane(s)
		return VS_NOT_VISIBLE;
	}
	
	if( !(rx < -radius) ) {
		// Intersects plane(s)
		result = VS_PARTIALLY_VISIBLE;
	}

	// Check y
	//
	float ry = (float) fabs(view_pos.y) * v_norm.y + view_pos.z * v_norm.z;

	if( ry > radius ) {
		// Outside y plane(s).
		return VS_NOT_VISIBLE;
	}
	
	if( !(ry < -radius) ) {
		// Intersecs y plane(s)
		result = VS_PARTIALLY_VISIBLE;
	}

	// check to see if it's smaller than 1 pixel
	float radius_at_near = ( radius * -znear ) / view_pos.z;

	if( Tpi * radius_at_near * radius_at_near * hpc * -vpc < 1.0f ) {
		return VS_SUB_PIXEL;
	}

	return result;
}

//

GENRESULT COMAPI EngineCameraInstance::QueryInterface( const char *iid, void **out_iif )
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

U32 COMAPI EngineCameraInstance::AddRef( void )
{
	return 1;
}

//

U32 COMAPI EngineCameraInstance::Release( void )
{
	return 1;
}


#endif // EOF
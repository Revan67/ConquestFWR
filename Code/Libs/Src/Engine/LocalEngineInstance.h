#ifndef __LocalEngineInstance_h__
#define __LocalEngineInstance_h__


// ..........................................................................
// 
// LocalEngineInstance
//
// This is the default implementation of IEngineInstance used by the engine
// component when the client does not specify an instance handler.
//

struct LocalEngineInstance : IEngineInstance
{
public:	// Data

	Transform transform;
	Vector velocity, ang_velocity;
	float radius;
	Vector radius_center;

public: // Interface
	
	LocalEngineInstance();
	~LocalEngineInstance();

	// IEngineInstance
	//
	void  COMAPI initialize_instance( INSTANCE_INDEX ) ;
	void  COMAPI create_instance( INSTANCE_INDEX ) ;
	void  COMAPI destroy_instance( INSTANCE_INDEX ) ;
	void  COMAPI set_position( INSTANCE_INDEX , const Vector & position ) ;
	const Vector & COMAPI get_position( INSTANCE_INDEX ) const ;
	void  COMAPI set_orientation( INSTANCE_INDEX , const Matrix & orientation ) ;
	const Matrix & COMAPI get_orientation( INSTANCE_INDEX ) const ;
	void  COMAPI set_transform( INSTANCE_INDEX , const Transform & transform ) ;
	const Transform & COMAPI get_transform( INSTANCE_INDEX )  const ;
	const Vector & COMAPI get_velocity( INSTANCE_INDEX object ) const ;
	const Vector & COMAPI get_angular_velocity( INSTANCE_INDEX object ) const ;
	void COMAPI set_velocity( INSTANCE_INDEX object, const Vector & vel ) ;
	void COMAPI set_angular_velocity( INSTANCE_INDEX object, const Vector & ang ) ;
	void COMAPI get_centered_radius( INSTANCE_INDEX , SINGLE *r, Vector *center ) const ;
	void COMAPI set_centered_radius( INSTANCE_INDEX , const SINGLE r, const Vector & center ) ;
};

//

LocalEngineInstance::LocalEngineInstance() : transform( false )
{
}

//

LocalEngineInstance::~LocalEngineInstance()
{
	// DO NOT CALL destroy_instance
}

//

void COMAPI LocalEngineInstance::initialize_instance( INSTANCE_INDEX )
{
	transform.set_identity();
	velocity.zero();
	ang_velocity.zero();
	radius = 0.0f;
	radius_center.zero();
}

//

void COMAPI LocalEngineInstance::create_instance( INSTANCE_INDEX )
{
}

//

void COMAPI LocalEngineInstance::destroy_instance( INSTANCE_INDEX )
{
}

//

void COMAPI LocalEngineInstance::set_position( INSTANCE_INDEX , const Vector & position )
{
	transform.set_position( position );
}

//

const Vector & COMAPI LocalEngineInstance::get_position( INSTANCE_INDEX ) const
{
	return transform.get_position();
}

//

void COMAPI LocalEngineInstance::set_orientation( INSTANCE_INDEX , const Matrix & orientation )
{
	transform.set_orientation( orientation );
}

//

const Matrix & COMAPI LocalEngineInstance::get_orientation( INSTANCE_INDEX ) const
{
	return transform.get_orientation();
}

//

void COMAPI LocalEngineInstance::set_transform( INSTANCE_INDEX , const Transform & _transform )
{
	transform = _transform;
}

//

const Transform & COMAPI LocalEngineInstance::get_transform( INSTANCE_INDEX ) const
{
	return transform;
}

//

const Vector & COMAPI LocalEngineInstance::get_velocity( INSTANCE_INDEX ) const
{
	return velocity;
}

//

const Vector & COMAPI LocalEngineInstance::get_angular_velocity( INSTANCE_INDEX ) const
{
	return ang_velocity;
}

//

void COMAPI LocalEngineInstance::set_velocity( INSTANCE_INDEX , const Vector & vel )
{
	velocity = vel;
}

//

void COMAPI LocalEngineInstance::set_angular_velocity( INSTANCE_INDEX , const Vector & ang )
{
	ang_velocity = ang;
}

//

void COMAPI LocalEngineInstance::get_centered_radius( INSTANCE_INDEX , float *r, Vector *center ) const
{
	*r = radius;
	*center = radius_center;
}

//

void COMAPI LocalEngineInstance::set_centered_radius( INSTANCE_INDEX , const float r, const Vector & center )
{
	ASSERT(r >= 0.0f);

	radius = r;
	radius_center = center;
}

//

#endif	// EOF

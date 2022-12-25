#ifndef BASELIGHT_H
#define BASELIGHT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                BaseLight.h                               //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/Libs/Include/BaseLight.h 5     4/28/00 11:57p Rmarr $
*/
//---------------------------------------------------------------------------

#ifndef ILIGHT_H
#include "ILight.h"   
#endif

#ifndef TCOMPONENT_H
#include "TComponent.h"
#endif

#ifndef _3DMATH_H
#include "3DMath.h"
#endif

#ifndef ENGINE_H
#include "Engine.h"
#endif

#ifndef SYSTEM_H
#include "System.h"
#endif

#ifndef LIGHTMAN_H
#include "Lightman.h"
#endif

#ifndef TSMARTPOINTER_H
#include "TSmartPointer.h"
#endif

#ifndef HEAPOBJ_H
#include "heapobj.h"
#endif

#ifndef __da_heap_utility_h__
#include "da_heap_utility.h"
#endif

#ifndef FDUMP_H
#include "fdump.h"
#endif

#include <malloc.h>		// for calloc
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// This struct defines the lighting properties that can be saved/loaded from disk.
//
struct LightSaveState
{
	/* define public properties */
	LightRGB color;
	Vector direction;
	SINGLE range;
	BOOL32 infinite;
	SINGLE cutoff;
	BOOL32 on;
	U32 map;
};
//------------------------------------------------------------------------------
//
struct __baseLightImpl : public ILight, LightSaveState
{
	//
	// Interfaces supported
	//
	BEGIN_DACOM_MAP_INBOUND(__baseLightImpl)
	DACOM_INTERFACE_ENTRY(ILight)
	DACOM_INTERFACE_ENTRY2(IID_ILight,ILight)
	END_DACOM_MAP()

	/* ILight methods */
	
	DEFMETHOD(GetColor) (struct LightRGB & _color) const
	{
		_color = color;
		return GR_OK;
	}

	DEFMETHOD(GetDirection) (class Vector & _direction)	const
	{
		_direction = direction;
		return GR_OK;
	}
	
	DEFMETHOD_(SINGLE,GetRange) (void) const 
	{
		return range;
	}

	DEFMETHOD_(BOOL32,IsInfinite) (void) const
	{
		return infinite;
	}

	DEFMETHOD_(SINGLE,GetCutoff) (void) const
	{
		return cutoff;
	}

	DEFMETHOD_(U32,GetMap) (void) const
	{
		return map;
	}

	/* __baseLightImpl methods */

	__baseLightImpl (void)
	{
		on = 0;
	}

	virtual ~__baseLightImpl (void)
	{
	}

	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void operator delete( void *ptr )
	{
		free( ptr );
	}

	DEFMETHOD(set_On) (BOOL32 _on)  
	{
		on = _on;
		return GR_OK;
	}

	DEFMETHOD(get_ILight) (void **ptr)
	{
		if(ptr)
		{
			*ptr = static_cast<ILight *>(this);
			static_cast<ILight *>(this)->AddRef();
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

	DEFMETHOD(get_Index) (S32 * pIndex)
	{
		if(pIndex)
			return GR_GENERIC;
		return GR_INVALID_PARMS;
	}

};
//------------------------------------------------------------------------------
//

struct BaseLight : public DAComponent<__baseLightImpl>, IEngineInstance
{
	/* light local variables */

	INSTANCE_INDEX index;		// engine instance
	Transform transform;
	Vector velocity, ang_velocity;

	COMPTR<IEngine> engine;
	COMPTR<ILightManager> light;

	BaseLight (IEngine * _engine, ISystemContainer * _system)
	{
		index = INVALID_INSTANCE_INDEX;

		ASSERT( _engine != NULL );
		ASSERT( _system != NULL );

		engine = _engine;

		// this archetype busines is just because it's no longer legal to call
		// create_instance() w/ an INVALID_ARCHETYPE_INDEX
		ARCHETYPE_INDEX arch = engine->allocate_archetype( NULL );
		index = engine->create_instance2( arch, this );
		engine->release_archetype( arch );
		
		if( FAILED( _system->QueryInterface( IID_ILightManager, light ) ) ) {
			GENERAL_WARNING( "BaseLight: ctor: unable to acquire IID_ILightManager" );
		}
	}

	virtual ~BaseLight (void)
	{
		if (index != INVALID_INSTANCE_INDEX)
		{
			light->unregister_light( this );		// this must be before engine->destroy_instance()
			engine->destroy_instance( index );
		}
	}

	/* ILight methods */

	DEFMETHOD(GetTransform) (class Transform & _transform) const
	{
		_transform = transform;
		return GR_OK;
	}

	DEFMETHOD(GetPosition) (class Vector & position) const
	{
		position = transform.get_position();
		return GR_OK;
	}

	/* IEngineInstance methods */
protected:
	virtual void   COMAPI initialize_instance (INSTANCE_INDEX index)
	{
	}
	virtual void   COMAPI create_instance (INSTANCE_INDEX index)
	{
	}
	virtual void   COMAPI destroy_instance (INSTANCE_INDEX index)
	{
	}
	virtual void   COMAPI set_position (INSTANCE_INDEX index, const Vector & position)
	{
		transform.set_position(position);
	}
	virtual const Vector & COMAPI get_position (INSTANCE_INDEX index) const
	{
		return transform.get_position();
	}
	virtual void   COMAPI set_orientation (INSTANCE_INDEX index, const Matrix & orientation)
	{
		transform.set_orientation(orientation);
	}
	virtual const Matrix & COMAPI get_orientation (INSTANCE_INDEX index) const
	{
		return transform.get_orientation();
	}
	virtual void      COMAPI set_transform (INSTANCE_INDEX index, const Transform & _transform)
	{
		transform = _transform;
	}
	virtual const Transform & COMAPI get_transform (INSTANCE_INDEX index) const
	{
		return transform;
	}
	virtual const Vector & COMAPI get_velocity (INSTANCE_INDEX object) const
	{
		return velocity;
	}
	virtual const Vector & COMAPI get_angular_velocity (INSTANCE_INDEX object) const
	{
		return ang_velocity;
	}

	virtual void COMAPI set_velocity (INSTANCE_INDEX object, const Vector & vel)
	{
		velocity = vel;
	}
	virtual void COMAPI set_angular_velocity (INSTANCE_INDEX object, const Vector & ang)
	{
		ang_velocity = ang;
	}

	virtual void COMAPI get_centered_radius (INSTANCE_INDEX object, float *radius, Vector *center) const
	{
		*radius = 0.0f;
		center->zero();
	}
	virtual void COMAPI set_centered_radius (INSTANCE_INDEX object, const float r, const Vector & center) {}

	/* BaseLight methods */
public:
	DEFMETHOD(set_On) (BOOL32 _on)  
	{
		if (index != INVALID_INSTANCE_INDEX)
		{
			if (on && _on==0)	// turning off
				light->unregister_light(this);
			else 
			if (on==0 && _on)  // turning on
				light->register_light(this);

			on = _on;
		}
		return GR_OK;
	}

	DEFMETHOD(get_Index) (S32 * pIndex)
	{
		if(pIndex)
		{
			*pIndex = index;
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

	const Vector & get_position (void) const
	{
		return transform.get_position();
	}

	const Transform & get_transform (void) const
	{
		return transform;
	}

	const Matrix & get_orientation (void) const
	{
		return transform.get_orientation();
	}
	
	void set_position (const Vector & position)
	{
		transform.set_position(position);
	}

	void set_orientation (const Matrix & orientation)
	{
		transform.set_orientation(orientation);
	}

	void set_transform (const Transform & _transform)
	{
		transform = _transform;
	}

	void enable (void)
	{
		set_On(1);
	}

	void disable (void)
	{
		set_On(0);
	}

	void zero (void)	// set all properties to zero, except "On"
	{
		BOOL32 bOldOn = on;
		memset(static_cast<LightSaveState *>(this), 0, sizeof(LightSaveState));
		on = bOldOn;
	}

	GENRESULT SetColor(const LightRGB & _color)
	{
		color = _color;
		return GR_OK;
	}

	GENRESULT SetRange(const float _range)
	{
		range = _range;
		return GR_OK;
	}

	GENRESULT SetCutoff(const float _cutoff)
	{
		cutoff = _cutoff;
		return GR_OK;
	}

	GENRESULT SetDirection(const class Vector & _direction)
	{
		direction = _direction;
		return GR_OK;
	}

	GENRESULT SetInfinite(const BOOL32 _infinite)
	{
		infinite = _infinite;
		return GR_OK;
	}

	GENRESULT SetMap(const U32 _map)
	{
		map = _map;
		return GR_OK;
	}
};

#endif	// EOF

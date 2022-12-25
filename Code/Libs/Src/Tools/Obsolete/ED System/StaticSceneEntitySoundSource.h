// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef StaticSceneEntitySoundSource_h
#define StaticSceneEntitySoundSource_h
// --------------------------------------------------------------------------
#include "ISound.h"
// --------------------------------------------------------------------------
namespace ROS
{
class AStaticSceneEntity;
}
// --------------------------------------------------------------------------
class StaticSceneEntitySoundSource: public ISoundSource
{
	public:
		StaticSceneEntitySoundSource(const ROS::AStaticSceneEntity* staticSE, SOUND_ARCH_INDEX arch, U32 start_time)
		: mRefCount(1), mStaticSE(staticSE), mSoundArch(arch), mStartTime(start_time), mAttenuation(0), mMinDistance(1), mMaxDistance(100)
		{
		}

		void set_start_time(U32 time)
		{
			mStartTime = time;
		}

		void set_min_distance(SINGLE distance)
		{
			mMinDistance = distance;
		}

		void set_max_distance(SINGLE distance)
		{
			mMaxDistance = distance;
		}

		void set_attenuation(SINGLE attenuation)
		{
			mAttenuation = attenuation;
		}

		virtual U32 get_start_time()
		{
			return mStartTime;
		}

		virtual bool COMAPI is_on()
		{
			return true;
		}

		virtual bool COMAPI is_3D()
		{
			return (mStaticSE != NULL);
		}

		virtual bool COMAPI is_looping()
		{
			return false;
		}

		virtual SOUND_ARCH_INDEX get_archetype()
		{
			return mSoundArch;
		}

		virtual GENRESULT COMAPI get_attenuation(SINGLE *attenuation)
		{
			*attenuation = mAttenuation;

			return GR_OK;
		}

		virtual GENRESULT COMAPI get_frequency(SINGLE *frequency)
		{
			return GR_NOT_IMPLEMENTED;
		}

	// 2D only properties
		virtual GENRESULT COMAPI get_pan(S32 *)
		{
			return GR_NOT_IMPLEMENTED;
		}

	// 3D only properties
		virtual GENRESULT COMAPI get_position(Vector *position);

		// SOUNDS ACTUALLY HALF IN VOLUME AT TWICE MIN_DISTANCE
		virtual GENRESULT COMAPI get_min_distance(SINGLE *distance)		// sound does not get louder closer to the listener than min 
		{
			*distance = mMinDistance;

			return GR_OK;
		}
		
		virtual GENRESULT COMAPI get_max_distance(SINGLE *distance)		// sound does not get quiter farther than max from the listener
		{
			*distance = mMaxDistance;

			return GR_OK;
		}
		
		virtual GENRESULT COMAPI get_cone_angles(DWORD *insideAngle, DWORD *outsideAngle)
		{
			return GR_NOT_IMPLEMENTED;
		}

		virtual GENRESULT COMAPI get_cone_orientation(Vector *)
		{
			return GR_NOT_IMPLEMENTED;
		}

		virtual GENRESULT COMAPI get_cone_outside_attenuation(SINGLE *)	// attenuation of sound beyond outside cone - within inside cone, attenuation is normal attenuation as set by get_attenuation  between cones the attenuation is scaled between these two volumes
		{
			return GR_NOT_IMPLEMENTED;
		}

		virtual GENRESULT COMAPI get_velocity(Vector *)
		{
			return GR_NOT_IMPLEMENTED;
		}

		// 3D EAX/A3D/Property set extensions
		virtual GENRESULT COMAPI get_reverb_mix(SINGLE *)	// allows control of reverb on an instance basis
		{
			return GR_NOT_IMPLEMENTED;
		}

		virtual GENRESULT COMAPI get_sound_mode(S32 *)		// mode of 3D sound: disable, normal, or headRelative (= absolute values are automatically updated when listener moves)
		{
			return GR_NOT_IMPLEMENTED;
		}

		virtual GENRESULT COMAPI get_apply_mode(S32 *)		// determines when to apply any changes made to properties: immediate or deferred
		{
			return GR_NOT_IMPLEMENTED;
		}

		GENRESULT COMAPI QueryInterface(const char *n, void ** i) 
		{
			return GR_INTERFACE_UNSUPPORTED;
		}

		U32 COMAPI AddRef() 
		{
			return ++mRefCount;
		}

		U32 COMAPI Release() 
		{
			mRefCount--;

			if (mRefCount == 0) 
			{
				delete this;
			}

			return mRefCount;
		}

	private:
		~StaticSceneEntitySoundSource()		// private to prevent direct deletion. Use Release() instead.
		{
		}

		U32								mRefCount;
		const ROS::AStaticSceneEntity*	mStaticSE;
		SOUND_ARCH_INDEX				mSoundArch;
		U32								mStartTime;
		SINGLE							mAttenuation;
		SINGLE							mMinDistance;
		SINGLE							mMaxDistance;
};
// --------------------------------------------------------------------------
#endif
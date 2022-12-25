// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef StaticSceneEntitySoundListener_h
#define StaticSceneEntitySoundListener_h
// --------------------------------------------------------------------------
#include "ISoundListener.h"
// --------------------------------------------------------------------------
class ASoundListener;
// --------------------------------------------------------------------------
class StaticSceneEntitySoundListener: public ISoundListener
{
	public:
		StaticSceneEntitySoundListener(const ASoundListener* targetListener);

		void set_target(const ASoundListener* targetListener)
		{
			mTargetListener = targetListener;
		}

		const ASoundListener* get_target() const
		{
			return mTargetListener;
		}

		virtual GENRESULT COMAPI StaticSceneEntitySoundListener::get_ear_orientation(Vector *front, Vector *up);

		virtual GENRESULT COMAPI StaticSceneEntitySoundListener::get_ear_position(Vector *position);

		virtual GENRESULT COMAPI get_ear_velocity(Vector *velocity)
		{
			return GR_NOT_IMPLEMENTED;
		}

		virtual GENRESULT COMAPI get_ear_distance_factor(SINGLE *distance)
		{
			*distance = 1;

			return GR_OK;
		}

		virtual GENRESULT COMAPI get_ear_doppler_factor(SINGLE *doppler)
		{
			return GR_NOT_IMPLEMENTED;
		}

		virtual GENRESULT COMAPI get_ear_rolloff_factor(SINGLE *rolloff)
		{
			*rolloff = 1;

			return GR_OK;
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
		~StaticSceneEntitySoundListener()			// private to prevent direct deletion. Use Release() instead.
		{
		}

		U32								mRefCount;
		const ASoundListener*			mTargetListener;
};
// --------------------------------------------------------------------------
#endif
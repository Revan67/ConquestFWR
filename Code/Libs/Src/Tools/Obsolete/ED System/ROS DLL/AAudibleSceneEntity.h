// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AAudibleSceneEntity_h
#define AAudibleSceneEntity_h
// --------------------------------------------------------------------------
#include <set>
#include "StringType.h"
#include "TimeType.h"
#include "ROSDLL.h"
#include "ASceneEntity.h"
#include "AudioRole.h"
// --------------------------------------------------------------------------

namespace ROS
{
// --------------------------------------------------------------------------
//  AAudibleSceneEntity
// --------------------------------------------------------------------------
class CPP_DECL AAudibleSceneEntity: public virtual ASceneEntity
{
	friend class ConstAudioStateAccessor;
	friend class AudioStateAccessor;

    public:
		virtual void											Delete() = 0;

        virtual const std::auto_ptr<ConstAudioStateAccessor>	GetConstAudioStateAccessor() const;
        virtual std::auto_ptr<AudioStateAccessor>				GetAudioStateAccessor();

		virtual void											Write(std::ostream& oStream) const;
		virtual void											Read(std::istream& iStream);

    protected:							
    	typedef	std::set<Time> AudioPlayingFlags;

		// --------------------------------------------------------------------------
		// AudioRoleCallback
		// --------------------------------------------------------------------------
		class AudioRoleCallback: public AudioRole::UpdateCallback
		{
			public:
				AudioRoleCallback(AAudibleSceneEntity& audibleSE);

				virtual void RemoveStarted(const AudioRole::UpdateCallback::RoleType& role, Time time);
				virtual void RemoveFinished(const AudioRole::UpdateCallback::RoleType& role, Time time);
				virtual void RemoveStarted(const AudioRole::UpdateCallback::RoleType& role, unsigned int timePointIndex);
				virtual void RemoveFinished(const AudioRole::UpdateCallback::RoleType& role, unsigned int timePointIndex); 

			private:
				void RemoveAudio();

				AAudibleSceneEntity&	mAudibleSE;
				const DAAudioObject*	mLastDAAudioObjectRemoved;
		};

		friend class AudioRoleCallback;

											AAudibleSceneEntity();

		virtual								~AAudibleSceneEntity();
																
        virtual void						Goto(Time time);
																
        virtual void						GotoForAudioRole(Time time);
											
//		virtual void						StateUpdated(Update::ID update);
		virtual void						StateUpdated(Update::ID update, Time time);

		virtual int							GetAudioRoleIndex() const = 0;

		virtual AudioPlayingFlags&			GetAudioPlayingFlags() = 0;
		virtual const AudioPlayingFlags&	GetAudioPlayingFlags() const = 0;

	private:
		typedef ASceneEntity	BaseClass;

		virtual void						StartSound(const ROSString& name, const StringList& descriptionStrings, Time startTimePoint);

		void 								InitAudioPlayingFlags();
        void								SetAudioActuallyPlayingFlag(Time startTime, bool isPlaying);
        bool								IsAudioActuallyPlayingFlagSet(Time startTime) const;

		const DAAudioObject*				GetDAAudioObject(const ROSString& audioName, Time startTime);
		void								PlayDAAudioObject(const DAAudioObject* dAAudioObject, Time startTime);
		void								DestroyAllDAAudioObjects();
		void								DestroyDAAudioObject(const DAAudioObject* dAAudioObject);
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif

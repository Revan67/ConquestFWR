// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AmbientLight_h
#define AmbientLight_h

#include "ALight.h"
#include "LightState.h"
#include "Links.h"
#include "ConstLightStateAccessor.h"
#include "LightStateAccessor.h"
#include "SceneEntityState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	AmbientLight
// --------------------------------------------------------------------------
class AmbientLight: public ALight
{
	public:
		virtual void Delete();

        virtual ROSString GetArchetypeName()  const;
        static ROSString GetAmbientLightArchetypeName();

        virtual void Write(std::ostream& oStream) const;
        virtual void Read(std::istream& iStream);

	protected:
		AmbientLight(const ROSString& name, bool makeNameUnique, Scene& scene, ALightState& lightState);
#if 1
/*****FL GAMESTOCK HACK*********/
		AmbientLight(Scene& scene, ALightState* lightState);
#else
		AmbientLight(Scene& scene, ALightState& lightState);
#endif

        virtual void StateUpdated(Update::ID update, Time time);

		virtual int GetLightRoleIndex() const;

		/*******FOR FL GAMESTOCK*******/
		void SetLightState(ALightState& lightState);

	private:
    	typedef ALight BaseClass;
        
		void InitializeRole();

#if 1
/*****FL GAMESTOCK HACK*********/
	protected:
		virtual SceneEntityState& GetSceneEntityState();

	private:
#else
		virtual SceneEntityState& GetSceneEntityState();
#endif
		virtual const SceneEntityState& GetSceneEntityState() const;
		virtual APhysicalState& GetPhysicalState();
		virtual const APhysicalState& GetPhysicalState() const;

        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

		SceneEntityState			mSceneEntityState;
		AggAPointer<ALightState>	mLightState;
		int							mLightRoleIndex;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
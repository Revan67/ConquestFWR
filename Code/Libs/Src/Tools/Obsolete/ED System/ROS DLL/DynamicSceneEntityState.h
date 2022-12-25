// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DynamicSceneEntityState_h
#define DynamicSceneEntityState_h
// --------------------------------------------------------------------------
#include "ADynamicsState.h"
#include "AStaticsState.h"
#include "StaticsState.h"
#include "DynamicsState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	DynamicSceneEntityState
// --------------------------------------------------------------------------
class DynamicSceneEntityState: public virtual AStaticsState, public virtual ADynamicsState
{
	public:
		DynamicSceneEntityState();

        virtual Position GetPosition() const { return mStaticsState.GetPosition();; }
        virtual void SetPosition(const Position& position) { mStaticsState.SetPosition(position); }

		virtual Force GetForce() const { return mDynamicsState.GetForce(); }
        virtual AngularVelocity GetAngularVelocity() const  { return mDynamicsState.GetAngularVelocity(); }
        virtual Torque GetTorque() const { return mDynamicsState.GetTorque(); }
        virtual LinearVelocity GetLinearVelocity() const { return mDynamicsState.GetLinearVelocity(); }
        virtual void SetForce(const Force& force)  { mDynamicsState.SetForce(force); }
        virtual void SetAngularVelocity(const AngularVelocity& angularVelocity) { mDynamicsState.SetAngularVelocity(angularVelocity); }
        virtual void SetTorque(const Torque& torque) { mDynamicsState.SetTorque(torque); }
        virtual void SetLinearVelocity(const LinearVelocity& linearVelocity) { mDynamicsState.SetLinearVelocity(linearVelocity); }

        virtual void Write(std::ostream& oStream) const;
    	virtual void Read(std::istream& iStream);

	private:
		typedef AStaticsState AStaticsStateBaseClass;
		typedef ADynamicsState ADynamicsStateBaseClass;

    	void WriteSubObject(std::ostream& oStream) const;
    	void ReadSubObject(std::istream& iStream);

		StaticsState	mStaticsState;
		DynamicsState	mDynamicsState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
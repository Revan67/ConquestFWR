// Author: Shaival Varma
//---------------------------------------------------------------------------
#ifndef DynamicsStateAccessor_h
#define DynamicsStateAccessor_h
// --------------------------------------------------------------------------
#include "ROSDLL.h"
#include "Force.h"
#include "AngularVelocity.h"
#include "Torque.h"
#include "LinearVelocity.h"
// --------------------------------------------------------------------------
namespace ROS
{
class ADynamicSceneEntity;
class ADynamicsState;
namespace Update
{
enum ID;
}
// --------------------------------------------------------------------------
//  DynamicsStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL DynamicsStateAccessor
{
    public:
    	DynamicsStateAccessor(ADynamicSceneEntity& owner, ADynamicsState& state);

        Force GetForce() const;
        AngularVelocity GetAngularVelocity() const;
        Torque GetTorque() const;
        LinearVelocity GetLinearVelocity() const;
        void SetForce(const Force& force);
        void SetAngularVelocity(const AngularVelocity& angularVelocity);
        void SetTorque(const Torque& torque);
        void SetLinearVelocity(const LinearVelocity& linearVelocity);

    protected:
    	ADynamicSceneEntity& GetOwner();
    	const ADynamicSceneEntity& GetOwner() const;

    	ADynamicsState& GetState();
    	const ADynamicsState& GetState() const;

		void OwnerStateUpdated(Update::ID update);

    private :
        ADynamicSceneEntity& mOwner;
        ADynamicsState& mState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
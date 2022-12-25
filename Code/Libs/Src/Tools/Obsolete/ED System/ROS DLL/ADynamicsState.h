// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ADynamicsState_h
#define ADynamicsState_h

#include "APhysicalState.h"
#include "Force.h"
#include "Torque.h"
#include "LinearVelocity.h"
#include "AngularVelocity.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  ADynamicsState
// --------------------------------------------------------------------------
class CPP_DECL ADynamicsState: public virtual APhysicalState
{
    public:
        ADynamicsState();

        virtual Force GetForce() const = 0;
        virtual AngularVelocity GetAngularVelocity() const = 0;
        virtual Torque GetTorque() const = 0;
        virtual LinearVelocity GetLinearVelocity() const = 0;
        virtual void SetForce(const Force& kForceR) = 0;
        virtual void SetAngularVelocity(const AngularVelocity& kAngularVelocityR) = 0;
        virtual void SetTorque(const Torque& kTorqueR) = 0;
        virtual void SetLinearVelocity(const LinearVelocity& kLinearVelocityR) = 0;

        virtual void SetDynamicsState(const ADynamicsState& dynamicsState);

		virtual void Interpolate(const ADynamicsState& nextState, float t, ADynamicsState& tState) const;

    protected:

    private:
        typedef APhysicalState BaseClass;
 };
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif

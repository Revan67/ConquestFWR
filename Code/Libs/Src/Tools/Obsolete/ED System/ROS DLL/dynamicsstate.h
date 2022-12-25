// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DynamicsState_h
#define DynamicsState_h
// --------------------------------------------------------------------------
#include "ADynamicsState.h"
#include "ROSDLL.h"
#include "TimeType.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  DynamicsState
// --------------------------------------------------------------------------
class CPP_DECL DynamicsState : public ADynamicsState
{
    public:
        DynamicsState();
        DynamicsState(const ADynamicsState& aDynamicsState);

        virtual Force GetForce() const;
        virtual AngularVelocity GetAngularVelocity() const;
        virtual Torque GetTorque() const;
        virtual LinearVelocity GetLinearVelocity() const;

        virtual void SetForce(const Force& kForceR);
        virtual void SetAngularVelocity(const AngularVelocity& kAngularVelocityR);
        virtual void SetTorque(const Torque& kTorqueR);
        virtual void SetLinearVelocity(const LinearVelocity& kLinearVelocityR);

        virtual void Write(std::ostream& oStream) const;
        virtual void Read(std::istream& iStream);

    private :
        typedef ADynamicsState BaseClass;

        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

        Force mForce;
        Torque mTorque;
        LinearVelocity mLinearVelocity;
        AngularVelocity mAngularVelocity;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif

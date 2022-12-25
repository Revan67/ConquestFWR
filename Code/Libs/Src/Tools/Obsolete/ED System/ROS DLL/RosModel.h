// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Model_h
#define Model_h

#include "Links.h"
#include "ModelNS.h"
#include "ROSDLL.h"
#include "Observer.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//	Model
// --------------------------------------------------------------------------
class CPP_DECL Model
{
	public:
        virtual ~Model() = 0;
        virtual void Detach(Observer& observerR);
        virtual void Attach(Observer& observerR);
        void AcquireObservers(Model& kModelR);

    private :
        /**#: [Cardinalities = "0..n/"]*/
        AssVector<Observer*>	mObserverPV;
        /**# message <Observer> messageObserver_ */
    protected :
        /**# :[Description = "Calls Observer::Update for all attached observers"] */
        void Notify(ModelNS::UpdateID updateID) const;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif

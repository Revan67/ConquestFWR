// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#ifdef _MSC_VER
#include <algorithm>
#else
#include <algorith>
#endif
#include "RosModel.h"
#include "Observer.h"
#include "ModelNS.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
/**# implementation Model:: id(C_0888347351)
*/
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN Model::~Model()
{
}
// --------------------------------------------------------------------------
CPP_DEFN void Model::Attach(Observer& observerR)
{
	mObserverPV.insert(mObserverPV.end(), &observerR);
    observerR.Update(ModelNS::kAll);
}
// --------------------------------------------------------------------------
CPP_DEFN void Model::Detach(Observer& observerR)
{
#if 0
	// STL bug. The remove algorithm doesn't work if the item being removed is the last in the collection
	std::remove(mObserverPV.begin(), mObserverPV.end(), &observerR);
#else
    AssVector<Observer*>::iterator		begin = mObserverPV.begin();
	const AssVector<Observer*>::const_iterator	kEnd = mObserverPV.end();

	while(begin != kEnd)
	{	if(*begin == &observerR)
		{	break;
		}
		
		++begin;
	}

	ASSERT(begin != kEnd);	// If this fires, the entry was not found!

	mObserverPV.erase(begin);
#endif
}
// --------------------------------------------------------------------------
CPP_DEFN void Model::Notify(ModelNS::UpdateID updateID) const
{
	// The observers will be notified in reverse order. This way, if an observer
	// upon recieving a message decides to Detach() itself, it removal will not
	// invalidate the iterators.
	const AssVector<Observer*>::const_iterator	kBegin = mObserverPV.begin();
    AssVector<Observer*>::const_iterator		end = mObserverPV.end();

	while(end != kBegin)
    {  	--end;
		(*end)->Update(updateID);
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void Model::AcquireObservers(Model& kModelR)
{
    AssVector<Observer*>::iterator      		begin = kModelR.mObserverPV.begin();
	const AssVector<Observer*>::const_iterator	kEnd = kModelR.mObserverPV.end();

	while(begin != kEnd)
    {	Attach(*(*begin));
    	++begin;
    }
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------


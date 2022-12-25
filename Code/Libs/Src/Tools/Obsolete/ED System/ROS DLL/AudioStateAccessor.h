// Author: Shaival Varma
//---------------------------------------------------------------------------
#ifndef AudioStateAccessor_h
#define AudioStateAccessor_h
//---------------------------------------------------------------------------
#include "ROSDLL.h"
#include "StringType.h"
#include "StringList.h"
#include "TimeType.h"
//---------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class AAudibleSceneEntity;
// --------------------------------------------------------------------------
//  AudioStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL AudioStateAccessor
{
    public:
    			AudioStateAccessor(AAudibleSceneEntity& owner);

        void	Start(const ROSString& name, const StringList& descriptionStrings, Time startPoint);

	protected:
    	AAudibleSceneEntity& GetOwner();
    	const AAudibleSceneEntity& GetOwner() const;

	private:
		AAudibleSceneEntity&	mOwner;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
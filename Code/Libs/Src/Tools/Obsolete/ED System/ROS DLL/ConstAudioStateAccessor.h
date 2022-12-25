// Author: Shaival Varma
//---------------------------------------------------------------------------
#ifndef ConstAudioStateAccessor_h
#define ConstAudioStateAccessor_h
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
//  ConstAudioStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL ConstAudioStateAccessor
{
    public:
    			ConstAudioStateAccessor(const AAudibleSceneEntity& owner);

        void	Start(const ROSString& name, const StringList& descriptionStrings, Time startPoint);

	protected:
    	const AAudibleSceneEntity& GetOwner() const;

	private:
		const AAudibleSceneEntity&	mOwner;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
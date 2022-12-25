// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ConstCompoundStateAccessor_h
#define ConstCompoundStateAccessor_h
// --------------------------------------------------------------------------
#include "CompoundSceneEntity.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  ConstCompoundStateAccessor
// --------------------------------------------------------------------------
class ConstCompoundStateAccessor
{
    public:
    	ConstCompoundStateAccessor(const CompoundSceneEntity& owner): mOwner(owner){}

        unsigned int    GetCameraCount() const { return mOwner.GetCameraCount(); }
        ROSString       GetCameraName(unsigned int idx) const { return mOwner.GetCameraName(idx); }
        Location        GetCameraLocation(unsigned int idx) const { return mOwner.GetCameraLocation(idx); }
        Orientation     GetCameraOrientation(unsigned int idx) const { return mOwner.GetCameraOrientation(idx); }
        float           GetCameraHorizontalFOV(unsigned int idx) const { return mOwner.GetCameraHorizontalFOV(idx); }
        float           GetCameraVerticalFOV(unsigned int idx) const { return mOwner.GetCameraVerticalFOV(idx); }

    protected:
    	const CompoundSceneEntity& GetOwner() const { return mOwner; };

    private :
        const CompoundSceneEntity& mOwner;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
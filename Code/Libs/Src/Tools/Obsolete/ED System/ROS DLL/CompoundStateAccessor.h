// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef CompoundStateAccessor_h
#define CompoundStateAccessor_h
// --------------------------------------------------------------------------
#include "CompoundSceneEntity.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  ConstCompoundStateAccessor
// --------------------------------------------------------------------------
     /**# :[Note = "Consider a base class AConstCompoundStateAccessor. Don't know if it is really needed."]
    */
class CompoundStateAccessor
{
    public:
    	CompoundStateAccessor(CompoundSceneEntity& owner): mOwner(owner){}

    protected:
    	CompoundSceneEntity& GetOwner() const { return mOwner; };

    private :
        CompoundSceneEntity& mOwner;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
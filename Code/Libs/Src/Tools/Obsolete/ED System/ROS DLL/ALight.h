// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ALight_h
#define ALight_h
// --------------------------------------------------------------------------
#include <iostream>

#include "StringType.h"
#include "ASceneEntity.h"
#include "TimeType.h"
#include "ConstLightStateAccessor.h"
#include "LightStateAccessor.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Scene;
class Color;
// --------------------------------------------------------------------------
//  ALight
// --------------------------------------------------------------------------
class ALight: public virtual ASceneEntity 
{
    public:
		friend class LightStateAccessor;

        virtual ROSString GetArchetypeName() const;
        static ROSString GetALightArchetypeName();

        virtual const std::auto_ptr<ConstLightStateAccessor> GetConstLightStateAccessor() const;
        virtual std::auto_ptr<LightStateAccessor> GetLightStateAccessor();

    protected:
		ALight();

        virtual void Goto(Time time);
        virtual void GotoForLightRole(Time time);

		virtual int GetLightRoleIndex() const = 0;

	private:
        typedef ASceneEntity BaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif

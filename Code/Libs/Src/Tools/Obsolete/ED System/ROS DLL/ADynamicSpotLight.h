// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ADynamicSpotLight_h
#define ADynamicSpotLight_h
// --------------------------------------------------------------------------
#include "ADynamicSceneEntity.h"
#include "ASpotLight.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
class ConstSpotLightStateAccessor;
class SpotLightStateAccessor;
class Scene;
class ARole;
// --------------------------------------------------------------------------
//  ADynamicSpotLight
// --------------------------------------------------------------------------
class CPP_DECL ADynamicSpotLight: public virtual ADynamicSceneEntity, virtual public ASpotLight
{
    public:
    protected:
        ADynamicSpotLight();

        virtual ~ADynamicSpotLight() = 0;

		virtual void Goto(Time time);

		virtual void StateUpdated(Update::ID update, Time time);

    private:
        typedef ADynamicSceneEntity ADynamicSceneEntityBaseClass;
        typedef ASpotLight ASpotLightBaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
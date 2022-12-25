// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ASpotLight_h
#define ASpotLight_h
// --------------------------------------------------------------------------
#include "ALight.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
class ConstSpotLightStateAccessor;
class SpotLightStateAccessor;
class Scene;
class ARole;
// --------------------------------------------------------------------------
//  ASpotLight
// --------------------------------------------------------------------------
class CPP_DECL ASpotLight: public virtual ALight
{
    public:
        virtual const std::auto_ptr<ConstSpotLightStateAccessor> GetConstSpotLightStateAccessor() const;
        virtual std::auto_ptr<SpotLightStateAccessor> GetSpotLightStateAccessor();

    protected:
        ASpotLight();

        virtual ~ASpotLight() = 0;

		virtual void Goto(Time time);

		virtual int GetLightRoleIndex() const { return GetSpotLightRoleIndex(); }

		virtual int GetSpotLightRoleIndex() const = 0;

    private:
        typedef ALight BaseClass;

		void GotoForSpotLightRole(Time time);
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
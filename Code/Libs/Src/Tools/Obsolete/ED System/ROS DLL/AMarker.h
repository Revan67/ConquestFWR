// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AMarker_h
#define AMarker_h
// --------------------------------------------------------------------------
#include "AStaticSceneEntity.h"
#include "StaticsState.h"
#include "TimeType.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  AMarker
// --------------------------------------------------------------------------
class CPP_DECL AMarker: public AStaticSceneEntity
{
    public:
        AMarker(const ROSString& name, bool makeNameUnique, Scene& scene);

    protected:
		AMarker();

		virtual void Render(const ROS::DABaseCamera* camera) const;

    private:
        typedef AStaticSceneEntity BaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
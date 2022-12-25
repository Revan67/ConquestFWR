// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ADynamicSceneEntity_h
#define ADynamicSceneEntity_h
// --------------------------------------------------------------------------
#include "AStaticSceneEntity.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
class ConstDynamicsStateAccessor;
class DynamicsStateAccessor;
class Scene;
class ARole;
// --------------------------------------------------------------------------
//  ADynamicSceneEntity
// --------------------------------------------------------------------------
class CPP_DECL ADynamicSceneEntity: public virtual AStaticSceneEntity
{
	friend class DynamicsStateAccessor;

    public:
        virtual const std::auto_ptr<ConstDynamicsStateAccessor> GetConstDynamicsStateAccessor() const;
        virtual std::auto_ptr<DynamicsStateAccessor> GetDynamicsStateAccessor();

    protected:
        ADynamicSceneEntity();

        virtual ~ADynamicSceneEntity() = 0;

		virtual void Goto(Time time);
		virtual void GotoForDynamicRole(Time time);

		virtual void StateUpdated(Update::ID id);
		virtual void StateUpdated(Update::ID id, Time time);

		virtual void DynamicStateUpdated(Time time);

		virtual int GetDynamicRoleIndex() const = 0;

    private:
        typedef AStaticSceneEntity BaseClass;
};
// --------------------------------------------------------------------------
}
#endif
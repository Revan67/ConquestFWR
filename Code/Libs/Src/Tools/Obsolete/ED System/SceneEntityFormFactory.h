// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SceneEntityFormType_h
#define SceneEntityFormType_h

#include "SceneEntityControls.h"
// --------------------------------------------------------------------------
template<class TBaseType>
class ASceneEntityFormFactory
{
    public:
        virtual bool Matches(const TBaseType& kObjR) const = 0;
        virtual TBaseType* Manufacture(CWnd* Owner, const TSceneEntityForm::CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity) const = 0;
};
// --------------------------------------------------------------------------
template<class TBaseType, class TDescendantType>
class SceneEntityFormFactory: public ASceneEntityFormFactory<TBaseType>
{
    public:
        virtual bool Matches(const TBaseType& kObjR) const
        {
            return dynamic_cast<const TDescendantType*>(&kObjR) != NULL;
        }
        virtual TBaseType* Manufacture(CWnd* Owner, const TSceneEntityForm::CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity) const
        {
        	return new TDescendantType(Owner, callback, sceneEntity);
        }
};
// --------------------------------------------------------------------------
#endif
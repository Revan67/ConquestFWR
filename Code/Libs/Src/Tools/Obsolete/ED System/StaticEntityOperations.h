// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef StaticEntityOperations_h
#define StaticEntityOperations_h
// --------------------------------------------------------------------------
#include "AOperation.h"
#include "AStaticSceneEntity.h"
#include "View.h"
#include "OrientationMemento.h"
#include "ConstStaticsStateAccessor.h"
#include "StaticsStateAccessor.h"
// --------------------------------------------------------------------------
class StaticEntityOrientationChange: public ROS::AOperation
{
	public:
		StaticEntityOrientationChange(ROS::AStaticSceneEntity& entity, View& view)
		: BaseClass("Orientation Change", &entity), mView(view)
		, mMemento(entity.GetConstStaticsStateAccessor()->GetOrientationMemento(view.GetCurrentSceneTime()))
		{
		}

		virtual ROS::AOperation* Perform()
		{
			ROS::AStaticSceneEntity*	entity = dynamic_cast<ROS::AStaticSceneEntity*>(GetEntity());
				
			// Use the private constructor to create inverse.
			StaticEntityOrientationChange* inverse = new StaticEntityOrientationChange(*entity, mView, mMemento.GetTime());

			entity->GetStaticsStateAccessor()->SetOrientationMemento(mMemento);

			mView.SetSecondarySceneEntity(entity);
			mView.SecondarySceneEntityUpdated();

			return inverse;
		}

	private:
		typedef ROS::AOperation BaseClass;

		StaticEntityOrientationChange(ROS::AStaticSceneEntity& entity, View& view, ROS::Time time)
		: BaseClass("Orientation Change", &entity), mView(view)
		, mMemento(entity.GetConstStaticsStateAccessor()->GetOrientationMemento(time))
		{
		}

		ROS::OrientationMemento		mMemento;
		View&						mView;
};
// --------------------------------------------------------------------------
class StaticEntityLocationChange: public ROS::AOperation
{
	public:
		StaticEntityLocationChange(ROS::AStaticSceneEntity& entity, View& view)
		: BaseClass("Location Change", &entity), mView(view)
		, mMemento(entity.GetConstStaticsStateAccessor()->GetLocationMemento(view.GetCurrentSceneTime()))
		{
		}

		virtual ROS::AOperation* Perform()
		{
			ROS::AStaticSceneEntity*	entity = dynamic_cast<ROS::AStaticSceneEntity*>(GetEntity());
				
			// Use the private constructor to create inverse.
			StaticEntityLocationChange* inverse = new StaticEntityLocationChange(*entity, mView, mMemento.GetTime());

			entity->GetStaticsStateAccessor()->SetLocationMemento(mMemento);

			mView.SetSecondarySceneEntity(entity);
			mView.SecondarySceneEntityUpdated();

			return inverse;
		}

	private:
		typedef ROS::AOperation BaseClass;

		StaticEntityLocationChange(ROS::AStaticSceneEntity& entity, View& view, ROS::Time time)
		: BaseClass("Orientation Change", &entity), mView(view)
		, mMemento(entity.GetConstStaticsStateAccessor()->GetLocationMemento(time))
		{
		}

		ROS::LocationMemento		mMemento;
		View&						mView;
};
// --------------------------------------------------------------------------
#endif
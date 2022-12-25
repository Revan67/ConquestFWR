// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "LocationKeyPointMarker.h"
#include "LocationKeyPointStaticsState.h"
#include "GLUtils.h"
#include "RPUL.h"
#include "DARenderPipeline.h"
#include "ConstStaticsStateAccessor.h"
#include "SceneEntityStateAccessor.h"
#include "Spline.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN LocationKeyPointMarker::LocationKeyPointMarker(const ROSString& name, bool makeNameUnique, Scene& scene, AStaticSceneEntity& entity, unsigned int keyPointIndex)
:BaseClass(name, makeNameUnique, scene, *new LocationKeyPointStaticsState(entity, keyPointIndex)), mBaseName(name)
{
	SetName();

	entity.GetSceneEntityStateAccessor()->AddListener(*this);	// So that this object gets informed when the entity is deleted
	GetSceneEntityState().AddSource(entity);
}
// --------------------------------------------------------------------------
CPP_DEFN LocationKeyPointMarker::~LocationKeyPointMarker()
{
	// Decrement the key point indices of the LocationKeyPointMarker instances that follow
	Marker*	marker = GetNextMarker();

	while(marker != NULL)
	{
		LocationKeyPointMarker*	lMarker = dynamic_cast<LocationKeyPointMarker*>(marker);
		ASSERT(lMarker);

		lMarker->SetKeyPointIndex(lMarker->GetKeyPointIndex() - 1);

		marker = lMarker->GetNextMarker();
	}
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int LocationKeyPointMarker::GetKeyPointIndex() const
{
	const APhysicalState*				pState = &GetPhysicalState();
	const LocationKeyPointStaticsState*	lState = dynamic_cast<const LocationKeyPointStaticsState*>(pState);
	ASSERT(lState);

	return lState->GetKeyPointIndex();
}
// --------------------------------------------------------------------------
CPP_DEFN void LocationKeyPointMarker::SetKeyPointIndex(unsigned int keyPointIndex)
{
	APhysicalState*					pState = &GetPhysicalState();
	LocationKeyPointStaticsState*	lState = dynamic_cast<LocationKeyPointStaticsState*>(pState);
	ASSERT(lState);

	lState->SetKeyPointIndex(keyPointIndex);

	SetName();
}
// --------------------------------------------------------------------------
CPP_DEFN void LocationKeyPointMarker::RenderMarker(const ROS::DABaseCamera* camera) const
{
	const LocationKeyPointStaticsState*	locationState = dynamic_cast<const LocationKeyPointStaticsState*>(&GetPhysicalState());
	ASSERT(locationState);

	const LocationKeyPointStaticsState::InterpolationType	type = locationState->GetInterpolationType();

	if(type == LocationKeyPointStaticsState::kLinearFixed || type == LocationKeyPointStaticsState::kLinearBlend)
	{
		BaseClass::RenderMarker(camera);
	}
	else
	{
		ASSERT(type == LocationKeyPointStaticsState::kSplineFixed || type == LocationKeyPointStaticsState::kSplineBlend);

		float	color[] = {0.0, 1.0, 1.0};

		GL::WireCube(1, color);
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void LocationKeyPointMarker::RenderPathToNextMarker(const ROS::DABaseCamera* camera) const
{
	const LocationKeyPointStaticsState*	locationState = dynamic_cast<const LocationKeyPointStaticsState*>(&GetPhysicalState());
	ASSERT(locationState);

	const LocationKeyPointStaticsState::InterpolationType	type = locationState->GetInterpolationType();

	if(type == LocationKeyPointStaticsState::kLinearFixed || type == LocationKeyPointStaticsState::kLinearBlend)
	{
		BaseClass::RenderPathToNextMarker(camera);
	}
	else
	{
		ASSERT(type == LocationKeyPointStaticsState::kSplineFixed || type == LocationKeyPointStaticsState::kSplineBlend);

		if(GetNextMarker())
		{
			const Time		time = GetCurrentTimePoint();

			Location	p1 = GetLocation(time);
			Location	p2 = GetNextMarker()->GetConstStaticsStateAccessor()->GetLocation(time);

			Location	p0 = GetPreviousMarker() ? GetPreviousMarker()->GetConstStaticsStateAccessor()->GetLocation(time) : p1;
			Location	p3 = GetNextMarker()->GetNextMarker() ? GetNextMarker()->GetNextMarker()->GetConstStaticsStateAccessor()->GetLocation(time) : p2;

			// Treat p1 as origin
			p0 -= p1;
			p2 -= p1;
			p3 -= p1;
			p1 = Location(0, 0, 0);

			Spline	spline(p0.GetVector(), p1.GetVector(), p2.GetVector(), p3.GetVector());

			// Draw the connecting line.
			ASSERT(PIPE);

			PrimitiveBuilder pb(PIPE);

			pb.Color3f(0, 1, 1);

			pb.Begin(PB_LINE_STRIP);
				spline.first_point();
				pb.Vertex3f	(0, 0, 0);

				float t = 0;

				do
				{
					t += 0.1;

					Vector	iPoint;

					spline.next_point(0.1, iPoint);

					pb.Vertex3f(iPoint.x, iPoint.y, iPoint.z);
				}
				while(t < 1.0);

			pb.End();

#if 0
			Vector	point, tangent;
			Vector	v0 = p0.GetVector();
			Vector	v1 = p1.GetVector();
			Vector	v2 = p2.GetVector();
			Vector	v3 = p3.GetVector();
			
			calculateCRSpline(&v0, &v1, &v2, &v3, 0.5, &point);
			calculateCRSplineTangent(&v0, &v1, &v2, &v3, 0.5, &tangent);

			pb.Color3f(1, 1, 0);

			pb.Begin(GL_LINES);

			pb.Vertex3f(point.x - tangent.x, point.y - tangent.y, point.z - tangent.z);
			pb.Vertex3f(point.x + tangent.x, point.y + tangent.y, point.z + tangent.z);
			
			pb.End();
#endif
		}
	}
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString LocationKeyPointMarker::GetArchetypeName() const
{
	return GetLocationKeyPointMarkerArchetypeName();
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString LocationKeyPointMarker::GetLocationKeyPointMarkerArchetypeName()
{
	return "LocationKeyPointMarker";
}
// --------------------------------------------------------------------------
CPP_DEFN void LocationKeyPointMarker::SetName()
{
	char	indexStr[100];

	GetSceneEntityState().SetName(mBaseName + ROSString(" Location ") + itoa(GetKeyPointIndex() + 1, indexStr, 10));
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include "DARenderPipeline.h"
#include "RPUL.h"
#include "GLUtils.h"
#include "PositionMarker.h"
#include "MatrixUtil.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
enum FieldID
{
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN PositionMarker::PositionMarker(const ROSString& kNameR, bool makeNameUnique, Scene& scene)
: BaseClass(kNameR, makeNameUnique, scene)
{

}
// --------------------------------------------------------------------------
CPP_DEFN PositionMarker::PositionMarker(Scene& scene)
: BaseClass(scene)
{
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString PositionMarker::GetArchetypeName() const
{
    return GetPositionMarkerArchetypeName();
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString PositionMarker::GetPositionMarkerArchetypeName()
{
    return "PositionMarker";
}
// --------------------------------------------------------------------------
bool PositionMarker::FindIntersect(const IntersectInfo& intersectInfo, float* distance) const
{
	return false;
}
// --------------------------------------------------------------------------
CPP_DEFN void PositionMarker::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void PositionMarker::Read(std::istream& iStream)
{
    BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void PositionMarker::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
}
// --------------------------------------------------------------------------
void PositionMarker::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void PositionMarker::Render(const ROS::DABaseCamera* camera) const
{
	// Set up transform to make the cone stand on its tip
	::Matrix	rot;
	Transform	trans;
    Transform	oldModelView, currModelView;
	const float	coneHeight = 2;

	PIPE->get_modelview(oldModelView);

	rot.compose_rotation(X_AXIS, 90);
	trans = Transform(rot, Vector(0.0, coneHeight, 0.0));

	currModelView = oldModelView * trans;

	PIPE->set_modelview(currModelView);

	const float	color[] = {1.0, 1.0, 0.0};

	PIPE->set_render_state(D3DRS_TEXTUREHANDLE, 0);

	GL::SolidCone(0.5, coneHeight, 8, 1, color);

	// restore the model-view
	PIPE->set_modelview(oldModelView);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------


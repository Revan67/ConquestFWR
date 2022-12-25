// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include "GLUtils.h"
#include "Actor.h"
#include "MatrixUtil.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
/**# implementation Actor:: id(C_0886778923)
*/
// --------------------------------------------------------------------------
enum FieldID
{
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN Actor::Actor(const ROSString& kNameR, bool makeNameUnique, Scene& scene)
: BaseClass(kNameR, makeNameUnique, scene)
{

}
// --------------------------------------------------------------------------
CPP_DEFN Actor::Actor(Scene& scene)
: BaseClass(scene)
{
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString Actor::GetArchetypeName() const
{
    return GetActorArchetypeName();
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString Actor::GetActorArchetypeName()
{
    return "Actor";
}
// --------------------------------------------------------------------------
bool Actor::FindIntersect(const IntersectInfo& intersectInfo, float* distance) const
{
	return false;
}
// --------------------------------------------------------------------------
CPP_DEFN void Actor::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void Actor::Read(std::istream& oStream)
{
    BaseClass::Read(oStream);
}
// --------------------------------------------------------------------------
void Actor::WriteSubObject(std::ostream& oStream) const
{
    OStreamWiz<FieldID>	oWiz(oStream);
}
// --------------------------------------------------------------------------
void Actor::ReadSubObject(std::istream& oStream)
{
    IStreamWiz<FieldID>	iWiz(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void Actor::Render(const ROS::DABaseCamera* camera) const
{
#if 0
    const float coneHeight = 2;
    const float sphereRadius = 0.5;

    glPushMatrix();

    glRotatef(-90, 1.0, 0.0, 0.0);
    glutSolidCone(0.5, coneHeight, 4, 1);

    glTranslatef(0.0, 0.0, coneHeight + sphereRadius);
    glutWireSphere(sphereRadius, 8, 4);

    glPopMatrix();
#else
	float	color[] = {0.0, 0.0, 1.0};

	GL::SolidCone(0.5, 2, 4, 1, color);

	GL::WireCube(2, color);
#endif
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------


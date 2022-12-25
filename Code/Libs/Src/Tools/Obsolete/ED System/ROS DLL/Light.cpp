// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include "DARenderPipeline.h"
#include "RPUL.h"
#include "GLUtils.h"
#include "Light.h"
#include "CodeMsg.h"
/**# implementation Light:: id(C_0886789919) 
*/
// --------------------------------------------------------------------------
enum FieldID
{
	kAmbient,
	kDiffuse,
	kSpecular,
	kIsPositionFixed,
	kIsColorFixed
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
Light::Light(const ROSString& kNameR, bool makeNameUnique, Scene& scene)
: BaseClass(kNameR, makeNameUnique, scene)
{

}
// --------------------------------------------------------------------------
Light::Light(Scene& scene)
: BaseClass(scene)
{
}
// --------------------------------------------------------------------------
bool Light::FindIntersect(const IntersectInfo& intersectInfo, float* distance) const
{
	return false;
}
// --------------------------------------------------------------------------
void Light::Write(std::ostream& oStreamR) const
{
    BaseClass::Write(oStreamR);

    WriteSubObject(oStreamR);
}
// --------------------------------------------------------------------------
void Light::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void Light::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

    oWiz.Put(kAmbient, mAmbient);
    oWiz.Put(kDiffuse, mDiffuse);
    oWiz.Put(kSpecular, mSpecular);
    oWiz.Put(kIsPositionFixed, mIsPositionFixed);
    oWiz.Put(kIsColorFixed, mIsColorFixed);
}
// --------------------------------------------------------------------------
void Light::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

    iWiz.Get(kAmbient, mAmbient);
    iWiz.Get(kDiffuse, mDiffuse);
    iWiz.Get(kSpecular, mSpecular);
    iWiz.Get(kIsPositionFixed, mIsPositionFixed);
    iWiz.Get(kIsColorFixed, mIsColorFixed);
}
// --------------------------------------------------------------------------
ROSString Light::GetArchetypeName() const
{
    return GetLightArchetypeName();
}
// --------------------------------------------------------------------------
ROSString Light::GetLightArchetypeName()
{
    return "Light";
}
// --------------------------------------------------------------------------
void Light::Render(const ROS::DABaseCamera* camera) const
{
	ASSERT(PIPE);

	PrimitiveBuilder	pb(PIPE);

	float	solidCubeColor[] = {1.0, 1.0, 1.0};
	float	wireCubeColor[] = {0.0, 1.0, 0.0};

	GL::SolidCube(0.2, solidCubeColor);
    
	GL::WireCube(0.4, wireCubeColor);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------


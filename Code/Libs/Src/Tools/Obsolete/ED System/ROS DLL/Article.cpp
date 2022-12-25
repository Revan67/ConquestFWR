// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include "DARenderPipeline.h"
#include "RPUL.h"
#include "GLUtils.h"
#include "Article.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
/**# implementation Article:: id(C_0886778875)
*/
// --------------------------------------------------------------------------
enum FieldID
{
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
Article::Article(const ROSString& kNameR, bool makeNameUnique, Scene& scene)
:BaseClass(kNameR, makeNameUnique, scene)
{
}
// --------------------------------------------------------------------------
Article::Article(Scene& scene)
:BaseClass(scene)
{
}
// --------------------------------------------------------------------------
ROSString Article::GetArchetypeName()  const
{
    return GetArticleArchetypeName();
}
// --------------------------------------------------------------------------
ROSString Article::GetArticleArchetypeName()
{
    return "Article";
}
// --------------------------------------------------------------------------
bool Article::FindIntersect(const IntersectInfo& intersectInfo, float* distance) const
{
	return false;
}
// --------------------------------------------------------------------------
void Article::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void Article::Read(std::istream& iStream)
{
    BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void Article::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
}
// --------------------------------------------------------------------------
void Article::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
}
// --------------------------------------------------------------------------
void Article::Render(const ROS::DABaseCamera* camera) const
{
#if 0
    glPushMatrix();

    glScalef(2.0, 1.5, 1.0);
    
    glColor3f(0.5, 0.0, 1.0);
    glutSolidCube(1.0);

    glPopMatrix();
#else
	ASSERT(PIPE);

	PrimitiveBuilder	pb(PIPE);
    
    float	color[] = {0.5, 0.0, 1.0};

	GL::SolidCube(1.0, color);
#endif
}
// --------------------------------------------------------------------------
}

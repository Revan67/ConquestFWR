// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Article_h
#define Article_h
#include "StringType.h"
#include "DynamicSceneEntity.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Scene;
// --------------------------------------------------------------------------
//  Article
// --------------------------------------------------------------------------
class Article: public DynamicSceneEntity 
{
    public:
        Article(const ROSString& kNameR, bool makeNameUnique, Scene& scene);
        explicit Article(Scene& scene);

        virtual ROSString GetArchetypeName()  const;
        static ROSString GetArticleArchetypeName();

        virtual void Write(std::ostream& ostream) const;
        virtual void Read(std::istream& iStream);

    protected:
        virtual void Render(const ROS::DABaseCamera* camera) const;
		virtual bool FindIntersect(const IntersectInfo& intersectInfo, float* distance) const;

        void WriteSubObject(std::ostream& ostream) const;
        void ReadSubObject(std::istream& iStream);

    private:
        typedef DynamicSceneEntity BaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif

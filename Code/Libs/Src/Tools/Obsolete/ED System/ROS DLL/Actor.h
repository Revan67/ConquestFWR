// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Actor_h
#define Actor_h

#include "StringType.h"
#include "DynamicSceneEntity.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Scene;
// --------------------------------------------------------------------------
//  Actor
// --------------------------------------------------------------------------
class CPP_DECL Actor: public DynamicSceneEntity
{
    public:
        Actor(const ROSString& kNameR, bool makeNameUnique, Scene& scene);
        explicit Actor(Scene& scene);

        virtual ROSString GetArchetypeName() const;
        static ROSString GetActorArchetypeName();

        virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

    protected:
        virtual void Render(const ROS::DABaseCamera* camera) const;
		virtual bool FindIntersect(const IntersectInfo& intersectInfo, float* distance) const;

        void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);

    private:
        typedef DynamicSceneEntity BaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif

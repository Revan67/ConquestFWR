// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef PositionMarker_h
#define PositionMarker_h

#include "StringType.h"
#include "DynamicSceneEntity.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Scene;
// --------------------------------------------------------------------------
//  PositionMarker
// --------------------------------------------------------------------------
class CPP_DECL PositionMarker: public DynamicSceneEntity
{
    public:
        PositionMarker(const ROSString& kNameR, bool makeNameUnique, Scene& scene);
        explicit PositionMarker(Scene& scene);

        virtual ROSString GetArchetypeName() const;
        static ROSString GetPositionMarkerArchetypeName();

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

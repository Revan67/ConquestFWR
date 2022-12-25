// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef OrientationKeyPointMarker_h
#define OrientationKeyPointMarker_h
// --------------------------------------------------------------------------
#include "Marker.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  OrientationKeyPointMarker
// --------------------------------------------------------------------------
class CPP_DECL OrientationKeyPointMarker: public Marker
{
    public:
        OrientationKeyPointMarker(const ROSString& name, bool makeNameUnique, Scene& scene, AStaticSceneEntity& entity, unsigned int keyPointIndex);
		virtual ~OrientationKeyPointMarker();

		unsigned int GetKeyPointIndex() const;

        virtual ROSString GetArchetypeName() const;
        static ROSString GetOrientationKeyPointMarkerArchetypeName();

	protected:
		void SetKeyPointIndex(unsigned int keyPointIndex);
		virtual void Render(const ROS::DABaseCamera* camera) const;

    private:
        typedef Marker BaseClass;

		void SetName();

		const ROSString	mBaseName;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
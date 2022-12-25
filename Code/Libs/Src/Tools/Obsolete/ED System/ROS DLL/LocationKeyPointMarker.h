// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LocationKeyPointMarker_h
#define LocationKeyPointMarker_h
// --------------------------------------------------------------------------
#include "Marker.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  LocationKeyPointMarker
// --------------------------------------------------------------------------
class CPP_DECL LocationKeyPointMarker: public Marker
{
    public:
        LocationKeyPointMarker(const ROSString& name, bool makeNameUnique, Scene& scene, AStaticSceneEntity& entity, unsigned int keyPointIndex);

		unsigned int GetKeyPointIndex() const;

        virtual ROSString GetArchetypeName() const;
        static ROSString GetLocationKeyPointMarkerArchetypeName();

		virtual void RenderMarker(const ROS::DABaseCamera* camera) const;
		virtual void RenderPathToNextMarker(const ROS::DABaseCamera* camera) const;

	protected:
		virtual ~LocationKeyPointMarker();

		void SetKeyPointIndex(unsigned int keyPointIndex);

    private:
        typedef Marker BaseClass;

		void SetName();

		const ROSString	mBaseName;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
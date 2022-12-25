// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DADynamicSpotLight_h
#define DADynamicSpotLight_h
// --------------------------------------------------------------------------
#include "DynamicSpotLight.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  DADynamicSpotLight
// --------------------------------------------------------------------------
class CPP_DECL DADynamicSpotLight: public DynamicSpotLight
{
    public:
        DADynamicSpotLight(const ROSString& name, bool makeNameUnique, Scene& scene);
        explicit DADynamicSpotLight(Scene& scene);

        virtual ROSString GetArchetypeName() const;
        static ROSString GetDADynamicSpotLightArchetypeName();

		virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

    private:
        typedef DynamicSpotLight BaseClass;

		void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
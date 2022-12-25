// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DAAmbientLight_h
#define DAAmbientLight_h
// --------------------------------------------------------------------------
#include "AmbientLight.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	DAAmbientLight
// --------------------------------------------------------------------------
class DAAmbientLight: public AmbientLight 
{
	public:
		DAAmbientLight(const ROSString& name, bool makeNameUnique, Scene& scene);
        explicit DAAmbientLight(Scene& scene);

        virtual ROSString GetArchetypeName()  const;
        static ROSString GetDAAmbientLightArchetypeName();

		virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

	private:
    	typedef AmbientLight BaseClass;

		void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
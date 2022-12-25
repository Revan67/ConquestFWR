// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DAAmbientLightState_h
#define DAAmbientLightState_h
// --------------------------------------------------------------------------
#include "Color.h"
#include "ALightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	DAAmbientLightState
// --------------------------------------------------------------------------
class DAAmbientLightState: public ALightState
{
	public:
        DAAmbientLightState();
        explicit DAAmbientLightState(const Color& color);

		virtual ~DAAmbientLightState();

		virtual void SetColor(const Color& color);
		virtual Color GetColor() const;

        virtual void Write(std::ostream& oStream) const;
    	virtual void Read(std::istream& iStream);

	private:
		typedef ALightState BaseClass;

    	void WriteSubObject(std::ostream& oStream) const;
    	void ReadSubObject(std::istream& iStream);
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
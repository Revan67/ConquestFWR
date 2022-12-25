// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LightState_h
#define LightState_h
// --------------------------------------------------------------------------
#include "Color.h"
#include "ALightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	LightState
// --------------------------------------------------------------------------
class LightState: public ALightState
{
	public:
        LightState();
		LightState(const ALightState& lightState);
        explicit LightState(const Color& color);

		virtual void SetColor(const Color& color);
		virtual Color GetColor() const;

		LightState& operator=(const LightState& lightState);

        virtual void Write(std::ostream& oStream) const;
    	virtual void Read(std::istream& iStream);

	private:
    	void WriteSubObject(std::ostream& oStream) const;
    	void ReadSubObject(std::istream& iStream);

		Color	mColor;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ALightState_h
#define ALightState_h
// --------------------------------------------------------------------------
#include "APhysicalState.h"
#include "Color.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	LightState
// --------------------------------------------------------------------------
class ALightState: public virtual APhysicalState
{
	public:
		virtual ~ALightState();

		virtual void SetLightState(const ALightState& lightState);

		virtual void SetColor(const Color& color) = 0;
		virtual Color GetColor() const = 0;

		void Interpolate(const ALightState& nextState, float t, ALightState& tState) const;

	private:
		typedef APhysicalState BaseClass;

		ALightState& operator=(const ALightState& light);	// Declaring private and leaving undefined

};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
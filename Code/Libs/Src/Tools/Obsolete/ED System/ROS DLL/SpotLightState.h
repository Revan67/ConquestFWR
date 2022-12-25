// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SpotLightState_h
#define SpotLightState_h
// --------------------------------------------------------------------------
#include "ASpotLightState.h"
#include "LightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	SpotLightState
// --------------------------------------------------------------------------
class SpotLightState: public ASpotLightState
{
	public:
		SpotLightState();
		SpotLightState(const ASpotLightState& spotLightState);

		SpotLightState& operator=(const ASpotLightState& spotLightState);

		virtual void SetColor(const Color& color) { mLightState.SetColor(color); }
		virtual Color GetColor() const { return mLightState.GetColor(); }

		virtual void SetInfinite(bool infinite);
		virtual void SetRange(float range);
		virtual void SetCutOff(float cutOff);

		virtual bool IsInfinite() const;
		virtual float GetRange() const;
		virtual float GetCutOff() const;

		virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

	private:
		typedef ASpotLightState BaseClass;

		void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);

		LightState		mLightState;
		bool			mIsInfinite;
		float			mRange;
		float			mCutOff;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
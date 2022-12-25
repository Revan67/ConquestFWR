// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef CameraState_h
#define CameraState_h
// --------------------------------------------------------------------------
#include "ACameraState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  CameraState
// --------------------------------------------------------------------------
class CameraState: public virtual ACameraState
{
    public:
		CameraState(const ACameraState& cameraState);
		explicit CameraState(float hFOV = 45.0000, float vFOV = 36.8699);

        virtual float GetHorizontalFOV() const;
        virtual float GetVerticalFOV() const;

        virtual void SetHorizontalFOV(float hFOV);
        virtual void SetVerticalFOV(float vFOV);

		CameraState Interpolate(const CameraState& nextCamera, float t) const;

		virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

	private:
		typedef ACameraState BaseClass;

		void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);
		
		float mHorFOV;
		float mVerFOV;
};
// --------------------------------------------------------------------------
inline CameraState CameraState::Interpolate(const CameraState& nextCamera, float t) const
{
	const float diff = 1 - t;

	return CameraState(	(GetHorizontalFOV() * diff) + nextCamera.GetHorizontalFOV() * t,
    					(GetVerticalFOV() * diff) + nextCamera.GetVerticalFOV() * t);

}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif

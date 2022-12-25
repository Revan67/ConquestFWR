// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LiveCameraState_h
#define LiveCameraState_h
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class CPP_DECL LiveCameraState
{
	public:
		LiveCameraState();
		LiveCameraState(ADynamicCamera* rollingCamera);

		const ADynamicCamera* GetRollingCamera() const;
		ADynamicCamera* GetRollingCamera();

		void SetRollingCamera(ADynamicCamera* rollingCamera);

        void Write(std::ostream& oStream) const;
		void Read(std::istream& oStream);

    private :
        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

		ADynamicCamera* mRollingCamera;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::LiveCameraState& state)
{
	state.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::LiveCameraState& state)
{
	state.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif